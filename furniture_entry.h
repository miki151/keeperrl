#pragma once

#include "util.h"
#include "effect.h"
#include "tribe.h"

class Position;
class Creature;
class Furniture;

class FurnitureEntry {
  public:
  struct Sokoban {};

  struct Trap {
    Trap(Effect e, bool s = false) : effect(e), invisible(s) {}
    SERIALIZATION_CONSTRUCTOR(Trap)
    Effect SERIAL(effect);
    bool SERIAL(invisible);
    SERIALIZE_ALL(effect, invisible)
  };

  struct Water {};
  struct Magma {};

  using EntryData = variant<Sokoban, Trap, Water, Magma>;
  template <typename T>
  FurnitureEntry(const T& t) : FurnitureEntry(EntryData(t)) {}
  FurnitureEntry(EntryData);
  void handle(WFurniture, Creature*);
  bool isVisibleTo(WConstFurniture, const Creature*) const;

  SERIALIZATION_DECL(FurnitureEntry)

  EntryData SERIAL(entryData);
};
