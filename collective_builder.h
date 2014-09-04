#ifndef _COLLECTIVE_BUILDER_H
#define _COLLECTIVE_BUILDER_H

#include "enums.h"
#include "util.h"

class Tribe;
class Level;
class Creature;

class CollectiveBuilder {
  public:
  CollectiveBuilder(CollectiveConfigId, Tribe*);
  void setLevel(Level*);
  void addCreature(Creature*, EnumSet<MinionTrait>);
  PCollective build();

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
