#ifndef _COLLECTIVE_BUILDER_H
#define _COLLECTIVE_BUILDER_H

#include "enums.h"
#include "util.h"
#include "minion_task.h"
#include "collective_config.h"

class Tribe;
class Level;
class Creature;
struct ImmigrantInfo;
class Position;

class CollectiveBuilder {
  public:
  CollectiveBuilder(CollectiveConfig, TribeId);
  CollectiveBuilder& setLevel(Level*);
  CollectiveBuilder& setCredit(map<CollectiveResourceId, int>);
  CollectiveBuilder& addCreature(Creature*);
  CollectiveBuilder& addSquares(const vector<Vec2>&);
  CollectiveBuilder& addSquares(const vector<Position>&);
  CollectiveBuilder& setName(const string&);
  PCollective build();
  bool hasCreatures() const;

  private:
  Level* level = nullptr;
  struct CreatureInfo {
    Creature* creature;
    EnumSet<MinionTrait> traits;
  };
  vector<CreatureInfo> creatures;
  CollectiveConfig config;
  TribeId tribe;
  map<CollectiveResourceId, int> credit;
  vector<Vec2> squares;
  optional<string> name;
};

#endif
