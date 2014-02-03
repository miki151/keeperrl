#ifndef _STATISTICS_H
#define _STATISTICS_H

#include "util.h"
#include "enums.h"

enum class StatId {
  DEATH,
  CHOPPED_HEAD,
  CHOPPED_LIMB,
  INNOCENT_KILLED,
  SPELL_CAST,
  SCROLL_READ,
  ARMOR_PRODUCED,
  WEAPON_PRODUCED,
  POTION_PRODUCED,
};

ENUM_HASH(StatId);

class Statistics {
  public:
  static void init();
  static void add(StatId);
  static vector<string> getText();

  private:
  static unordered_map<StatId, int> count;
};

#endif
