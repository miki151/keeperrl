#ifndef _EFFECT_H
#define _EFFECT_H

#include "util.h"
#include "enums.h"

class Level;
class Creature;
class Item;

class Effect {
  public:
  static void applyToCreature(Creature*, EffectType, EffectStrength);
  static void applyToPosition(Level*, Vec2, EffectType, EffectStrength);

  template <class Archive>
  static void registerTypes(Archive& ar);
};

#endif
