#pragma once

#include "util.h"
#include "effect_type.h"
#include "tribe.h"

class Position;
class Creature;
class Furniture;

class FurnitureEntry {
  public:
  struct Sokoban {};

  struct Trap {
    Trap(EffectType e, bool vis = false) : effect(e), alwaysVisible(vis) {}
    SERIALIZATION_CONSTRUCTOR(Trap)
    EffectType SERIAL(effect);
    bool SERIAL(alwaysVisible);
    SERIALIZE_ALL(effect, alwaysVisible)
  };

  struct Water {};
  struct Magma {};

  using EntryData = variant<Sokoban, Trap, Water, Magma>;
  template <typename T>
  FurnitureEntry(const T& t) : FurnitureEntry(EntryData(t)) {}
  FurnitureEntry(EntryData);
  void handle(WFurniture, WCreature);
  bool isVisibleTo(WConstFurniture, WConstCreature) const;

  SERIALIZATION_DECL(FurnitureEntry)

  private:
  EntryData SERIAL(entryData);
};
