#pragma once

namespace MiniGame {
// Bボタンだけで遊ぶ、Avatarとの会話主体のミニゲーム集。
// ゲーム選択画面を出して1つ遊び、ごきげん上昇量(0-30)を返す。
// 放置などでキャンセルした場合は -1。Avatarはmain側のグローバルを使う。
int selectAndPlay();
}  // namespace MiniGame
