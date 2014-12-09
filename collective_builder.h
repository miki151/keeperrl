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

class CollectiveBuilder {
  public:
  CollectiveBuilder(CollectiveConfig, Tribe*);
  CollectiveBuilder& setLevel(Level*);
  CollectiveBuilder& addCreature(Creature*, EnumSet<MinionTrait>);
  PCollective build(const string& name);
  bool hasCreatures() const;

  private:
  Level* level;
  struct CreatureInfo {
    Creature* creature;
    EnumSet<MinionTrait> traits;
  };
  vector<CreatureInfo> creatures;
  CollectiveConfig config;
  Tribe* tribe;
};

#endif
