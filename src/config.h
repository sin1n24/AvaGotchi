#pragma once

// ===== WiFi設定（天気取得用） =====
// secrets.example.h を secrets.h にコピーして自宅のWiFi情報を書き込んでください
#include "secrets.h"

// ===== 天気取得の位置 =====
// 0.0 のままなら IPアドレスから地域を自動判定します
#define WEATHER_LAT 0.0
#define WEATHER_LON 0.0

// ===== すれちがい通信（ESP-NOW） =====
#define ESPNOW_CHANNEL 1
#define PET_MAGIC 0x41564754UL  // "AVGT"
#define PET_NAME "ぐれーちゃん"  // すれちがい時に相手へ伝える名前（半角12byteまで）

// ===== ゲームバランス =====
#define DECAY_INTERVAL_MS (60 * 1000UL)  // ステータス減衰の間隔
#define SATIETY_DECAY 2                   // 満腹度の減り(毎分)
#define HAPPINESS_DECAY 1                 // ごきげんの減り(毎分)
#define ENERGY_DECAY 1                    // 元気の減り(毎分)
