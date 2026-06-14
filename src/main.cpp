// AvaGotchi — M5Stack Gray たまごっち風ペット育成ゲーム
//   キャラクター: M5Stack-Avatar / すれちがい: ESP-NOW / 天気: Open-Meteo
//   ボタンA: ごはん  ボタンB: ミニゲーム  ボタンC: ステータス  C長押し: 天気更新
#include <M5Unified.h>
#include <Avatar.h>
#include <time.h>
#include <sys/time.h>
#include "config.h"
#include "pet.h"
#include "streetpass.h"
#include "weather.h"
#include "minigame.h"

using namespace m5avatar;

Avatar avatar;
Pet pet;
WeatherInfo weather;
bool hasImu = false;          // IMU搭載か（Gray=有, Basic無印=無）。起動時に自動判定

// --- 個体差（チップ固有IDから決まる見た目・性格） ---
uint16_t gHue = 200;          // 体色(色相 0-359)
int gFavorite = 0;            // 好物index
const char *PERSONALITY_NAMES[4] = {"くいしんぼう", "あまえんぼう", "げんきっこ", "おっとり"};
const char *FAVORITES[5] = {"おさかな", "おにく", "くだもの", "おこめ", "おやつ"};

// HSV(色相0-359)から RGB565 を作る
uint16_t hsv565(uint16_t h, uint8_t s = 200, uint8_t v = 255) {
  float hh = h / 60.0f; int i = (int)hh; float f = hh - i;
  float vv = v / 255.0f, ss = s / 255.0f;
  float p = vv * (1 - ss), q = vv * (1 - ss * f), t = vv * (1 - ss * (1 - f));
  float r, g, b;
  switch (i % 6) {
    case 0: r = vv; g = t;  b = p;  break;
    case 1: r = q;  g = vv; b = p;  break;
    case 2: r = p;  g = vv; b = t;  break;
    case 3: r = p;  g = q;  b = vv; break;
    case 4: r = t;  g = p;  b = vv; break;
    default: r = vv; g = p; b = q;  break;
  }
  return M5.Display.color565((uint8_t)(r * 255), (uint8_t)(g * 255), (uint8_t)(b * 255));
}

// 解除済みの最高称号（実績バッジ）
const char *achievementTitle() {
  if (pet.st.goldMedals >= 10)   return "ゲームマスター";
  if (pet.st.battleWins >= 10)   return "バトルおうじゃ";
  if (pet.st.eggs >= 10)         return "でんせつの おやどり";
  if (pet.st.friends >= 20)      return "にんきもの";
  if (pet.st.totalFeeds >= 100)  return "たべるの だいすき";
  if (pet.st.eggs >= 3)          return "いっかの たいちょう";
  if (pet.st.friends >= 5)       return "ともだち おおい";
  return "みならい";
}

// すれちがい用に、自分の最新情報をブロードキャストデータへ反映
void updateMyInfo() {
  StreetPass::setMyInfo(PET_NAME, pet.st.eggs, pet.power(), (uint8_t)pet.personality, gHue);
}

uint32_t lastDecayMs = 0;
uint32_t speechUntilMs = 0;   // この時刻まで吹き出しを表示
uint32_t dizzyUntilMs = 0;    // 目が回っている間
String speechBuf;             // setSpeechTextへ渡す文字列の保持用

// --- IMUインタラクションの状態 ---
uint32_t lastShakeMs = 0;     // 直近に「目が回る」を検知した時刻
uint32_t lastTapMs = 0;       // 直近に「くすぐったい」を検知した時刻
int spinCount = 0;            // 連続して回転している(ジャイロ大)フレーム数
uint32_t upsideStartMs = 0;   // 上下逆さが始まった時刻（0=正常）
bool upsideNotified = false;  // 逆さに気付いて画面を出したか

// --- その他のインタラクション状態 ---
uint32_t grumpyUntilMs = 0;   // 「もう食べれない」不満顔の継続時刻
int fullPressCount = 0;       // 満腹状態でAを押した連続回数
int lastChimeHour = -1;       // 時報を鳴らした最後の「時」

