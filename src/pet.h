#pragma once
#include <Arduino.h>

// ペットの状態
struct PetState {
  int satiety   = 70;  // 満腹度 0-100
  int happiness = 70;  // ごきげん 0-100
  int energy    = 70;  // 元気 0-100
  uint16_t friends = 0;  // すれちがった友達の数
  uint8_t  level   = 1;  // レベル（1から。状態が良いと経験値が貯まって上がる）
  uint16_t exp     = 0;  // 現在レベル内の経験値
  uint32_t ageMin  = 0;  // 年齢(分)
  bool sleeping = false; // おやすみ中
  // --- 実績カウンタ（世代交代しても引き継ぐ累計値） ---
  uint16_t totalFeeds   = 0;  // 累計ごはん回数
  uint16_t goldMedals   = 0;  // 金メダル数
  uint16_t silverMedals = 0;  // 銀メダル数
  uint16_t bronzeMedals = 0;  // 銅メダル数
  uint16_t battleWins   = 0;  // すれちがいバトルの勝利数
};

// 性格（チップ固有IDから決まる機体ごとの個性）
enum class Personality { Glutton = 0, Spoiled, Energetic, Easygoing };

class Pet {
 public:
  PetState st;
  int personality = 0;  // Personality。機体固有なのでNVSではなくIDから毎回設定

  void load();                 // NVSから復元
  void save();                 // NVSへ保存
  int  decay();                // 1分ごとの減衰＋経験値加算。戻り値=この回で上がったレベル数
  void feed();                 // ごはん
  void play(int score);        // ミニゲーム結果を反映
  void meetFriend();           // すれちがい成立
  void shaken();               // 強く振られた（目が回る）
  void recordMedal(int up);    // ゲーム結果(0-30)を金銀銅で記録
  int  addExp(int amount);     // 経験値を加算しレベル判定。戻り値=上がったレベル数

  // 次のレベルに必要な経験値（レベルが上がるほど増える）
  int expForNext() const;
  // この子の総合力（すれちがいバトルの強さ）
  uint16_t power() const;

 private:
  static int clamp100(int v) { return v < 0 ? 0 : (v > 100 ? 100 : v); }
};
