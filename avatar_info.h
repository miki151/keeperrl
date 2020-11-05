#pragma once

#include "util.h"
#include "keeper_creature_info.h"
#include "adventurer_creature_info.h"
#include "content_factory.h"

struct AvatarInfo {
  PCreature playerCreature;
  variant<KeeperCreatureInfo, AdventurerCreatureInfo> creatureInfo;
  string avatarId;
  TribeAlignment tribeAlignment;
  optional<string> chosenBaseName;
};

class View;
class GameConfig;
class Options;

extern variant<AvatarInfo, AvatarMenuOption> getAvatarInfo(View*, const vector<pair<string, KeeperCreatureInfo>>&,
    const vector<pair<string, AdventurerCreatureInfo>>&, ContentFactory*);
extern AvatarInfo getQuickGameAvatar(View*, const vector<pair<string, KeeperCreatureInfo>>&, CreatureFactory*);
extern TribeId getPlayerTribeId(TribeAlignment);