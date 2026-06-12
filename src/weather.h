#pragma once
#include <Arduino.h>

enum class WeatherKind { Unknown, Clear, Cloudy, Fog, Rain, Snow, Thunder };

struct WeatherInfo {
  WeatherKind kind = WeatherKind::Unknown;
  float temperature = 0;
  bool valid = false;
};

namespace Weather {
// WiFiに接続して天気を取得する（ブロッキング・呼ぶ前にStreetPass::end()すること）
// 成功でtrue。終了時WiFiはOFFに戻す。
bool fetch(WeatherInfo &out);
const char *kindLabel(WeatherKind k);  // 「はれ」など
}  // namespace Weather