// ジャイロ(角速度 deg/s)で回転を、加速度スパイクでタップを検出する
constexpr float GYRO_SPIN   = 200.0f;  // これ以上の角速度を「回転」とみなす（やや鈍く）
constexpr int   SPIN_FRAMES = 6;       // 連続で回り続けたら目が回る(約120ms)
constexpr float GYRO_QUIET  = 80.0f;   // これ未満なら「回っていない」
constexpr float TAP_MOTION  = 0.6f;    // 加速度スパイクをタップとみなすしきい値（やや鈍く）

// ---- 時刻が同期済みか（NTP後 or 物理RTCから復元済み） ----
bool clockReady() {
  struct tm t;
  if (!getLocalTime(&t, 10)) return false;
  return (t.tm_year + 1900) >= 2024;
}

// ---- 時間帯に応じた起動あいさつ ----
String greetingByTime() {
  struct tm t;
  if (!getLocalTime(&t, 10)) return "やっほー!";  // 時刻未同期（Cながおしで天気＝時刻あわせ）
  int h = t.tm_hour;
  if (h < 5)  return "こんな よふかし めっ!";
  if (h < 10) return "おはよう!";
  if (h < 17) return "こんにちは!";
  if (h < 22) return "こんばんは!";
  return "もう ねるじかんだよ";
}

// ---- 吹き出しを一定時間表示 ----
void say(const String &text, uint32_t ms = 3000) {
  speechBuf = text;
  avatar.setSpeechText(speechBuf.c_str());
  speechUntilMs = millis() + ms;
}

// ---- 天気に合わせて背景色を変える ----
void applyWeatherPalette() {
  ColorPalette cp;
  uint16_t bg = TFT_BLACK;
  switch (weather.kind) {
    case WeatherKind::Clear:   bg = M5.Display.color565(40, 90, 170);  break;  // 青空
    case WeatherKind::Cloudy:  bg = M5.Display.color565(90, 90, 100);  break;  // 灰色
    case WeatherKind::Fog:     bg = M5.Display.color565(120, 120, 120); break;
    case WeatherKind::Rain:    bg = M5.Display.color565(30, 40, 80);   break;  // 暗い青
    case WeatherKind::Snow:    bg = M5.Display.color565(150, 160, 180); break; // 白っぽい
    case WeatherKind::Thunder: bg = M5.Display.color565(60, 40, 90);   break;  // 紫
    default: break;
  }
  cp.set(COLOR_BACKGROUND, bg);
  cp.set(COLOR_PRIMARY, hsv565(gHue, 200, 255));  // 顔の色は個体ごとに異なる
  avatar.setColorPalette(cp);
}

// ---- ステータスに応じた表情 ----
void updateExpression() {
  if (millis() < dizzyUntilMs) {
    avatar.setExpression(Expression::Angry);  // 振り回されて目が回る
    return;
  }
  if (millis() < grumpyUntilMs) {
    avatar.setExpression(Expression::Angry);  // もう食べられない…ぷんぷん
    return;
  }
  if (pet.st.sleeping) {
    avatar.setExpression(Expression::Sleepy);
    return;
  }
  if (pet.st.satiety < 30) {
    avatar.setExpression(Expression::Sad);    // おなかぺこぺこ
  } else if (pet.st.happiness >= 70 || pet.st.satiety >= 90) {
    avatar.setExpression(Expression::Happy);  // ごきげん or 満腹で笑顔
  } else if (pet.st.happiness < 30) {
    avatar.setExpression(Expression::Doubt);  // かまってほしい
  } else {
    avatar.setExpression(Expression::Neutral);
  }
}

