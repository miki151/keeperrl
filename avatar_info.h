#pragma once

#include "util.h"
#include "keeper_creature_info.h"
#include "adventurer_creature_info.h"
#include "content_factory.h"

struct AvatarInfo {
  PCreature playerCreature;
  variant<KeeperCreatureInfo, AdventurerCreatureInfo> creatureInfo;
  TribeAlignment tribeAlignment;
  optional<string> chosenBaseName;
};

class View;
class GameConfig;
class Options;

struct WarlordInfo {
  vector<PCreature> SERIAL(creatures);
  ContentFactory SERIAL(factory);
  SERIALIZE_ALL_NO_VERSION(creatures, factory)
};

struct WarlordInfoWithReference {
  vector<shared_ptr<Creature>> SERIAL(creatures);
  ContentFactory* SERIAL(factory);
  SERIALIZE_ALL_NO_VERSION(creatures, serializeAsValue(factory))
};

extern variant<AvatarInfo, AvatarMenuOption> getAvatarInfo(View*, const vector<KeeperCreatureInfo>&,
    const vector<AdventurerCreatureInfo>&, const vector<WarlordInfo>&, ContentFactory*);
extern AvatarInfo getQuickGameAvatar(View*, const vector<KeeperCreatureInfo>&, CreatureFactory*);
