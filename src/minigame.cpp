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

// timeoutMs の間にBがクリックされたら true（経過msをelapsedで返す）。
// Avatarは裏で描画を続けるので、ここではM5.update()だけ回す。
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

// ===== 各ゲーム（戻り値＝ごきげん上昇量 0〜30） =====

// 1. 5びょうあて：合図から5秒ぴったりでBを押す
int gameTiming() {
  avatar.setExpression(Expression::Happy);
  sayG("5びょう ぴったりで\nBを おしてね！");
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
  sayG("「いまだ！」で\nすぐ Bを おして");
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
  sayG("7を かぞえたら\nBを おして！");
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
  sayG("にらめっこ！\nわらったら Bで こうさん");
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

const char *MENU[5] = {"5びょう あて", "はんしゃ しんけい", "れんだ", "ラッキー7", "がまんくらべ"};

// ゲーム選択画面（Avatarを止めてリスト表示）。B短押しで送り、B長押しで決定。
int selectMenu() {
  avatar.suspend();
  auto &lcd = M5.Display;
  int idx = 0;
  auto draw = [&]() {
    lcd.fillScreen(TFT_BLACK);
    lcd.setTextColor(TFT_WHITE, TFT_BLACK);
    lcd.setTextDatum(top_left);
    lcd.setFont(&fonts::efontJA_16);
    lcd.drawString("ゲームを えらぶ", 8, 6);
    lcd.setFont(&fonts::efontJA_12);
    lcd.drawString("Bおす:つぎ  Bながおし:けってい", 8, 28);
    lcd.setFont(&fonts::efontJA_16);
    for (int i = 0; i < 5; i++) {
      lcd.setTextColor(i == idx ? TFT_YELLOW : TFT_WHITE, TFT_BLACK);
      lcd.drawString((i == idx ? "> " : "  ") + String(MENU[i]), 16, 52 + i * 30);
    }
  };
  draw();
  waitButtonRelease();
  uint32_t idleStart = millis();
  while (true) {
    M5.update();
    if (M5.BtnB.wasHold()) {  // 長押しで決定
      avatar.resume();
      waitButtonRelease();
      return idx;
    }
    if (M5.BtnB.wasClicked()) {  // 短押しで次へ
      idx = (idx + 1) % 5;
      draw();
      idleStart = millis();
    }
    if (millis() - idleStart > 15000) {  // 放置でキャンセル
      avatar.resume();
      return -1;
    }
    delay(20);
  }
}

}  // namespace

namespace MiniGame {

int selectAndPlay() {
  int sel = selectMenu();
  if (sel < 0) {
    clearSpeech();
    return -1;
  }
  int up = 0;
  switch (sel) {
    case 0: up = gameTiming();   break;
    case 1: up = gameReaction(); break;
    case 2: up = gameMash();     break;
    case 3: up = gameLucky7();   break;
    case 4: up = gameStare();    break;
  }
  clearSpeech();
  return up;
}

}  // namespace MiniGame
