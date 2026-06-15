#include "pet.h"
#include <Preferences.h>
#include "config.h"

static Preferences prefs;

void Pet::load() {
  prefs.begin("avagotchi", true);
  st.satiety   = prefs.getInt("sat", 70);
  st.happiness = prefs.getInt("hap", 70);
  st.energy    = prefs.getInt("ene", 70);
  st.friends   = prefs.getUShort("frd", 0);
  st.level     = prefs.getUChar("lvl", 1);
  st.exp       = prefs.getUShort("exp", 0);
  st.ageMin    = prefs.getULong("age", 0);
  st.tutorialDone = prefs.getBool("tut", false);
  st.totalFeeds   = prefs.getUShort("fed", 0);
  st.goldMedals   = prefs.getUShort("gld", 0);
  st.silverMedals = prefs.getUShort("slv", 0);
  st.bronzeMedals = prefs.getUShort("brz", 0);
  st.battleWins   = prefs.getUShort("win", 0);
  prefs.end();
}

void Pet::save() {
  prefs.begin("avagotchi", false);
  prefs.putInt("sat", st.satiety);
  prefs.putInt("hap", st.happiness);
  prefs.putInt("ene", st.energy);
  prefs.putUShort("frd", st.friends);
  prefs.putUChar("lvl", st.level);
  prefs.putUShort("exp", st.exp);
  prefs.putULong("age", st.ageMin);
  prefs.putBool("tut", st.tutorialDone);
  prefs.putUShort("fed", st.totalFeeds);
  prefs.putUShort("gld", st.goldMedals);
  prefs.putUShort("slv", st.silverMedals);
  prefs.putUShort("brz", st.bronzeMedals);
  prefs.putUShort("win", st.battleWins);
  prefs.end();
}

int Pet::decay() {
  st.ageMin++;
  // 性格で減りやすさが変わる（個体差）
  int sd = SATIETY_DECAY, hd = HAPPINESS_DECAY, ed = ENERGY_DECAY;
  switch ((Personality)personality) {
    case Personality::Glutton:   sd = sd * 3 / 2; break;             // 食いしん坊：おなかが空きやすい
    case Personality::Spoiled:   hd = hd * 2;     break;             // 甘えん坊：さみしがり
    case Personality::Energetic: ed = (ed + 1) / 2; break;           // 元気っ子：疲れにくい
    case Personality::Easygoing: sd = (sd + 1) / 2; hd = (hd + 1) / 2; ed = (ed + 1) / 2; break;  // おっとり
  }
  st.satiety   = clamp100(st.satiety - sd);
  st.happiness = clamp100(st.happiness - hd);
  // 寝ている間は元気が回復する
  if (st.sleeping) {
    st.energy = clamp100(st.energy + 10);
    if (st.energy >= 100) st.sleeping = false;  // 全快で起きる
  } else {
    st.energy = clamp100(st.energy - ed);
    if (st.energy <= 10) st.sleeping = true;    // 疲れたら寝る
  }
  // おなかが空きすぎるとごきげんも下がる
  if (st.satiety < 20) st.happiness = clamp100(st.happiness - 3);

  // --- 経験値：満腹・ごきげん・元気が高いほど貯まる（快適に過ごすと成長） ---
  // 寝ている間は成長しない（夢の中ではレベルアップしない）
  int gain = 0;
  if (!st.sleeping) {
    if (st.satiety   >= 70) gain++;
    if (st.happiness >= 70) gain++;
    if (st.energy    >= 70) gain++;
    if (gain >= 3) gain++;  // すべて好調ならボーナス（最大4/分）
  }
  return addExp(gain);
}

// 経験値を加算し、必要量に達したらレベルアップ。上がったレベル数を返す。
int Pet::addExp(int amount) {
  if (amount <= 0) return 0;
  int ups = 0;
  st.exp += amount;
  while (st.level < 99 && st.exp >= expForNext()) {
    st.exp -= expForNext();
    st.level++;
    ups++;
  }
  return ups;
}

// 次のレベルに必要な経験値。最初は小さく、レベルが上がるほど急に増える。
//   Lv1→2:10  2→3:25  3→4:50  4→5:85  5→6:130 …（5 + level^2 * 5）
int Pet::expForNext() const {
  return 5 + (int)st.level * (int)st.level * 5;
}

void Pet::feed() {
  st.satiety   = clamp100(st.satiety + 30);
  // 食いしん坊はごはんで余計に喜ぶ
  st.happiness = clamp100(st.happiness + ((Personality)personality == Personality::Glutton ? 10 : 5));
  st.totalFeeds++;
  save();
}

void Pet::recordMedal(int up) {
  if (up >= 25)      st.goldMedals++;
  else if (up >= 15) st.silverMedals++;
  else               st.bronzeMedals++;
  save();
}

// 総合力＝ステータス＋育成日数＋実績の合算（すれちがいバトルで使用）
uint16_t Pet::power() const {
  uint32_t p = st.satiety + st.happiness + st.energy;
  p += st.level * 15 + st.friends * 5;
  p += st.goldMedals * 10 + st.silverMedals * 5 + st.bronzeMedals * 2;
  p += st.ageMin / 1440 * 10;  // 育成日数×10
  return (uint16_t)(p > 65535 ? 65535 : p);
}

void Pet::play(int score) {
  st.happiness = clamp100(st.happiness + score);
  st.energy    = clamp100(st.energy - 10);
  st.satiety   = clamp100(st.satiety - 5);
  save();
}

void Pet::meetFriend() {
  st.friends++;
  st.happiness = clamp100(st.happiness + 10);
  save();
}

void Pet::shaken() {
  st.happiness = clamp100(st.happiness - 5);
}
