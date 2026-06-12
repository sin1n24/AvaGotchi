#pragma once
#include <Arduino.h>

// すれちがいで受け取る相手の情報
struct FriendInfo {
  char name[16];
  uint16_t eggs;
};

namespace StreetPass {
void begin();                       // ESP-NOW開始（WiFi STA + ブロードキャスト）
void end();                         // 停止（天気取得でWiFiを使う前に呼ぶ）
void update();                      // 定期ブロードキャスト（loopから呼ぶ）
bool popNewFriend(FriendInfo &out); // 新しいすれちがいがあれば取り出す
void setMyInfo(uint16_t eggs);      // 自分の情報を更新
}  // namespace StreetPass