// ---- ステータス画面（Avatarを止めて描画） ----
void showStatus() {
  avatar.suspend();
  auto &lcd = M5.Display;
  const int W = lcd.width();
  lcd.fillScreen(TFT_BLACK);
  lcd.setFont(&fonts::efontJA_12);
  lcd.setTextColor(TFT_WHITE, TFT_BLACK);
  int y = 16;
  const int rowH = 24;
  const int barX = 112;                  // ラベル文字と重ならないようゲージを右へ
  const int barW = W - barX - 12;         // 画面幅に合わせてゲージ幅を自動調整
  auto bar = [&](const char *label, int v, uint16_t color) {
    lcd.setTextDatum(middle_left);        // 文字をバーの高さの中央に合わせる
    lcd.drawString(label, 10, y + 7);
    lcd.drawRect(barX, y, barW, 14, TFT_DARKGREY);
    lcd.fillRect(barX + 2, y + 2, (barW - 4) * v / 100, 10, color);
    y += rowH;
  };
  bar("まんぷく", pet.st.satiety, TFT_ORANGE);
  bar("ごきげん", pet.st.happiness, TFT_PINK);
  bar("げんき",   pet.st.energy, TFT_GREEN);

  y += 14;
  lcd.setTextDatum(top_left);
  lcd.drawString("ともだち: " + String(pet.st.friends) + "ひき", 10, y); y += 24;
  lcd.drawString("たまご  : " + String(pet.st.eggs) + "こ", 10, y);     y += 24;
  lcd.drawString("ねんれい: " + String(pet.st.ageMin) + "ふん", 10, y); y += 24;
  if (weather.valid) {
    lcd.drawString("てんき  : " + String(Weather::kindLabel(weather.kind)) +
                       " " + String(weather.temperature, 1) + "C",
                   10, y);
  } else {
    lcd.drawString("てんき  : Cながおしで", 10, y);
  }
  struct tm tnow;
  if (getLocalTime(&tnow, 10)) {           // 時刻が同期済みなら現在時刻も表示
    y += 24;
    char buf[8];
    snprintf(buf, sizeof(buf), "%02d:%02d", tnow.tm_hour, tnow.tm_min);
    lcd.drawString(String("じこく  : ") + buf, 10, y);
  }
  // 4秒経過、またはもう一度Cボタンを押したら即座に顔へ戻る
  uint32_t t0 = millis();
  while (millis() - t0 < 4000) {
    M5.update();
    if (M5.BtnC.wasClicked()) break;
    delay(20);
  }
  avatar.resume();
}

// ---- 天気の取得（ESP-NOWを一旦止めてWiFi接続） ----
void fetchWeather() {
  say("てんき しらべるね", 60000);
  StreetPass::end();
  bool ok = Weather::fetch(weather);
  StreetPass::begin();
  updateMyInfo();

  // 時刻が同期できていれば物理RTCにも保存（次回起動で復元。Gray非搭載機はスキップ）
  if (M5.Rtc.isEnabled() && clockReady()) {
    struct tm t;
    getLocalTime(&t, 10);
    M5.Rtc.setDateTime({{(int16_t)(t.tm_year + 1900), (int8_t)(t.tm_mon + 1), (int8_t)t.tm_mday, (int8_t)t.tm_wday},
                        {(int8_t)t.tm_hour, (int8_t)t.tm_min, (int8_t)t.tm_sec}});
  }

  if (ok) {
    applyWeatherPalette();
    say(String(Weather::kindLabel(weather.kind)) + "だよ! " +
        String(weather.temperature, 1) + "ど");
  } else {
    say("わかんなかった…");
  }
}

// ---- たまごイベント ----
void eggEvent() {
  say("たまご うんだよ!!", 6000);
  avatar.setExpression(Expression::Happy);
  delay(4000);
  pet.layEgg();
  updateMyInfo();
  say("あたらしい なかまだよ");
}

// ---- すれちがいバトル＆相性（ESP-NOWで出会った相手と総合力で対戦） ----
void battleWithFriend(const FriendInfo &fi) {
  uint16_t myPow = pet.power();
  say(String(fi.name) + "に あった！", 2500);
  delay(2500);
  say("バトル！ " + String(myPow) + " 対 " + String(fi.power), 3000);
  delay(3000);
  if (myPow > fi.power) {
    pet.st.battleWins++;
    pet.st.happiness = min(100, pet.st.happiness + 15);
    avatar.setExpression(Expression::Happy);
    say("かった〜！", 3000);
  } else if (myPow < fi.power) {
    avatar.setExpression(Expression::Sad);
    say("まけた… くやしい", 3000);
  } else {
    say("ひきわけ！", 3000);
  }
  delay(3000);
  // 体色(色相)の近さで相性を算出
  int dh = abs((int)gHue - (int)fi.hue);
  if (dh > 180) dh = 360 - dh;
  int aisho = 100 - dh * 100 / 180;
  avatar.setExpression(Expression::Neutral);
  say(String(PERSONALITY_NAMES[fi.personality & 3]) + "と\nあいしょう " + String(aisho) + "%", 3500);
  delay(3500);
  pet.save();
}

