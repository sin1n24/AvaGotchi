#pragma once
#include <Arduino.h>

// ペットの状態
struct PetState {
  int satiety   = 70;  // 満腹度 0-100
  int happiness = 70;  // ごきげん 0-100
  int energy    = 70;  // 元気 0-100
  uint16_t friends = 0;  // すれちがった友達の数
  uint16_t eggs    = 0;  // 産んだたまごの数
  uint32_t ageMin  = 0;  // 年齢(分)
  bool sleeping = false; // おやすみ中
};

class Pet {
 public:
  PetState st;

  void load();                 // NVSから復元
  void save();                 // NVSへ保存
  void decay();                // 1分ごとの減衰
  void feed();                 // ごはん
  void play(int score);        // ミニゲーム結果を反映
  void meetFriend();           // すれちがい成立
  void shaken();               // 強く振られた（目が回る）
  bool checkEgg();             // たまご条件を満たしたか
  void layEgg();               // たまごを産んでリセット

 private:
  static int clamp100(int v) { return v < 0 ? 0 : (v > 100 ? 100 : v); }
};
