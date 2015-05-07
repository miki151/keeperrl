#ifndef _GAME_INFO_H
#define _GAME_INFO_H

#include "view_object.h"
#include "unique_entity.h"
#include "player_message.h"

/** Represents all the game information displayed around the map window.*/
class GameInfo {
  public:
  enum class InfoType { PLAYER, BAND, SPECTATOR} infoType = InfoType::PLAYER;
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
      optional<pair<ViewObject, int>> cost;
      string count;
      enum { ACTIVE, GRAY_CLICKABLE, INACTIVE} state;
      string help;
      char hotkey;
      string groupName;
    };
    vector<Button> buildings;
    vector<Button> minionButtons;
    vector<Button> libraryButtons;
    int minionCount;
    int minionLimit;
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
    optional<TeamId> currentTeam;
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

    struct Task {
      string name;
      optional<UniqueEntity<Creature>::Id> creature;
      bool priority;
    };
    vector<Task> taskMap;
  } bandInfo;

  class PlayerInfo {
    public:
    struct AttributeInfo {
      string name;
      enum Id { ATT, DEF, STR, DEX, ACC, SPD };
      Id id;
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
    int level;
    vector<string> adjectives;
    string title;
    string levelName;
    string weaponName;
    struct Effect {
      string name;
      string help;
      bool bad;
    };
    vector<Effect> effects;
    struct Spell {
      string name;
      string help;
      bool available;
    };
    vector<Spell> spells;
    struct ItemInfo {
      string name;
      string fullName;
      string description;
      int number;
      ViewObject viewObject;
      vector<UniqueEntity<Item>::Id> ids;
      enum Action { DROP, APPLY, EQUIP, UNEQUIP, THROW };
      vector<Action> actions;
      bool equiped;
    };
    vector<ItemInfo> lyingItems;
    struct InventorySection {
      string title;
      vector<ItemInfo> items;
    };
    vector<InventorySection> inventory;
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