// ---- 逆さ専用画面：文字だけ180度回転して表示し、正立に戻るまで維持 ----
void showUpsideScreen() {
  avatar.suspend();  // Avatarを止めて画面を占有（再描画競合を防ぐ）
  auto &lcd = M5.Display;
  uint8_t rot = lcd.getRotation();
  lcd.fillScreen(hsv565(gHue, 120, 70));  // 個体色の下地
  int cx = lcd.width() / 2, cy = lcd.height() / 2;
  // びっくり顔（正立のまま）
  lcd.fillCircle(cx - 34, cy - 6, 9, TFT_WHITE);
  lcd.fillCircle(cx + 34, cy - 6, 9, TFT_WHITE);
  lcd.fillCircle(cx, cy + 28, 13, TFT_WHITE);
  // 「文字だけ」180度回転（逆さに持っている人が読める向き）
  lcd.setRotation(rot ^ 2);
  lcd.setFont(&fonts::efontJA_16);
  lcd.setTextColor(TFT_WHITE);
  lcd.setTextDatum(middle_center);
  lcd.drawString("さかさまだよ！", lcd.width() / 2, 34);
  lcd.setRotation(rot);
  // 正立に戻るまで待つ（少しヒステリシスを持たせてバタつき防止）
  while (true) {
    M5.update();
    float ax, ay, az;
    if (!M5.Imu.getAccel(&ax, &ay, &az)) break;
    if (!((az < -0.4f) || (ay < -0.6f))) break;
    delay(60);
  }
  avatar.resume();
}

// ---- じまんカード（QR＋成績）。スマホで読むとPagesのカードが開く ----
void showQrCard() {
  avatar.suspend();
  auto &lcd = M5.Display;
  lcd.fillScreen(TFT_BLACK);

  char url[200];
  snprintf(url, sizeof(url),
           "https://sin1.studio/AvaGotchi/card.html#p=%d&c=%u&d=%lu&e=%u&f=%u&g=%u&s=%u&b=%u&w=%u",
           pet.personality, gHue, (unsigned long)(pet.st.ageMin / 1440),
           pet.st.eggs, pet.st.friends, pet.st.goldMedals, pet.st.silverMedals,
           pet.st.bronzeMedals, pet.st.battleWins);
  lcd.qrcode(url, 8, 44, 140, 7);  // 左側にQR(140px)

  lcd.setTextColor(TFT_WHITE, TFT_BLACK);
  lcd.setTextDatum(top_left);
  lcd.setFont(&fonts::efontJA_16);
  lcd.drawString("じまんカード", 8, 8);

  lcd.setFont(&fonts::efontJA_12);  // 右側ステータス（画面右端からはみ出さない短い表記）
  int x = 156, y = 44;
  lcd.drawString(String(PERSONALITY_NAMES[pet.personality]), x, y);  y += 22;
  lcd.drawString("すき:" + String(FAVORITES[gFavorite]), x, y);      y += 22;
  lcd.drawString("そだて" + String(pet.st.ageMin / 1440) + "日", x, y); y += 22;
  lcd.drawString("たまご" + String(pet.st.eggs), x, y);              y += 22;
  lcd.drawString("ともだち" + String(pet.st.friends), x, y);          y += 22;
  lcd.drawString("金" + String(pet.st.goldMedals) + " 銀" + String(pet.st.silverMedals) +
                     " 銅" + String(pet.st.bronzeMedals), x, y);

  lcd.setTextColor(TFT_CYAN, TFT_BLACK);  // 称号は下部に全幅で
  lcd.drawString("「" + String(achievementTitle()) + "」", 8, 196);
  lcd.setFont(&fonts::efontJA_10);
  lcd.setTextColor(TFT_WHITE, TFT_BLACK);
  lcd.drawString("スマホでQR→SNSへ", 8, 220);

  // 15秒、またはいずれかのボタンで戻る
  uint32_t t0 = millis();
  while (millis() - t0 < 15000) {
    M5.update();
    if (M5.BtnA.wasClicked() || M5.BtnB.wasClicked() || M5.BtnC.wasClicked()) break;
    delay(20);
  }
  avatar.resume();
}

