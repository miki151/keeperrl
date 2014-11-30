#ifndef _COLLECTIVE_BUILDER_H
#define _COLLECTIVE_BUILDER_H

#include "enums.h"
#include "util.h"
#include "minion_task.h"

class Tribe;
class Level;
class Creature;

class CollectiveBuilder {
  public:
  CollectiveBuilder(CollectiveConfigId, Tribe*);
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
  CollectiveConfigId config;
  Tribe* tribe;
};

#endif
