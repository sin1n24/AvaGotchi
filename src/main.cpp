// AvaGotchi — M5Stack Gray たまごっち風ペット育成ゲーム
//   キャラクター: M5Stack-Avatar / すれちがい: ESP-NOW / 天気: Open-Meteo
//   ボタンA: ごはん  ボタンB: ミニゲーム  ボタンC: ステータス  C長押し: 天気更新
#include <M5Unified.h>
#include <Avatar.h>
#include "config.h"
#include "pet.h"
#include "streetpass.h"
#include "weather.h"
#include "minigame.h"

using namespace m5avatar;

Avatar avatar;
Pet pet;
WeatherInfo weather;

uint32_t lastDecayMs = 0;
uint32_t speechUntilMs = 0;   // この時刻まで吹き出しを表示
uint32_t dizzyUntilMs = 0;    // 目が回っている間
uint32_t lastShakeMs = 0;
String speechBuf;             // setSpeechTextへ渡す文字列の保持用

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
  cp.set(COLOR_PRIMARY, TFT_WHITE);
  avatar.setColorPalette(cp);
}

// ---- ステータスに応じた表情 ----
void updateExpression() {
  if (millis() < dizzyUntilMs) {
    avatar.setExpression(Expression::Angry);  // 振り回されてプンプン
    return;
  }
  if (pet.st.sleeping) {
    avatar.setExpression(Expression::Sleepy);
    return;
  }
  if (pet.st.satiety < 30) {
    avatar.setExpression(Expression::Sad);    // おなかぺこぺこ
  } else if (pet.st.happiness >= 70) {
    avatar.setExpression(Expression::Happy);
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
  lcd.fillScreen(TFT_BLACK);
  lcd.setFont(&fonts::efontJA_16);
  lcd.setTextColor(TFT_WHITE, TFT_BLACK);
  lcd.setTextDatum(top_left);
  int y = 12;
  auto bar = [&](const char *label, int v, uint16_t color) {
    lcd.drawString(label, 12, y);
    lcd.drawRect(100, y, 204, 14, TFT_DARKGREY);
    lcd.fillRect(102, y + 2, v * 2, 10, color);
    y += 28;
  };
  bar("まんぷく", pet.st.satiety, TFT_ORANGE);
  bar("ごきげん", pet.st.happiness, TFT_PINK);
  bar("げんき",   pet.st.energy, TFT_GREEN);
  lcd.drawString("ともだち: " + String(pet.st.friends) + "ひき", 12, y); y += 24;
  lcd.drawString("たまご  : " + String(pet.st.eggs) + "こ", 12, y);     y += 24;
  lcd.drawString("ねんれい: " + String(pet.st.ageMin) + "ふん", 12, y); y += 24;
  if (weather.valid) {
    lcd.drawString("てんき  : " + String(Weather::kindLabel(weather.kind)) +
                       " " + String(weather.temperature, 1) + "C",
                   12, y);
  } else {
    lcd.drawString("てんき  : Cながおしで しゅとく", 12, y);
  }
  delay(4000);
  avatar.resume();
}

// ---- 天気の取得（ESP-NOWを一旦止めてWiFi接続） ----
void fetchWeather() {
  say("てんき しらべるね", 60000);
  StreetPass::end();
  bool ok = Weather::fetch(weather);
  StreetPass::begin();
  StreetPass::setMyInfo(pet.st.eggs);
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
  StreetPass::setMyInfo(pet.st.eggs);
  say("あたらしい なかまだよ");
}

void setup() {
  auto cfg = M5.config();
  cfg.internal_imu = true;
  M5.begin(cfg);

  pet.load();

  avatar.init();
  avatar.setSpeechFont(&fonts::efontJA_16);
  applyWeatherPalette();
  say("おはよう!");

  StreetPass::begin();
  StreetPass::setMyInfo(pet.st.eggs);
  lastDecayMs = millis();
}

void loop() {
  M5.update();
  const uint32_t now = millis();

  // --- ボタン操作 ---
  if (M5.BtnA.wasPressed()) {           // ごはん
    if (pet.st.sleeping) {
      pet.st.sleeping = false;          // 起こしてしまった
      say("ふぁ… おきたよ");
    } else if (pet.st.satiety >= 100) {
      say("おなか いっぱい");
    } else {
      pet.feed();
      say("もぐもぐ おいしい!");
    }
  }
  if (M5.BtnB.wasPressed() && !pet.st.sleeping) {  // ミニゲーム
    avatar.suspend();
    int score = MiniGame::playBallGame();
    avatar.resume();
    pet.play(score);
    say(score >= 30 ? "たのしかった〜!!" : "もっと あそぼ?");
  }
  if (M5.BtnC.wasHold()) {              // 天気更新
    fetchWeather();
  } else if (M5.BtnC.wasClicked()) {    // ステータス
    showStatus();
  }

  // --- IMU: 振る・傾けるインタラクション ---
  float ax, ay, az;
  if (M5.Imu.getAccel(&ax, &ay, &az)) {
    float mag = sqrtf(ax * ax + ay * ay + az * az);
    if (mag > 2.2f && now - lastShakeMs > 1500) {  // 強く振られた
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
    // ゆるい傾きで顔も一緒に傾く
    if (now >= dizzyUntilMs) {
      float tilt = constrain(ax, -0.5f, 0.5f) * 20.0f;  // 最大±10度
      avatar.setRotation(tilt);
    }
  }

  // --- すれちがい通信 ---
  StreetPass::update();
  FriendInfo fi;
  if (StreetPass::popNewFriend(fi)) {
    pet.meetFriend();
    say(String(fi.name) + "に あったよ!", 5000);
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