void setup() {
  auto cfg = M5.config();
  cfg.internal_imu = true;
  M5.begin(cfg);

  hasImu = M5.Imu.isEnabled();  // Gray等はtrue、IMU非搭載のBasic無印はfalse

  // タイムゾーンを日本時間に固定（mktime/localtimeが一貫してJSTになる）
  setenv("TZ", "JST-9", 1);
  tzset();

  // 物理RTCがあれば前回保存した時刻をシステム時刻へ復元（Gray非搭載機はスキップ）
  if (M5.Rtc.isEnabled()) {
    auto dt = M5.Rtc.getDateTime();
    if (dt.date.year >= 2024) {
      struct tm tmv = {};
      tmv.tm_year = dt.date.year - 1900;
      tmv.tm_mon  = dt.date.month - 1;
      tmv.tm_mday = dt.date.date;
      tmv.tm_hour = dt.time.hours;
      tmv.tm_min  = dt.time.minutes;
      tmv.tm_sec  = dt.time.seconds;
      time_t tt = mktime(&tmv);
      struct timeval tv = {tt, 0};
      settimeofday(&tv, nullptr);
    }
  }

  pet.load();

  // チップ固有IDから個体差（色・性格・好物）を決める
  uint64_t mac = ESP.getEfuseMac();
  uint32_t id = (uint32_t)(mac ^ (mac >> 32));
  pet.personality = id % 4;
  gHue = id % 360;
  gFavorite = (id / 4) % 5;

  avatar.init();
  avatar.setSpeechFont(&fonts::efontJA_12);  // 吹き出しは小さめにして長文も収める
  applyWeatherPalette();
  say(greetingByTime());  // 時間帯に応じたあいさつ

  StreetPass::begin();
  updateMyInfo();
  lastDecayMs = millis();
}

