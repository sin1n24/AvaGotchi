#pragma once

namespace MiniGame {
// Bボタンだけで遊ぶ、Avatarとの会話主体のミニゲーム集。
// 選択画面を出して1つ遊び、ごきげん上昇量(0-30)を返す。放置キャンセルは -1。
// imuAvailable=true(Gray等) のときは傾け操作の「エサとり」ゲームも選択肢に加わる。
// weatherGameIdx>=0 のとき、その天気限定ゲーム(weatherGameName)をメニュー先頭に追加。
// Avatarはmain側のグローバルを使う。
int selectAndPlay(bool imuAvailable, int weatherGameIdx, const char *weatherGameName);
}  // namespace MiniGame
