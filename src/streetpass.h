#pragma once
#include <Arduino.h>

// すれちがいで受け取る相手の情報
struct FriendInfo {
  char name[16];
  uint16_t eggs;
  uint16_t power;        // 相手の総合力（バトル比較用）
  uint8_t  personality;  // 相手の性格
  uint16_t hue;          // 相手の体色(色相0-359)。相性診断に使用
};

namespace StreetPass {
void begin();                       // ESP-NOW開始（WiFi STA + ブロードキャスト）
void end();                         // 停止（天気取得でWiFiを使う前に呼ぶ）
void update();                      // 定期ブロードキャスト（loopから呼ぶ）
bool popNewFriend(FriendInfo &out); // 新しいすれちがいがあれば取り出す
// 自分の情報を更新（バトル・相性に使う）
void setMyInfo(const char *name, uint16_t eggs, uint16_t power, uint8_t personality, uint16_t hue);
}  // namespace StreetPass
