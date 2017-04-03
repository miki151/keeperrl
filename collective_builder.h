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
  CollectiveBuilder& setLevel(WLevel);
  CollectiveBuilder& addCreature(WCreature);
  CollectiveBuilder& setArea(Rectangle);
  CollectiveBuilder& setLocationName(const string&);
  CollectiveBuilder& setRaceName(const string&);
  PCollective build();
  bool hasCreatures() const;

  private:
  WLevel level = nullptr;
  struct CreatureInfo {
    WCreature creature;
    EnumSet<MinionTrait> traits;
  };
  vector<CreatureInfo> creatures;
  HeapAllocated<CollectiveConfig> config;
  HeapAllocated<TribeId> tribe;
  vector<Vec2> squares;
  optional<Vec2> centralPoint;
  optional<string> locationName;
  optional<string> raceName;
};
