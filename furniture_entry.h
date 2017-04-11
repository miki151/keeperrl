#pragma once

#include "util.h"
#include "effect_type.h"
#include "tribe.h"

struct SokobanEntryType {};
struct TrapEntryType {
  TrapEntryType(EffectType e, TribeId t, bool vis = false) : effect(e), tribeId(t), alwaysVisible(vis) {}
  SERIALIZATION_CONSTRUCTOR(TrapEntryType)
  EffectType SERIAL(effect);
  TribeId SERIAL(tribeId);
  bool SERIAL(alwaysVisible);
  SERIALIZE_ALL(effect, tribeId, alwaysVisible)
};

class Position;
class Creature;
class Furniture;

class FurnitureEntry {
  public:
  using EntryData = variant<SokobanEntryType, TrapEntryType>;
  FurnitureEntry(EntryData);
  void handle(WFurniture, WCreature);
  bool isVisibleTo(WConstFurniture, WConstCreature) const;

  SERIALIZATION_DECL(FurnitureEntry)

  private:
  EntryData SERIAL(entryData);
};