void loop() {
  M5.update();
  const uint32_t now = millis();

  // --- ボタン操作 ---
  if (M5.BtnA.wasHold()) {              // じまんカード(QR)を表示
    showQrCard();
  } else if (M5.BtnA.wasClicked()) {    // ごはん
    if (pet.st.sleeping) {
      pet.st.sleeping = false;          // 起こしてしまった
      say("ふぁ… おきたよ");
    } else if (pet.st.satiety >= 100) {
      // 満腹なのに何度もあげると不満になる
      if (++fullPressCount >= 2) {
        say("もう たべれないよ！");
        grumpyUntilMs = now + 3000;
      } else {
        say("おなか いっぱい");
      }
    } else {
      pet.feed();
      say(String(FAVORITES[gFavorite]) + " もぐもぐ!");
      fullPressCount = 0;
      for (int i = 0; i < 4; i++) {        // もぐもぐ口を動かす
        avatar.setMouthOpenRatio(0.8f);
        delay(120);
        avatar.setMouthOpenRatio(0.0f);
        delay(120);
      }
    }
  }
  if (M5.BtnB.wasClicked() && !pet.st.sleeping) {  // ミニゲーム選択
    // 天気が取得済みなら、その日の天気限定ゲームをメニューに加える
    int wIdx = -1;
    const char *wName = nullptr;
    if (weather.valid) {
      switch (weather.kind) {
        case WeatherKind::Clear:   wIdx = 4; wName = "はれ:ひなたぼっこ"; break;  // がまんくらべ
        case WeatherKind::Cloudy:  wIdx = 3; wName = "くもり:かずあて";   break;  // ラッキー7
        case WeatherKind::Rain:    wIdx = 1; wName = "あめ:あまやどり";   break;  // はんしゃ
        case WeatherKind::Snow:    wIdx = 2; wName = "ゆき:ゆきかき";     break;  // れんだ
        case WeatherKind::Thunder: wIdx = 1; wName = "かみなり:よけ";     break;  // はんしゃ
        default: break;
      }
    }
    int up = MiniGame::selectAndPlay(hasImu, wIdx, wName);
    if (up >= 0) {
      pet.play(up);
      pet.recordMedal(up);   // 金・銀・銅を実績に記録
      updateMyInfo();
      say(up >= 25 ? "たのしかった〜!!" : up >= 10 ? "また あそぼ!" : "うーん…ざんねん");
    }
  }
  if (M5.BtnC.wasHold()) {              // 天気更新
    fetchWeather();
  } else if (M5.BtnC.wasClicked()) {    // ステータス
    showStatus();
  }

  // --- IMU: 回転(目が回る) / タップ(くすぐったい) / 傾き / 上下逆さ ---
  //     IMU搭載機(Gray等)のみ。Basic無印(IMU無し)では丸ごとスキップ。
  float ax, ay, az, gx, gy, gz;
  bool haveAccel = hasImu && M5.Imu.getAccel(&ax, &ay, &az);
  bool haveGyro  = hasImu && M5.Imu.getGyro(&gx, &gy, &gz);
  if (haveAccel) {
    float motion = fabsf(sqrtf(ax * ax + ay * ay + az * az) - 1.0f);  // 重力を除いた動き
    float gyroMag = haveGyro ? sqrtf(gx * gx + gy * gy + gz * gz) : 0;  // 角速度 deg/s

    // 回転が連続したら目が回る
    if (gyroMag > GYRO_SPIN) {
      spinCount++;
      if (spinCount == SPIN_FRAMES && now - lastShakeMs > 1500) {
        lastShakeMs = now;
        if (pet.st.sleeping) {
          pet.st.sleeping = false;
          say("びっくりした!");
        } else {
          pet.shaken();
          dizzyUntilMs = now + 2500;
          say("めが まわる〜");
        }
      }
    } else {
      spinCount = 0;
    }

    // タップ：加速度スパイクがあり、かつ回転していない（つつかれた）
    if (motion > TAP_MOTION && gyroMag < GYRO_QUIET && now - lastTapMs > 500) {
      lastTapMs = now;
      if (pet.st.sleeping) {
        say("う〜ん…");  // そっと触られた程度では起きない
      } else {
        int d = (pet.personality == 1) ? 3 : 1;  // 甘えん坊は喜びやすい
        pet.st.happiness = min(100, pet.st.happiness + d);
        say("くすぐったい!", 1500);
      }
    }

    // --- 上下逆さ検知（頭が下） ---
    // 通常姿勢で重力は概ね +Z(平置き) か +Y(立て持ち)。逆さでこれらが負に振れる。
    // ★実機の持ち方で軸の出方が変わるので、必要なら下のしきい値/軸を調整。
    bool upsideNow = (az < -0.5f) || (ay < -0.7f);
    if (upsideNow) {
      if (upsideStartMs == 0) upsideStartMs = now;
      else if (!upsideNotified && now - upsideStartMs > 700) {  // 0.7秒続いたら気付く
        upsideNotified = true;
        showUpsideScreen();      // 正立に戻るまで逆さ画面（文字だけ反転）
        upsideStartMs = 0;
        upsideNotified = false;
      }
    } else {
      upsideStartMs = 0;
      upsideNotified = false;
    }

    // ゆるい傾きで顔も一緒に傾く（目が回る間は固定）
    if (now >= dizzyUntilMs) {
      float tilt = constrain(ax, -0.5f, 0.5f) * 20.0f;  // 最大±10度
      avatar.setRotation(tilt);
    }
  }

  // --- すれちがい通信＆バトル ---
  updateMyInfo();          // 送信前に総合力などを最新化
  StreetPass::update();
  FriendInfo fi;
  if (StreetPass::popNewFriend(fi)) {
    pet.meetFriend();
    battleWithFriend(fi);  // 総合力で対戦＋相性診断
    updateMyInfo();
  }

  // --- 毎正時の時報（時刻が同期済みのときだけ） ---
  {
    struct tm tnow;
    if (getLocalTime(&tnow, 0) && (tnow.tm_year + 1900) >= 2024) {
      if (tnow.tm_min == 0 && tnow.tm_hour != lastChimeHour) {
        lastChimeHour = tnow.tm_hour;
        say(String(tnow.tm_hour) + "じだよ！ ポーン", 4000);
      }
    }
  }

  // --- 時間経過 ---
  if (now - lastDecayMs >= DECAY_INTERVAL_MS) {
    lastDecayMs = now;
    pet.decay();
    pet.save();
    if (pet.checkEgg()) eggEvent();
    // ときどき天気の感想をつぶやく
    if (weather.valid && random(4) == 0) {
      switch (weather.kind) {
        case WeatherKind::Rain:    say("あめ ざーざー"); break;
        case WeatherKind::Clear:   say("いい てんき!");  break;
        case WeatherKind::Snow:    say("ゆきだ! さむ〜"); break;
        case WeatherKind::Thunder: say("かみなり こわい"); break;
        default: break;
      }
    }
  }

  // --- 吹き出しの消去と表情更新 ---
  if (speechUntilMs && now > speechUntilMs) {
    speechUntilMs = 0;
    avatar.setSpeechText("");
  }
  updateExpression();

  delay(20);
}
