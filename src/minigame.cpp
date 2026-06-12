#include "minigame.h"
#include <M5Unified.h>

namespace MiniGame {

// 本体を傾けてボールを転がし、制限時間内にいくつ的に当てられるかを競う
int playBallGame() {
  auto &lcd = M5.Display;
  const int W = lcd.width(), H = lcd.height();
  const uint32_t kTimeLimitMs = 20000;

  float bx = W / 2.0f, by = H / 2.0f;  // ボール位置
  float vx = 0, vy = 0;                // ボール速度
  const float kBallR = 8, kTargetR = 14;
  int score = 0;

  // 最初の的
  randomSeed(millis());
  float tx = random(20, W - 20), ty = random(40, H - 20);

  // カウントダウン
  lcd.fillScreen(TFT_BLACK);
  lcd.setFont(&fonts::efontJA_24);
  lcd.setTextColor(TFT_WHITE, TFT_BLACK);
  lcd.setTextDatum(middle_center);
  lcd.drawString("かたむけて まとに あてろ!", W / 2, H / 2 - 20);
  for (int i = 3; i > 0; i--) {
    lcd.drawString(String(i), W / 2, H / 2 + 20);
    delay(800);
  }

  M5Canvas canvas(&lcd);
  canvas.createSprite(W, H);
  const uint32_t start = millis();

  while (millis() - start < kTimeLimitMs) {
    M5.update();
    float ax, ay, az;
    M5.Imu.getAccel(&ax, &ay, &az);

    // 傾きを加速度に変換（M5Stack横持ち: X軸→左右, Y軸→上下）
    vx += -ax * 1.8f;
    vy += ay * 1.8f;
    vx *= 0.95f;  // 摩擦
    vy *= 0.95f;
    bx += vx;
    by += vy;

    // 壁で跳ね返る
    if (bx < kBallR)      { bx = kBallR;      vx = -vx * 0.6f; }
    if (bx > W - kBallR)  { bx = W - kBallR;  vx = -vx * 0.6f; }
    if (by < kBallR)      { by = kBallR;      vy = -vy * 0.6f; }
    if (by > H - kBallR)  { by = H - kBallR;  vy = -vy * 0.6f; }

    // 的に当たったか
    float dx = bx - tx, dy = by - ty;
    if (dx * dx + dy * dy < (kBallR + kTargetR) * (kBallR + kTargetR)) {
      score++;
      tx = random(20, W - 20);
      ty = random(40, H - 20);
    }

    // 描画
    canvas.fillScreen(TFT_BLACK);
    canvas.fillCircle(tx, ty, kTargetR, TFT_RED);
    canvas.fillCircle(tx, ty, kTargetR - 5, TFT_WHITE);
    canvas.fillCircle(tx, ty, kTargetR - 10, TFT_RED);
    canvas.fillCircle(bx, by, kBallR, TFT_CYAN);
    canvas.setFont(&fonts::efontJA_16);
    canvas.setTextColor(TFT_WHITE);
    canvas.setTextDatum(top_left);
    canvas.drawString("のこり:" + String((kTimeLimitMs - (millis() - start)) / 1000) +
                          "  スコア:" + String(score),
                      4, 4);
    canvas.pushSprite(0, 0);
    delay(16);  // 約60fps
  }
  canvas.deleteSprite();

  // 結果表示
  lcd.fillScreen(TFT_BLACK);
  lcd.setFont(&fonts::efontJA_24);
  lcd.setTextDatum(middle_center);
  lcd.drawString("スコア: " + String(score), W / 2, H / 2);
  delay(2000);

  return score > 10 ? 30 : score * 3;  // ごきげん上昇量に変換
}

}  // namespace MiniGame
