#pragma once

#include "util.h"
#include "effect.h"
#include "tribe.h"

class Position;
class Creature;
class Furniture;

class FurnitureEntry {
  public:
  using Sokoban = EmptyStruct<struct SokobanTag>;

  struct Trap {
    Trap(Effect e, bool s = false) : effect(e), invisible(s) {}
    SERIALIZATION_CONSTRUCTOR(Trap)
    Effect SERIAL(effect);
    bool SERIAL(invisible) = false;
    SERIALIZE_ALL(NAMED(effect), OPTION(invisible))
  };

  using Water = EmptyStruct<struct WaterTag>;
  using Magma = EmptyStruct<struct MagmaTag>;

  MAKE_VARIANT(EntryData, Sokoban, Trap, Water, Magma);
  template <typename T>
  FurnitureEntry(const T& t) : FurnitureEntry(EntryData(t)) {}
  FurnitureEntry(EntryData);
  void handle(Furniture*, Creature*);
  bool isVisibleTo(const Furniture*, const Creature*) const;

  SERIALIZATION_DECL(FurnitureEntry)

  EntryData SERIAL(entryData);
};
