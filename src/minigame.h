#pragma once

namespace MiniGame {
// 傾けてボールを転がし、的に当てるゲーム。
// 終了までブロッキングし、スコア(0-30)を返す。Avatarはsuspendしてから呼ぶこと。
int playBallGame();
}  // namespace MiniGame
