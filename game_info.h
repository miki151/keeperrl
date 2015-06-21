#ifndef _GAME_INFO_H
#define _GAME_INFO_H

#include "view_object.h"
#include "unique_entity.h"
#include "minion_task.h"
#include "item_action.h"

enum class SpellId;

class PlayerMessage;

struct CreatureInfo {
  CreatureInfo(const Creature*);
  ViewId viewId;
  UniqueEntity<Creature>::Id uniqueId;
  string name;
  string speciesName;
  int expLevel;
  double morale;
};

class CollectiveInfo {
  public:
  string warning;
  struct Button {
    ViewId viewId;
    string name;
    optional<pair<ViewId, int>> cost;
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
    ViewId viewId;
    int count;
    string name;
  };
  vector<Resource> numResource;
  struct Team {
    TeamId id;
    vector<UniqueEntity<Creature>::Id> members;
    bool active;
  };
  vector<Team> teams;
  optional<int> currentTeam;
  bool newTeam;
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
};

struct ItemInfo {
  string name;
  string fullName;
  string description;
  int number;
  ViewId viewId;
  vector<UniqueEntity<Item>::Id> ids;
  vector<ItemAction> actions;
  bool equiped;
  bool locked;
  bool pending;
  optional<EquipmentSlot> slot;
  optional<CreatureInfo> owner;
};

class PlayerInfo {
  public:
  void readFrom(const Creature*);
  string getFirstName() const;
  string getTitle() const;
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
  string firstName;
  string name;
  int level;
  vector<string> adjectives;
  string levelName;
  string weaponName;
  struct Effect {
    string name;
    string help;
    bool bad;
  };
  vector<Effect> effects;
  struct Spell {
    SpellId id;
    string name;
    string help;
    optional<int> timeout;
  };
  vector<Spell> spells;
  vector<ItemInfo> lyingItems;
  vector<ItemInfo> inventory;
  vector<CreatureInfo> team;
  struct MinionTaskInfo {
    MinionTask task;
    bool inactive;
    bool current;
  };
  vector<MinionTaskInfo> minionTasks;
  UniqueEntity<Creature>::Id creatureId;
  double morale;
  ViewId viewId;
  enum Action {
    CONTROL,
    RENAME,
    BANISH,
  };
  vector<Action> actions;
};

/** Represents all the game information displayed around the map window.*/
class GameInfo {
  public:
  enum class InfoType { PLAYER, BAND, SPECTATOR} infoType = InfoType::PLAYER;
  double time;

  CollectiveInfo collectiveInfo;
  PlayerInfo playerInfo;

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
