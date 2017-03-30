#pragma once

#include "enums.h"
#include "util.h"
#include "minion_trait.h"

class Tribe;
class TribeId;
class Level;
class Creature;
class ImmigrantInfo;
class Position;
class CollectiveConfig;

class CollectiveBuilder {
  public:
  CollectiveBuilder(const CollectiveConfig&, TribeId);
  CollectiveBuilder& setLevel(Level*);
  CollectiveBuilder& addCreature(WCreature);
  CollectiveBuilder& addSquares(const vector<Vec2>&);
  CollectiveBuilder& addSquares(const vector<Position>&);
  CollectiveBuilder& setLocationName(const string&);
  CollectiveBuilder& setRaceName(const string&);
  PCollective build();
  bool hasCreatures() const;

  private:
  Level* level = nullptr;
  struct CreatureInfo {
    WCreature creature;
    EnumSet<MinionTrait> traits;
  };
  vector<CreatureInfo> creatures;
  HeapAllocated<CollectiveConfig> config;
  HeapAllocated<TribeId> tribe;
  vector<Vec2> squares;
  optional<string> locationName;
  optional<string> raceName;
};
