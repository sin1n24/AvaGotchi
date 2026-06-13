#include "minigame.h"
#include <M5Unified.h>
#include <Avatar.h>

using namespace m5avatar;
extern Avatar avatar;  // main.cpp のグローバル

namespace {

// Avatarのセリフはポインタ参照されるので、寿命のある実体に保持する
String g_speech;
void sayG(const String &s) {
  g_speech = s;
  avatar.setSpeechText(g_speech.c_str());
}
void clearSpeech() {
  g_speech = "";
  avatar.setSpeechText("");
}

// timeoutMs の間にBがクリックされたら true（経過msをelapsedで返す）
bool waitClick(uint32_t timeoutMs, uint32_t *elapsed = nullptr) {
  uint32_t t0 = millis();
  while (millis() - t0 < timeoutMs) {
    M5.update();
    if (M5.BtnB.wasClicked()) {
      if (elapsed) *elapsed = millis() - t0;
      return true;
    }
    delay(10);
  }
  if (elapsed) *elapsed = timeoutMs;
  return false;
}

// 前のゲームのボタン押下を持ち越さないよう、離すまで待つ
void waitButtonRelease() {
  do {
    M5.update();
    delay(10);
  } while (M5.BtnB.isPressed());
}

// ごきげん上昇量(0-30)を金・銀・銅の3段階メダルで評価して表示
void showMedal(int up) {
  const char *m = up >= 25 ? "きんメダル！" : up >= 15 ? "ぎんメダル！" : "どうメダル！";
  avatar.setExpression(up >= 15 ? Expression::Happy : Expression::Neutral);
  sayG(m);
  delay(2600);
}

// ===== 会話型ゲーム（Bボタンのみ・戻り値＝ごきげん上昇量 0〜30） =====

// 1. 5びょうあて：合図から5秒ぴったりでBを押す
int gameTiming() {
  avatar.setExpression(Expression::Happy);
  sayG("5びょうで\nBを おして！");
  delay(2800);
  sayG("ようい…");
  delay(random(800, 1700));
  sayG("スタート！");
  uint32_t el;
  bool ok = waitClick(10000, &el);
  if (!ok) {
    sayG("おそすぎ〜！");
    avatar.setExpression(Expression::Sad);
    delay(2500);
    return 5;
  }
  float sec = el / 1000.0f;
  float err = fabsf(sec - 5.0f);
  sayG(String(sec, 2) + "びょう！\nごさ " + String(err, 2) + "びょう");
  avatar.setExpression(err < 0.3f ? Expression::Happy : Expression::Doubt);
  delay(3200);
  return err < 0.1f ? 30 : err < 0.3f ? 25 : err < 0.7f ? 18 : err < 1.5f ? 10 : 5;
}

// 2. はんしゃしんけい：「いまだ！」で素早くB
int gameReaction() {
  avatar.setExpression(Expression::Neutral);
  sayG("あいずで\nすぐ Bを！");
  delay(2800);
  sayG("……");
  uint32_t waitMs = random(1500, 4500), t0 = millis();
  while (millis() - t0 < waitMs) {  // 合図前に押すとフライング
    M5.update();
    if (M5.BtnB.wasClicked()) {
      sayG("はやい！フライング！");
      avatar.setExpression(Expression::Angry);
      delay(2500);
      return 5;
    }
    delay(10);
  }
  sayG("いまだ！");
  avatar.setExpression(Expression::Happy);
  uint32_t el;
  if (!waitClick(2000, &el)) {
    sayG("ねぼけてる〜？");
    avatar.setExpression(Expression::Sleepy);
    delay(2500);
    return 5;
  }
  sayG(String(el) + "ミリびょう！");
  delay(2800);
  return el < 250 ? 30 : el < 400 ? 22 : el < 600 ? 15 : 8;
}

// 3. れんだ：5秒間でBを何回押せるか
int gameMash() {
  sayG("5びょうかん\nBを れんだ！");
  delay(2800);
  sayG("スタート！");
  avatar.setExpression(Expression::Happy);
  uint32_t t0 = millis();
  int cnt = 0;
  while (millis() - t0 < 5000) {
    M5.update();
    if (M5.BtnB.wasClicked()) cnt++;
    delay(5);
  }
  sayG(String(cnt) + "かい おした！");
  avatar.setExpression(cnt >= 30 ? Expression::Happy : Expression::Neutral);
  delay(2800);
  return cnt >= 40 ? 30 : cnt >= 25 ? 22 : cnt >= 15 ? 15 : 8;
}

// 4. ラッキー7：数を数えて7でBを押す
int gameLucky7() {
  sayG("7で Bを\nおして！");
  delay(2800);
  int pressedAt = -1;
  for (int n = 1; n <= 12; n++) {
    sayG(String(n));
    avatar.setExpression(n == 7 ? Expression::Happy : Expression::Neutral);
    uint32_t el;
    if (waitClick(750, &el)) {
      pressedAt = n;
      break;
    }
  }
  if (pressedAt < 0) {
    sayG("おさなかったね…");
    avatar.setExpression(Expression::Sad);
    delay(2500);
    return 5;
  }
  int d = abs(pressedAt - 7);
  sayG(d == 0 ? "ぴったり7！てんさい！" : String(pressedAt) + "…7とは " + String(d) + "ちがい");
  avatar.setExpression(d == 0 ? Expression::Happy : Expression::Doubt);
  delay(3200);
  return d == 0 ? 30 : d == 1 ? 18 : 10;
}

// 5. がまんくらべ（にらめっこ）：変な顔に耐えた時間を競う
int gameStare() {
  sayG("にらめっこ！\nわらったら B");
  delay(2800);
  Expression faces[] = {Expression::Angry, Expression::Doubt, Expression::Sad, Expression::Happy};
  const char *lines[] = {"あっぷっぷ！", "ふふ〜ん", "へんなかお！", "むふ〜"};
  uint32_t t0 = millis(), el = 0;
  bool gaveup = false;
  while (millis() - t0 < 12000) {
    int r = random(4);
    avatar.setExpression(faces[r]);
    sayG(lines[r]);
    uint32_t seg;
    if (waitClick(700, &seg)) {
      gaveup = true;
      el = millis() - t0;
      break;
    }
  }
  if (!gaveup) el = 12000;
  float sec = el / 1000.0f;
  avatar.setExpression(Expression::Neutral);
  sayG(String(sec, 1) + "びょう がまん！");
  delay(2800);
  return sec >= 10 ? 30 : sec >= 6 ? 22 : sec >= 3 ? 14 : 8;
}

// ===== 6. かたむけて エサとり（IMU傾け操作。Gray等のみ） =====

// ペットの顔（ボール）。0:ふつう 1:うれしい(エサ) 2:ぶつかった(壁)
enum Mood { MOOD_NORMAL, MOOD_HAPPY, MOOD_BONK };
void drawPet(M5GFX &lcd, int x, int y, int r, int mood) {
  lcd.fillCircle(x, y, r, TFT_CYAN);
  const int ex = r / 2, ey = r / 4;
  if (mood == MOOD_HAPPY) {  // ^ ^ のにっこり＋口
    lcd.drawLine(x - ex - 2, y - ey + 2, x - ex, y - ey - 2, TFT_BLACK);
    lcd.drawLine(x - ex, y - ey - 2, x - ex + 2, y - ey + 2, TFT_BLACK);
    lcd.drawLine(x + ex - 2, y - ey + 2, x + ex, y - ey - 2, TFT_BLACK);
    lcd.drawLine(x + ex, y - ey - 2, x + ex + 2, y - ey + 2, TFT_BLACK);
    lcd.drawLine(x - 3, y + ey + 1, x + 3, y + ey + 1, TFT_BLACK);
  } else if (mood == MOOD_BONK) {  // × × の目を回した顔
    lcd.drawLine(x - ex - 2, y - ey - 2, x - ex + 2, y - ey + 2, TFT_BLACK);
    lcd.drawLine(x - ex + 2, y - ey - 2, x - ex - 2, y - ey + 2, TFT_BLACK);
    lcd.drawLine(x + ex - 2, y - ey - 2, x + ex + 2, y - ey + 2, TFT_BLACK);
    lcd.drawLine(x + ex + 2, y - ey - 2, x + ex - 2, y - ey + 2, TFT_BLACK);
  } else {  // ・ ・ のふつうの目
    lcd.fillCircle(x - ex, y - ey, 2, TFT_BLACK);
    lcd.fillCircle(x + ex, y - ey, 2, TFT_BLACK);
  }
}

// 本体を傾けてペットを転がし、制限時間内にいくつエサ(的)に到達できるかを競う。
// スプライトは使わず差分描画（Avatarは呼び出し側でsuspend済みの前提）。
int gameBall() {
  auto &lcd = M5.Display;
  const int W = lcd.width(), H = lcd.height();
  const uint32_t kTimeLimitMs = 20000;
  const int kStatusH = 20;
  float bx = W / 2.0f, by = H / 2.0f, vx = 0, vy = 0;
  const float kBallR = 11, kTargetR = 14;
  int score = 0;
  uint32_t happyUntil = 0, bonkUntil = 0;

  randomSeed(millis());
  float tx = random(20, W - 20), ty = random(kStatusH + 20, H - 20);

  // カウントダウン
  lcd.fillScreen(TFT_BLACK);
  lcd.setFont(&fonts::efontJA_24);
  lcd.setTextColor(TFT_WHITE, TFT_BLACK);
  lcd.setTextDatum(middle_center);
  drawPet(lcd, W / 2, H / 2 - 50, 22, MOOD_HAPPY);
  lcd.drawString("かたむけて エサへ！", W / 2, H / 2);
  for (int i = 3; i > 0; i--) {
    lcd.fillRect(W / 2 - 20, H / 2 + 28, 40, 34, TFT_BLACK);
    lcd.drawString(String(i), W / 2, H / 2 + 44);
    delay(800);
  }

  lcd.fillScreen(TFT_BLACK);
  const uint32_t start = millis();
  float pbx = bx, pby = by, ptx = tx, pty = ty;
  int pscore = -1;
  uint32_t premain = 0xffffffff;

  while (millis() - start < kTimeLimitMs) {
    M5.update();
    const uint32_t now = millis();
    float ax, ay, az;
    M5.Imu.getAccel(&ax, &ay, &az);

    vx += -ax * 1.8f;
    vy += ay * 1.8f;
    vx *= 0.95f;
    vy *= 0.95f;
    bx += vx;
    by += vy;

    bool bonk = false;
    if (bx < kBallR)            { bx = kBallR;            vx = -vx * 0.6f; bonk |= fabsf(vx) > 2; }
    if (bx > W - kBallR)        { bx = W - kBallR;        vx = -vx * 0.6f; bonk |= fabsf(vx) > 2; }
    if (by < kStatusH + kBallR) { by = kStatusH + kBallR; vy = -vy * 0.6f; bonk |= fabsf(vy) > 2; }
    if (by > H - kBallR)        { by = H - kBallR;        vy = -vy * 0.6f; bonk |= fabsf(vy) > 2; }
    if (bonk) bonkUntil = now + 250;

    float dx = bx - tx, dy = by - ty;
    if (dx * dx + dy * dy < (kBallR + kTargetR) * (kBallR + kTargetR)) {
      score++;
      happyUntil = now + 350;
      tx = random(20, W - 20);
      ty = random(kStatusH + 20, H - 20);
    }

    int mood = (now < happyUntil) ? MOOD_HAPPY : (now < bonkUntil ? MOOD_BONK : MOOD_NORMAL);

    lcd.fillCircle(pbx, pby, kBallR, TFT_BLACK);
    if (ptx != tx || pty != ty) lcd.fillCircle(ptx, pty, kTargetR, TFT_BLACK);
    lcd.fillCircle(tx, ty, kTargetR, TFT_RED);
    lcd.fillCircle(tx, ty, kTargetR - 6, TFT_YELLOW);
    drawPet(lcd, bx, by, kBallR, mood);

    uint32_t remain = (kTimeLimitMs - (now - start)) / 1000;
    if (score != pscore || remain != premain) {
      lcd.fillRect(0, 0, W, kStatusH, TFT_BLACK);
      lcd.setFont(&fonts::efontJA_12);
      lcd.setTextColor(TFT_WHITE, TFT_BLACK);
      lcd.setTextDatum(top_left);
      lcd.drawString("のこり:" + String(remain) + "  たべた:" + String(score), 4, 4);
      pscore = score;
      premain = remain;
    }

    pbx = bx; pby = by; ptx = tx; pty = ty;
    delay(16);
  }

  int up = score > 10 ? 30 : score * 3;
  const char *m = up >= 25 ? "きんメダル！" : up >= 15 ? "ぎんメダル！" : "どうメダル！";
  uint16_t mc = up >= 25 ? TFT_YELLOW : up >= 15 ? TFT_WHITE : TFT_ORANGE;

  lcd.fillScreen(TFT_BLACK);
  drawPet(lcd, W / 2, H / 2 - 34, 26, up >= 15 ? MOOD_HAPPY : MOOD_NORMAL);
  lcd.setFont(&fonts::efontJA_24);
  lcd.setTextColor(TFT_WHITE, TFT_BLACK);
  lcd.setTextDatum(middle_center);
  lcd.drawString("たべた: " + String(score), W / 2, H / 2 + 16);
  lcd.setTextColor(mc, TFT_BLACK);
  lcd.drawString(m, W / 2, H / 2 + 50);
  delay(2600);

  return up;
}

// ===== 選択画面 =====
// B短押しでカーソル移動、B長押しで決定（Avatarを止めてリスト表示）
int selectMenu(const char **menu, int count) {
  avatar.suspend();
  auto &lcd = M5.Display;
  int idx = 0;
  auto draw = [&]() {
    lcd.fillScreen(TFT_BLACK);
    lcd.setTextColor(TFT_WHITE, TFT_BLACK);
    lcd.setTextDatum(top_left);
    lcd.setFont(&fonts::efontJA_14);
    lcd.drawString("ゲームを えらぶ", 8, 6);
    lcd.setFont(&fonts::efontJA_10);
    lcd.drawString("Bおす:つぎ  Bながおし:けってい", 8, 26);
    lcd.setFont(&fonts::efontJA_12);
    for (int i = 0; i < count; i++) {
      lcd.setTextColor(i == idx ? TFT_YELLOW : TFT_WHITE, TFT_BLACK);
      lcd.drawString((i == idx ? "> " : "  ") + String(menu[i]), 16, 46 + i * 22);
    }
  };
  draw();
  waitButtonRelease();
  uint32_t idleStart = millis();
  while (true) {
    M5.update();
    if (M5.BtnB.wasHold()) {  // 長押しで決定
      waitButtonRelease();
      return idx;
    }
    if (M5.BtnB.wasClicked()) {  // 短押しで次へ
      idx = (idx + 1) % count;
      draw();
      idleStart = millis();
    }
    if (millis() - idleStart > 15000) return -1;  // 放置でキャンセル
    delay(20);
  }
}

}  // namespace

namespace MiniGame {

int selectAndPlay(bool imuAvailable) {
  // IMUがある機種(Gray等)だけ傾け操作の「エサとり」を加える
  const char *menu[6] = {"5びょう あて", "はんしゃ しんけい", "れんだ", "ラッキー7", "がまんくらべ"};
  int count = 5;
  if (imuAvailable) menu[count++] = "かたむけて エサとり";

  int sel = selectMenu(menu, count);  // この中でavatar.suspend()
  if (sel < 0) {
    avatar.resume();
    clearSpeech();
    return -1;
  }

  int up = 0;
  if (imuAvailable && sel == 5) {
    up = gameBall();    // 傾けゲームはsuspendのままLCD直書き（メダルもLCDで表示）
    avatar.resume();
  } else {
    avatar.resume();    // 会話ゲームはAvatarを使う
    switch (sel) {
      case 0: up = gameTiming();   break;
      case 1: up = gameReaction(); break;
      case 2: up = gameMash();     break;
      case 3: up = gameLucky7();   break;
      case 4: up = gameStare();    break;
    }
    showMedal(up);      // 金・銀・銅で評価
  }
  clearSpeech();
  return up;
}

}  // namespace MiniGame
