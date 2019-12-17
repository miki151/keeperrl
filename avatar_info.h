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
class ContentFactory;

extern variant<AvatarInfo, AvatarMenuOption> getAvatarInfo(View*, const vector<KeeperCreatureInfo>&,
    const vector<AdventurerCreatureInfo>&, ContentFactory*);
extern AvatarInfo getQuickGameAvatar(View*, const vector<KeeperCreatureInfo>&, CreatureFactory*);
