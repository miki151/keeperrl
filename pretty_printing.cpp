#include "stdafx.h"
#include "pretty_printing.h"
#include "pretty_archive.h"
#include "creature_factory.h"
#include "furniture_type.h"
#include "attack_type.h"
#include "effect.h"
#include "lasting_effect.h"
#include "attr_type.h"
#include "body.h"
#include "item_type.h"
#include "technology.h"
#include "trap_type.h"
#include "campaign.h"
#include "enemy_factory.h"

template <typename T>
optional<T> PrettyPrinting::parseObject(const string& s) {
  try {
    PrettyInput input(s);
    T object;
    input.getArchive() >> object;
    return object;
  } catch (...) {}
  return none;
}

template <typename T>
vector<T> PrettyPrinting::parseObjects(int count, const string& s) {
  vector<T> ret;
  try {
    PrettyInput input(s);
    for (int i : Range(count)) {
      T object;
      input.getArchive() >> object;
      ret.push_back(object);
    }
  } catch (PrettyException ex) {
    USER_FATAL << ex.text;
  }
  return ret;
}

template
optional<Effect> PrettyPrinting::parseObject<Effect>(const string&);

template
optional<ItemType> PrettyPrinting::parseObject<ItemType>(const string&);

template
optional<CreatureId> PrettyPrinting::parseObject<CreatureId>(const string&);

template
vector<vector<Campaign::VillainInfo>> PrettyPrinting::parseObjects<vector<Campaign::VillainInfo>>(int, const string&);
