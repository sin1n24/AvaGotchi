#include "streetpass.h"
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include "config.h"

namespace {

struct __attribute__((packed)) Packet {
  uint32_t magic;
  char name[16];
  uint16_t level;
  uint16_t power;        // 総合力（バトル比較用）
  uint8_t  personality;  // 性格
  uint16_t hue;          // 体色(色相)
};

const uint8_t kBroadcastMac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
Packet myPacket;
uint32_t lastSendMs = 0;

// 直近にすれちがった相手（同じ相手と連続カウントしないため）
struct Seen {
  uint8_t mac[6];
  uint32_t atMs;
};
Seen seenList[8];
volatile bool hasNewFriend = false;
FriendInfo newFriend;

bool recentlySeen(const uint8_t *mac) {
  const uint32_t now = millis();
  for (auto &s : seenList) {
    if (memcmp(s.mac, mac, 6) == 0 && now - s.atMs < 5 * 60 * 1000UL) {
      s.atMs = now;  // 見かけた時刻を更新
      return true;
    }
  }
  // 一番古い枠に記録
  Seen *oldest = &seenList[0];
  for (auto &s : seenList)
    if (s.atMs < oldest->atMs) oldest = &s;
  memcpy(oldest->mac, mac, 6);
  oldest->atMs = now;
  return false;
}

void onRecv(const uint8_t *mac, const uint8_t *data, int len) {
  if (len != sizeof(Packet)) return;
  Packet p;
  memcpy(&p, data, sizeof(p));
  if (p.magic != PET_MAGIC) return;
  if (recentlySeen(mac)) return;  // 5分以内の再会はノーカウント
  p.name[sizeof(p.name) - 1] = '\0';
  strncpy(newFriend.name, p.name, sizeof(newFriend.name));
  newFriend.level       = p.level;
  newFriend.power       = p.power;
  newFriend.personality = p.personality;
  newFriend.hue         = p.hue;
  hasNewFriend = true;
}

}  // namespace

namespace StreetPass {

void begin() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE);
  if (esp_now_init() != ESP_OK) return;
  esp_now_register_recv_cb(onRecv);
  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, kBroadcastMac, 6);
  peer.channel = ESPNOW_CHANNEL;
  peer.encrypt = false;
  esp_now_add_peer(&peer);

  myPacket.magic = PET_MAGIC;
  strncpy(myPacket.name, PET_NAME, sizeof(myPacket.name) - 1);
}

// 自分の情報（名前・総合力・性格・色）を更新

void end() {
  esp_now_deinit();
  WiFi.mode(WIFI_OFF);
}

void update() {
  if (millis() - lastSendMs < 5000) return;  // 5秒ごとに存在を知らせる
  lastSendMs = millis();
  esp_now_send(kBroadcastMac, (uint8_t *)&myPacket, sizeof(myPacket));
}

bool popNewFriend(FriendInfo &out) {
  if (!hasNewFriend) return false;
  out = newFriend;
  hasNewFriend = false;
  return true;
}

void setMyInfo(const char *name, uint16_t level, uint16_t power, uint8_t personality, uint16_t hue) {
  if (name && name[0]) {
    strncpy(myPacket.name, name, sizeof(myPacket.name) - 1);
    myPacket.name[sizeof(myPacket.name) - 1] = '\0';
  }
  myPacket.level       = level;
  myPacket.power       = power;
  myPacket.personality = personality;
  myPacket.hue         = hue;
}

}  // namespace StreetPass
