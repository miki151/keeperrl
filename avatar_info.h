#pragma once

#include "util.h"
#include "keeper_creature_info.h"
#include "adventurer_creature_info.h"
#include "content_factory.h"
#include "villain_group.h"

struct AvatarInfo {
  PCreature playerCreature;
  variant<KeeperCreatureInfo, AdventurerCreatureInfo> creatureInfo;
  string avatarId;
  TribeAlignment tribeAlignment;
  vector<VillainGroup> villainGroups;
  optional<string> chosenBaseName;
};

class View;
class GameConfig;
class Options;
class Unlocks;

extern variant<AvatarInfo, AvatarMenuOption> getAvatarInfo(View*, const vector<pair<string, KeeperCreatureInfo>>&,
    const vector<pair<string, AdventurerCreatureInfo>>&, ContentFactory*, const Unlocks&);
extern AvatarInfo getQuickGameAvatar(View*, const vector<pair<string, KeeperCreatureInfo>>&, CreatureFactory*);
extern TribeId getPlayerTribeId(TribeAlignment);