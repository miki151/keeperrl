#pragma once

#include "util.h"
#include "keeper_creature_info.h"
#include "adventurer_creature_info.h"

using PlayerCreaturesInfo = pair<vector<KeeperCreatureInfo>, vector<AdventurerCreatureInfo>>;

struct AvatarInfo {
  PCreature playerCreature;
  variant<KeeperCreatureInfo, AdventurerCreatureInfo> creatureInfo;
  TribeAlignment tribeAlignment;
};

class View;
class GameConfig;
class Options;

extern variant<AvatarInfo, AvatarMenuOption> getAvatarInfo(View*, const PlayerCreaturesInfo*, Options*, CreatureFactory*);
extern AvatarInfo getQuickGameAvatar(View*, const PlayerCreaturesInfo*, CreatureFactory*);
