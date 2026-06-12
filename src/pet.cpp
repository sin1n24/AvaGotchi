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
  st.eggs      = prefs.getUShort("egg", 0);
  st.ageMin    = prefs.getULong("age", 0);
  prefs.end();
}

void Pet::save() {
  prefs.begin("avagotchi", false);
  prefs.putInt("sat", st.satiety);
  prefs.putInt("hap", st.happiness);
  prefs.putInt("ene", st.energy);
  prefs.putUShort("frd", st.friends);
  prefs.putUShort("egg", st.eggs);
  prefs.putULong("age", st.ageMin);
  prefs.end();
}

void Pet::decay() {
  st.ageMin++;
  st.satiety   = clamp100(st.satiety - SATIETY_DECAY);
  st.happiness = clamp100(st.happiness - HAPPINESS_DECAY);
  // 寝ている間は元気が回復する
  if (st.sleeping) {
    st.energy = clamp100(st.energy + 10);
    if (st.energy >= 100) st.sleeping = false;  // 全快で起きる
  } else {
    st.energy = clamp100(st.energy - ENERGY_DECAY);
    if (st.energy <= 10) st.sleeping = true;    // 疲れたら寝る
  }
  // おなかが空きすぎるとごきげんも下がる
  if (st.satiety < 20) st.happiness = clamp100(st.happiness - 3);
}

void Pet::feed() {
  st.satiety   = clamp100(st.satiety + 30);
  st.happiness = clamp100(st.happiness + 5);
  save();
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

bool Pet::checkEgg() {
  return st.happiness >= 100 && st.satiety >= 80;
}

void Pet::layEgg() {
  st.eggs++;
  uint16_t keepFriends = st.friends;
  uint16_t keepEggs    = st.eggs;
  st = PetState();           // ステータスを初期化
  st.friends = keepFriends;  // 友達とたまごの実績は引き継ぐ
  st.eggs    = keepEggs;
  save();
}
