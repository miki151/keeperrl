#pragma once

#include "util.h"
#include "avatar_variant.h"
#include "keeper_creature_info.h"
#include "adventurer_creature_info.h"

struct AvatarInfo {
  PCreature playerCreature;
  variant<KeeperCreatureInfo, AdventurerCreatureInfo> creatureInfo;
  TribeAlignment tribeAlignment;
};
