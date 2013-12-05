#ifndef _CREATURE_ATTRIBUTES_H
#define _CREATURE_ATTRIBUTES_H

#include <string>
#include <functional>

#include "util.h"
#include "skill.h"

// WTF is this defined
#undef HUGE

enum class CreatureSize { SMALL, MEDIUM, LARGE, HUGE };

#define CATTR(X) CreatureAttributes([&](CreatureAttributes& c) { X })

class CreatureAttributes {
  public:
  CreatureAttributes(function<void(CreatureAttributes&)> fun) {
    fun(*this);
  }

  MustInitialize<string> name;
  MustInitialize<int> speed;
  MustInitialize<CreatureSize> size;
  MustInitialize<int> strength;
  MustInitialize<int> dexterity;
  MustInitialize<int> weight;
  Optional<string> chatReactionFriendly;
  Optional<string> chatReactionHostile;
  Optional<string> firstName;
  bool specialMonster = false;
  int barehandedDamage = 0;
  int legs = 2;
  int arms = 2;
  int wings = 0;
  int heads = 1;
  bool noBody = false;
  bool fireResistant = false;
  bool breathing = true;
  MustInitialize<bool> humanoid;
  bool animal = false;
  bool healer = false;
  bool flyer = false;
  bool undead = false;
  bool walker = true;
  bool isFood = false;
  bool stationary = false;
  bool noSleep = false;
  double courage = 1;
  int maxLevel = 10;
  double carryingMultiplier = 1;
  bool permanentlyBlind = false;
  bool invincible = false;
  double damageMultiplier = 1;
  unordered_set<Skill*> skills;
  map<int, Skill*> skillGain {{4, Skill::twoHandedWeapon}, {6, Skill::knifeThrowing}, {10, Skill::archery}};
};

#endif
