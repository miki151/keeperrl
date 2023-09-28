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
struct CollectiveName;
class ContentFactory;

class CollectiveBuilder {
  public:
  CollectiveBuilder(const CollectiveConfig&, TribeId);
  CollectiveBuilder& setModel(Model*);
  CollectiveBuilder& setLevel(Level*);
  CollectiveBuilder& addCreature(Creature*, EnumSet<MinionTrait>);
  CollectiveBuilder& addArea(const vector<Vec2>&);
  CollectiveBuilder& setLocationName(const string&);
  CollectiveBuilder& setRaceName(const string&);
  CollectiveBuilder& setDiscoverable();
  TribeId getTribe();
  void setCentralPoint(Vec2);
  bool hasCentralPoint();

  PCollective build(const ContentFactory*) const;
  bool hasCreatures() const;
  optional<CollectiveName> generateName() const;

  private:
  optional<CollectiveName> getCollectiveName();
  Model* model = nullptr;
  Level* level = nullptr;
  struct CreatureInfo {
    Creature* creature = nullptr;
    EnumSet<MinionTrait> traits;
  };
  vector<CreatureInfo> creatures;
  HeapAllocated<CollectiveConfig> config;
  HeapAllocated<TribeId> tribe;
  vector<Vec2> squares;
  optional<Vec2> centralPoint;
  optional<string> locationName;
  optional<string> raceName;
  bool discoverable = false;
};
