# AvaGotchi（あばごっち）

M5Stack Gray で動く、たまごっち風ペット育成ゲームです。キャラクターは [M5Stack-Avatar](https://github.com/meganetaaan/m5stack-avatar) で描画され、状況に合わせて表情が変わります。

## 機能

- **表情変化**: 満腹度・ごきげん・元気に応じて Happy / Sad / Sleepy / Doubt / Angry に変化
- **すれちがい通信 (ESP-NOW)**: 近くの AvaGotchi と自動ですれちがい。出会うとごきげんUP・友達カウント（同じ相手は5分間ノーカウント）
- **天気連動 (WiFi)**: IPアドレスから現在地を推定し Open-Meteo で天気を取得。天気に合わせて背景色が変わり、ペットが天気の感想をつぶやく
- **IMUインタラクション**: 本体を傾けると顔も一緒に傾く。強く振ると目を回して怒る（寝ているときは起きる）
- **ミニゲーム**: 本体を傾けてボールを転がし、20秒間で的に当てた数を競う。スコアでごきげんUP
- **たまご**: ごきげん100&満腹80でたまごを産み、新しいペットが生まれる（友達・たまご数は引き継ぎ）
- **永続化**: ステータスはNVSに保存され、電源を切っても消えない

## 操作

| 操作 | 動作 |
|---|---|
| ボタンA | ごはんをあげる（寝ていたら起こす） |
| ボタンB | ミニゲーム開始 |
| ボタンC（短押し） | ステータス画面 |
| ボタンC（長押し） | 天気を取得・更新 |
| 強く振る | 目を回す / 寝ていたら起きる |

## セットアップ

1. `src/secrets.example.h` を `src/secrets.h` にコピーし、`WIFI_SSID` / `WIFI_PASS` を自宅のWiFiに書き換える（`secrets.h` はコミットされません）
2. 必要なら `PET_NAME`（すれちがいで相手に伝わる名前）と `WEATHER_LAT/LON`（固定座標にする場合）を変更
3. ビルド・書き込み:

```
pio run --target upload
pio device monitor
```

## 構成

- `src/main.cpp` — 全体統合・表情制御・ボタン/IMU処理
- `src/pet.h/.cpp` — ペットのステータスとNVS永続化
- `src/streetpass.h/.cpp` — ESP-NOWすれちがい通信
- `src/weather.h/.cpp` — WiFi接続・IP位置推定・Open-Meteo天気取得
- `src/minigame.h/.cpp` — IMUボール転がしゲーム

## 補足

- ESP-NOWとWiFi(STA)は同時利用しないよう、天気取得時は一旦ESP-NOWを停止→取得後に再開しています
- 天気APIはキー不要の Open-Meteo を使用（HTTP）
