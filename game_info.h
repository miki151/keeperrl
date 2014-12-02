#ifndef _GAME_INFO_H
#define _GAME_INFO_H

#include "view_object.h"
#include "unique_entity.h"
#include "player_message.h"

/** Represents all the game information displayed around the map window.*/
class GameInfo {
  public:
  enum class InfoType { PLAYER, BAND} infoType = InfoType::PLAYER;
  double time;

  struct CreatureInfo {
    CreatureInfo(const Creature*);
    ViewObject viewObject;
    UniqueEntity<Creature>::Id uniqueId;
    string name;
    string speciesName;
    int expLevel;
    double morale;
  };

  class BandInfo {
    public:
    string warning;
    struct Button {
      ViewObject object;
      string name;
      Optional<pair<ViewObject, int>> cost;
      string count;
      string inactiveReason;
      string help;
      char hotkey;
      string groupName;
    };
    vector<Button> buildings;
    vector<Button> minionButtons;
    vector<Button> libraryButtons;
    string monsterHeader;
    vector<CreatureInfo> minions;
    vector<CreatureInfo> enemies;
    map<UniqueEntity<Creature>::Id, string> tasks;
    struct Resource {
      ViewObject viewObject;
      int count;
      string name;
    };
    vector<Resource> numResource;
    Optional<TeamId> currentTeam;
    bool newTeam;
    map<TeamId, vector<UniqueEntity<Creature>::Id>> teams;
    CreatureInfo& getMinion(UniqueEntity<Creature>::Id);
    int nextPayout;
    int payoutTimeRemaining;

    struct TechButton {
      ViewId viewId;
      string name;
      char hotkey;
    };
    vector<TechButton> techButtons;

    struct Deity {
      string name;
      double standing;
    };
    vector<Deity> deities;
  } bandInfo;

  class PlayerInfo {
    public:
    struct AttributeInfo {
      string name;
      int value;
      int bonus;
      string help;
    };
    vector<AttributeInfo> attributes;
    struct SkillInfo {
      string name;
      string help;
    };
    vector<SkillInfo> skills;
    bool spellcaster;
    string playerName;
    vector<string> adjectives;
    string title;
    string levelName;
    string weaponName;
    struct Effect {
      string name;
      bool bad;
    };
    vector<Effect> effects;
    struct ItemInfo {
      string name;
      ViewObject viewObject;
    };
    vector<ItemInfo> lyingItems;
    string squareName;
    vector<CreatureInfo> team;
  } playerInfo;

  class VillageInfo {
    public:
    struct Village {
      string name;
      string tribeName;
      string state;
    };
    vector<Village> villages;
  } villageInfo;

  class SunlightInfo {
    public:
    string description;
    int timeRemaining;
  } sunlightInfo;

  vector<PlayerMessage> messageBuffer;
};

#endif
