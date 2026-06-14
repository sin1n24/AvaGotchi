# AvaGotchi（あばごっち）

M5Stack **Basic / Gray** で動く、たまごっち風ペット育成ゲームです。キャラクターは [M5Stack-Avatar](https://github.com/meganetaaan/m5stack-avatar) で描画され、状況に合わせて表情が変わります。1つのファームが両機種に対応し、IMU の有無（Gray=有 / Basic無印=無）を起動時に自動判定して機能を切り替えます。

## 🚀 ブラウザから書き込む（かんたん）

開発環境は不要です。**PC の Chrome / Edge** で下記ページを開き、M5Stack Gray を USB 接続してボタンを押すだけ。

### 👉 https://sin1.studio/AvaGotchi/

内部では [esp-web-tools](https://esphome.github.io/esp-web-tools/)（esptool-js ベース）を使い、Web Serial 経由で書き込みます。**必ず `https://` で開いてください**（Web Serial は HTTPS 必須）。スマホ・Safari・Firefox は Web Serial 非対応のため不可です。

## 機能

- **表情変化**: 満腹度・ごきげん・元気に応じて Happy / Sad / Sleepy / Doubt / Angry に変化
- **時間帯あいさつ・時報 (RTC/NTP)**: 起動時に時間帯別あいさつ。毎正時に時報。時刻は天気取得時に NTP で同期
- **個体差**: チップ固有IDから体色・性格（くいしんぼう／あまえんぼう／げんきっこ／おっとり）・好物が決まり、性格でステータスの減りや喜び方が変わる
- **すれちがいバトル＆相性 (ESP-NOW)**: 近くの AvaGotchi と出会うと総合力で対戦＋体色の相性診断。勝つとごきげんUP・勝利数カウント（同じ相手は5分間ノーカウント）
- **レベル＆成長**: まんぷく・ごきげん・げんきが高いと経験値が貯まりレベルアップ。最初は簡単に、だんだん上がりにくくなる。寝ている間は成長しない。レベルで遊べるゲームや機能（すれちがい・じまんカード）がやさしい順に解放される
- **じまんカード (QR共有)**: Aボタン長押しで成績をQR表示（Lv4で解放）。スマホで読むと Pages のプロフィールカードが開き、Xでシェアできる
- **実績バッジ**: たまご数・友達数・メダル数などで称号を解除（みならい→…→ゲームマスター 等）
- **天気連動 (WiFi)**: IPアドレスから現在地を推定し Open-Meteo で天気を取得。天気に合わせて背景色が変わり、ペットが天気の感想をつぶやく
- **時間帯あいさつ・時報 (RTC/NTP)**: 起動時に時間帯別あいさつ。毎正時に時報。時刻は天気取得時に NTP で同期
- **IMUインタラクション (IMU搭載機のみ)**: 傾けると顔も傾く／くるくる回すと目が回る／つつくとくすぐったがる／逆さにすると画面を反転してびっくり
- **ミニゲーム（Bボタンのみ・会話主体）**: かずあて／れんだ／5びょうあて／はんしゃ／がまんくらべ の5種＋IMU搭載機は「かたむけ」。難易度（かんたん／ふつう／むずい）が選択画面に表示され、結果は金・銀・銅メダルで評価。難易度×メダルぶんの経験値がもらえる
- **天気限定ゲーム**: 天気を取得していると、その日の天気に合った限定ゲームがメニュー先頭に追加（はれ→ひなたぼっこ／あめ→あまやどり／ゆき→ゆきかき 等）
- **永続化**: ステータス・レベル・実績はNVSに保存され、電源を切っても消えない

## 操作

| 操作 | 動作 |
|---|---|
| ボタンA（短押し） | ごはんをあげる（寝ていたら起こす／満腹で連打すると不満に） |
| ボタンA（長押し） | じまんカード（QR）を表示 |
| ボタンB | ミニゲーム選択（メニュー内では短押しで送り・長押しで決定） |
| ボタンC（短押し） | ステータス画面（もう一度Cで即閉じる） |
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
- `src/minigame.h/.cpp` — Bボタンのみの会話型ミニゲーム＋傾けゲーム
- `docs/` — GitHub Pages（ブラウザ書き込みページ・配布用 bin）

## ビルド対象

`platformio.ini` の env は `m5stack`（`board = m5stack-core-esp32`）。Basic(4MB)とGray(16MB)の両方で起動できるよう、パーティションは `huge_app.csv`（4MB・OTA無し・app約3MB）を使用しています。

## 配布 bin の更新手順

ファームを変更したら、`docs/firmware/avagotchi-merged.bin` を作り直して push します（flash_size は両機種で起動できるよう **4MB**）。

```
pio run
python <esptoolpy>/esptool.py --chip esp32 merge_bin \
  -o docs/firmware/avagotchi-merged.bin \
  --flash_mode dio --flash_freq 40m --flash_size 4MB \
  0x1000  .pio/build/m5stack/bootloader.bin \
  0x8000  .pio/build/m5stack/partitions.bin \
  0xe000  <framework>/tools/partitions/boot_app0.bin \
  0x10000 .pio/build/m5stack/firmware.bin
```

## 補足

- ESP-NOWとWiFi(STA)は同時利用しないよう、天気取得時は一旦ESP-NOWを停止→取得後に再開しています
- 天気APIはキー不要の Open-Meteo を使用（HTTP）。時刻は NTP（ntp.nict.jp 他）で JST に同期
- M5Stack Gray は物理RTC非搭載のため、時間帯あいさつ・時報は「天気取得で時刻同期した後」のセッションで有効（電源OFFでリセット）

## スタックチャンについて / クレジット

AvaGotchi のキャラクター表示には、ししかわ氏（[@meganetaaan](https://github.com/meganetaaan)）が作成した **[M5Stack-Avatar](https://github.com/meganetaaan/m5stack-avatar)** を使用しています。これは、M5Stack をベースにした自作コミュニケーションロボット **スタックチャン（ｽﾀｯｸﾁｬﾝ / stack-chan）** の「顔」を描画するためのライブラリです。

**スタックチャン** は、ししかわ氏とコミュニティによって開発されている、M5Stack ベースのオープンソースな自作ロボットです。手のひらサイズのかわいい見た目と、ゆたかな表情・しぐさが特徴で、世界中に多くのファンと派生作品・コミュニティを生んでいます。AvaGotchi の「表情で気持ちを伝える」体験は、このスタックチャン文化に大きく影響を受けています。

- 本家リポジトリ: <https://github.com/meganetaaan/stack-chan>
- 作者: ししかわ（Shunya Ishikawa）
- ライセンス: Apache License 2.0
- 公式ハッシュタグ: `#stackchan` `#ｽﾀｯｸﾁｬﾝ`

> **注記**: AvaGotchi は M5Stack-Avatar を利用した独自プロジェクトであり、公式リポジトリ管理下のデータのみで構成された「スタックチャン」そのものではありません。[スタックチャン二次創作ガイドライン](https://github.com/meganetaaan/stack-chan/blob/main/GUIDELINE_ja.md)では『スタックチャン』を「本リポジトリ管理下のデータのみを使って作られたロボット」と定義しているため、本プロジェクトはスタックチャンを名乗らず、リスペクトを込めて**関連プロジェクト**として紹介・クレジットします。

スタックチャン本家やコミュニティの作品にも、ぜひ触れてみてください 🤖
