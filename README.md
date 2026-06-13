# AvaGotchi（あばごっち）

M5Stack Gray で動く、たまごっち風ペット育成ゲームです。キャラクターは [M5Stack-Avatar](https://github.com/meganetaaan/m5stack-avatar) で描画され、状況に合わせて表情が変わります。

## 🚀 ブラウザから書き込む（かんたん）

開発環境は不要です。**PC の Chrome / Edge** で下記ページを開き、M5Stack Gray を USB 接続してボタンを押すだけ。

### 👉 https://sin1.studio/AvaGotchi/

内部では [esp-web-tools](https://esphome.github.io/esp-web-tools/)（esptool-js ベース）を使い、Web Serial 経由で書き込みます。**必ず `https://` で開いてください**（Web Serial は HTTPS 必須）。スマホ・Safari・Firefox は Web Serial 非対応のため不可です。

## 機能

- **表情変化**: 満腹度・ごきげん・元気に応じて Happy / Sad / Sleepy / Doubt / Angry に変化
- **時間帯あいさつ・時報 (RTC/NTP)**: 起動時に時間帯別あいさつ。毎正時に時報。時刻は天気取得時に NTP で同期
- **すれちがい通信 (ESP-NOW)**: 近くの AvaGotchi と自動ですれちがい。出会うとごきげんUP・友達カウント（同じ相手は5分間ノーカウント）
- **天気連動 (WiFi)**: IPアドレスから現在地を推定し Open-Meteo で天気を取得。天気に合わせて背景色が変わり、ペットが天気の感想をつぶやく
- **IMUインタラクション**: 傾けると顔も傾く／くるくる回すと目が回る／つつくとくすぐったがる／逆さにするとびっくり
- **ミニゲーム（5種・Bボタンのみ）**: 5びょうあて／反射神経／連打／ラッキー7／がまんくらべ。Avatar との会話で進行
- **たまご**: ごきげん100&満腹80でたまごを産み、新しいペットが生まれる（友達・たまご数は引き継ぎ）
- **永続化**: ステータスはNVSに保存され、電源を切っても消えない

## 操作

| 操作 | 動作 |
|---|---|
| ボタンA | ごはんをあげる（寝ていたら起こす／満腹で連打すると不満に） |
| ボタンB | ミニゲーム選択（短押しで送り・長押しで決定） |
| ボタンC（短押し） | ステータス画面 |
| ボタンC（長押し） | 天気を取得・更新＋時刻合わせ |
| くるくる回す | 目を回す / 寝ていたら起きる |
| つつく | くすぐったがる（ごきげん少しUP） |
| 逆さにする | びっくりしてコメント |

## セットアップ（自分でビルドする場合）

1. `src/secrets.example.h` を `src/secrets.h` にコピーし、`WIFI_SSID` / `WIFI_PASS` を自宅のWiFiに書き換える（`secrets.h` はコミットされません）
2. 必要なら `PET_NAME`（すれちがいで相手に伝わる名前）と `WEATHER_LAT/LON`（固定座標にする場合）を変更
3. ビルド・書き込み:

```
pio run --target upload
pio device monitor
```

## 構成

- `src/main.cpp` — 全体統合・表情制御・ボタン/IMU処理・RTC/時報
- `src/pet.h/.cpp` — ペットのステータスとNVS永続化
- `src/streetpass.h/.cpp` — ESP-NOWすれちがい通信
- `src/weather.h/.cpp` — WiFi接続・IP位置推定・Open-Meteo天気取得・NTP時刻同期
- `src/minigame.h/.cpp` — Bボタンのみで遊ぶ会話型ミニゲーム5種
- `docs/` — GitHub Pages（ブラウザ書き込みページ・配布用 bin）

## 配布 bin の更新手順

ファームを変更したら、`docs/firmware/avagotchi-merged.bin` を作り直して push します。

```
pio run
python <esptoolpy>/esptool.py --chip esp32 merge_bin \
  -o docs/firmware/avagotchi-merged.bin \
  --flash_mode dio --flash_freq 40m --flash_size 16MB \
  0x1000  .pio/build/m5stack-grey/bootloader.bin \
  0x8000  .pio/build/m5stack-grey/partitions.bin \
  0xe000  <framework>/tools/partitions/boot_app0.bin \
  0x10000 .pio/build/m5stack-grey/firmware.bin
```

## 補足

- ESP-NOWとWiFi(STA)は同時利用しないよう、天気取得時は一旦ESP-NOWを停止→取得後に再開しています
- 天気APIはキー不要の Open-Meteo を使用（HTTP）。時刻は NTP（ntp.nict.jp 他）で JST に同期
- M5Stack Gray は物理RTC非搭載のため、時間帯あいさつ・時報は「天気取得で時刻同期した後」のセッションで有効（電源OFFでリセット）
