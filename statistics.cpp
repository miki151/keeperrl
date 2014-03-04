#include "stdafx.h"
#include "statistics.h"

unordered_map<StatId, int> Statistics::count;

template <class Archive>
void Statistics::serialize(Archive& ar, const unsigned int version) {
  ar & BOOST_SERIALIZATION_NVP(count);
}

SERIALIZABLE(Statistics);

void Statistics::init() {
  count.clear();
}

void Statistics::add(StatId id) {
  ++count[id];
}

vector<pair<StatId, string>> text {
  {StatId::DEATH, "deaths"}, 
  {StatId::INNOCENT_KILLED, "cold blooded murders"}, 
  {StatId::CHOPPED_HEAD, "chopped heads" },
  {StatId::CHOPPED_LIMB, "chopped limbs" },
  {StatId::SPELL_CAST, "spells cast" },
  {StatId::SCROLL_READ, "scrolls read" },
  {StatId::WEAPON_PRODUCED, "weapons produced" },
  {StatId::ARMOR_PRODUCED, "pieces of armor produced" },
  {StatId::POTION_PRODUCED, "potions produced" },

};

vector<string> Statistics::getText() {
  vector<string> ret;
  for (auto elem : text) {
    if (int n = count[elem.first])
      ret.emplace_back(convertToString(n) + " " + elem.second);
  }
  return ret;
}

