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

struct WarlordInfo {
  vector<PCreature> SERIAL(creatures);
  ContentFactory SERIAL(contentFactory);
  string SERIAL(gameIdentifier);
  SERIALIZE_ALL_NO_VERSION(creatures, contentFactory, gameIdentifier)
};

struct WarlordInfoWithReference {
  vector<shared_ptr<Creature>> SERIAL(creatures);
  ContentFactory* SERIAL(contentFactory);
  string SERIAL(gameIdentifier);
  SERIALIZE_ALL_NO_VERSION(creatures, serializeAsValue(contentFactory), gameIdentifier)
};

extern variant<AvatarInfo, WarlordInfo, AvatarMenuOption> getAvatarInfo(View*, const vector<pair<string, KeeperCreatureInfo>>&,
    const vector<pair<string, AdventurerCreatureInfo>>&, vector<WarlordInfo>, ContentFactory*);
extern AvatarInfo getQuickGameAvatar(View*, const vector<pair<string, KeeperCreatureInfo>>&, CreatureFactory*);
extern TribeId getPlayerTribeId(TribeAlignment);