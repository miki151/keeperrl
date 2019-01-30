#pragma once

#include "util.h"
#include "keeper_creature_info.h"
#include "adventurer_creature_info.h"

struct AvatarInfo {
  PCreature playerCreature;
  variant<KeeperCreatureInfo, AdventurerCreatureInfo> creatureInfo;
  TribeAlignment tribeAlignment;
};

class View;
class GameConfig;
class Options;

extern variant<AvatarInfo, AvatarMenuOption> getAvatarInfo(View*, GameConfig*, Options*, const CreatureFactory*);
extern AvatarInfo getQuickGameAvatar(View*, GameConfig*, const CreatureFactory*);
