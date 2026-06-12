#include "weather.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "config.h"

namespace {

// Open-MeteoのWMO天気コードを大分類へ変換
WeatherKind codeToKind(int code) {
  if (code == 0 || code == 1) return WeatherKind::Clear;
  if (code <= 3) return WeatherKind::Cloudy;
  if (code == 45 || code == 48) return WeatherKind::Fog;
  if ((code >= 51 && code <= 67) || (code >= 80 && code <= 82))
    return WeatherKind::Rain;
  if ((code >= 71 && code <= 77) || code == 85 || code == 86)
    return WeatherKind::Snow;
  if (code >= 95) return WeatherKind::Thunder;
  return WeatherKind::Cloudy;
}

// IPアドレスから現在地の緯度経度を推定
bool geolocate(float &lat, float &lon) {
  HTTPClient http;
  http.begin("http://ip-api.com/json/?fields=status,lat,lon");
  http.setTimeout(8000);
  if (http.GET() != 200) { http.end(); return false; }
  JsonDocument doc;
  bool ok = !deserializeJson(doc, http.getString()) &&
            strcmp(doc["status"] | "", "success") == 0;
  if (ok) {
    lat = doc["lat"].as<float>();
    lon = doc["lon"].as<float>();
  }
  http.end();
  return ok;
}

}  // namespace

namespace Weather {

bool fetch(WeatherInfo &out) {
  out.valid = false;
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - t0 > 15000) {  // 15秒で接続を諦める
      WiFi.mode(WIFI_OFF);
      return false;
    }
    delay(200);
  }

  float lat = WEATHER_LAT, lon = WEATHER_LON;
  if (lat == 0.0f && lon == 0.0f) {
    if (!geolocate(lat, lon)) {
      WiFi.disconnect(true);
      WiFi.mode(WIFI_OFF);
      return false;
    }
  }

  char url[192];
  snprintf(url, sizeof(url),
           "http://api.open-meteo.com/v1/forecast"
           "?latitude=%.4f&longitude=%.4f&current_weather=true",
           lat, lon);
  HTTPClient http;
  http.begin(url);
  http.setTimeout(8000);
  if (http.GET() == 200) {
    JsonDocument doc;
    if (!deserializeJson(doc, http.getString())) {
      out.kind = codeToKind(doc["current_weather"]["weathercode"].as<int>());
      out.temperature = doc["current_weather"]["temperature"].as<float>();
      out.valid = true;
    }
  }
  http.end();
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  return out.valid;
}

const char *kindLabel(WeatherKind k) {
  switch (k) {
    case WeatherKind::Clear:   return "はれ";
    case WeatherKind::Cloudy:  return "くもり";
    case WeatherKind::Fog:     return "きり";
    case WeatherKind::Rain:    return "あめ";
    case WeatherKind::Snow:    return "ゆき";
    case WeatherKind::Thunder: return "かみなり";
    default:                   return "?";
  }
}

}  // namespace Weather
