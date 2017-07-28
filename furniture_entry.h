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
    Trap(EffectType e, bool s = false) : effect(e), spiderWeb(s) {}
    SERIALIZATION_CONSTRUCTOR(Trap)
    EffectType SERIAL(effect);
    bool SERIAL(spiderWeb);
    SERIALIZE_ALL(effect, spiderWeb)
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
