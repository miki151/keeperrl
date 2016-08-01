#ifndef _GAME_INFO_H
#define _GAME_INFO_H

#include "view_object.h"
#include "unique_entity.h"
#include "minion_task.h"
#include "item_action.h"
#include "village_action.h"
#include "cost_info.h"
#include "attack_trigger.h"
#include "view_id.h"
#include "player_message.h"

enum class SpellId;

class PlayerMessage;

struct CreatureInfo {
  CreatureInfo(const Creature*);
  ViewId HASH(viewId);
  UniqueEntity<Creature>::Id HASH(uniqueId);
  string HASH(name);
  string HASH(stackName);
  int HASH(expLevel);
  double HASH(morale); 
  optional<pair<ViewId, int>> HASH(cost);
  HASH_ALL(viewId, uniqueId, name, stackName, expLevel, morale, cost);
};


struct ItemInfo {
  string HASH(name);
  string HASH(fullName);
  string HASH(description);
  int HASH(number);
  ViewId HASH(viewId);
  vector<UniqueEntity<Item>::Id> HASH(ids);
  vector<ItemAction> HASH(actions);
  bool HASH(equiped);
  bool HASH(locked);
  bool HASH(pending);
  bool HASH(unavailable);
  optional<EquipmentSlot> HASH(slot);
  optional<CreatureInfo> HASH(owner);
  enum Type {EQUIPMENT, CONSUMABLE, OTHER} HASH(type);
  optional<pair<ViewId, int>> HASH(price);
  HASH_ALL(name, fullName, description, number, viewId, ids, actions, equiped, locked, pending, unavailable, slot, owner, type, price);
};


class PlayerInfo {
  public:
  void readFrom(const Creature*);
  string getFirstName() const;
  string getTitle() const;
  struct AttributeInfo {
    string HASH(name);
    enum Id { ATT, DEF, STR, DEX, ACC, SPD };
    Id HASH(id);
    int HASH(value);
    int HASH(bonus);
    string HASH(help);
    HASH_ALL(name, id, value, bonus, help);
  };
  vector<AttributeInfo> HASH(attributes);
  struct SkillInfo {
    string HASH(name);
    string HASH(help);
    HASH_ALL(name, help);
  };
  vector<SkillInfo> HASH(skills);
  string HASH(firstName);
  string HASH(name);
  string HASH(title);
  int HASH(level);
  vector<string> HASH(adjectives);
  string description;
  string HASH(levelName);
  int HASH(positionHash);
  string HASH(weaponName);
  struct Effect {
    string HASH(name);
    string HASH(help);
    bool HASH(bad);
    HASH_ALL(name, help, bad);
  };
  vector<Effect> HASH(effects);
  struct Spell {
    SpellId HASH(id);
    string HASH(name);
    string HASH(help);
    optional<int> HASH(timeout);
    HASH_ALL(id, name, help, timeout);
  };
  vector<Spell> HASH(spells);
  vector<ItemInfo> HASH(lyingItems);
  vector<ItemInfo> HASH(inventory);
  vector<CreatureInfo> HASH(team);
  struct MinionTaskInfo {
    MinionTask HASH(task);
    bool HASH(inactive);
    bool HASH(current);
    bool HASH(locked);
    HASH_ALL(task, inactive, current, locked);
  };
  vector<MinionTaskInfo> HASH(minionTasks);
  UniqueEntity<Creature>::Id HASH(creatureId);
  double HASH(morale);
  ViewId HASH(viewId);
  enum Action {
    CONTROL,
    RENAME,
    BANISH,
    WHIP,
    EXECUTE,
    TORTURE,
  };
  vector<Action> HASH(actions);
  HASH_ALL(attributes, skills, firstName, name, title, level, adjectives, levelName, positionHash, weaponName, effects, spells, lyingItems, inventory, team, minionTasks, creatureId, morale, viewId, actions);
};


class CollectiveInfo {
  public:
  string HASH(warning);
  struct Button {
    ViewId HASH(viewId);
    string HASH(name);
    optional<pair<ViewId, int>> HASH(cost);
    string HASH(count);
    enum { ACTIVE, GRAY_CLICKABLE, INACTIVE} HASH(state);
    string HASH(help);
    char HASH(hotkey);
    string HASH(groupName);
    bool HASH(hotkeyOpensGroup);
    HASH_ALL(viewId, name, cost, count, state, help, hotkey, groupName, hotkeyOpensGroup);
  };
  vector<Button> HASH(buildings);
  vector<Button> HASH(minionButtons);
  vector<Button> HASH(libraryButtons);
  int HASH(minionCount);
  int HASH(minionLimit);
  string HASH(monsterHeader);
  vector<CreatureInfo> HASH(minions);
  struct CreatureGroup {
    UniqueEntity<Creature>::Id HASH(creatureId);
    string HASH(name);
    ViewId HASH(viewId);
    int HASH(count);
    bool HASH(highlight);
    HASH_ALL(creatureId, name, viewId, count, highlight);
  };
  vector<CreatureGroup> HASH(minionGroups);
  vector<CreatureGroup> HASH(enemyGroups);
  struct ChosenInfo {
    UniqueEntity<Creature>::Id HASH(chosenId);
    vector<PlayerInfo> HASH(creatures);
    optional<TeamId> HASH(teamId);
    HASH_ALL(chosenId, creatures, teamId);
  };
  optional<ChosenInfo> HASH(chosen);
  struct Resource {
    ViewId HASH(viewId);
    int HASH(count);
    string HASH(name);
    HASH_ALL(viewId, count, name);
  };
  vector<Resource> HASH(numResource);
  struct Team {
    TeamId HASH(id);
    vector<UniqueEntity<Creature>::Id> HASH(members);
    bool HASH(active);
    bool HASH(highlight);
    HASH_ALL(id, members, active, highlight);
  };
  vector<Team> HASH(teams);
  CreatureInfo* getMinion(UniqueEntity<Creature>::Id);
  bool hasMinion(UniqueEntity<Creature>::Id);
  int HASH(nextPayout);
  int HASH(payoutTimeRemaining);

  struct TechButton {
    ViewId HASH(viewId);
    string HASH(name);
    char HASH(hotkey);
    HASH_ALL(viewId, name, hotkey);
  };
  vector<TechButton> HASH(techButtons);

  struct Task {
    string HASH(name);
    optional<UniqueEntity<Creature>::Id> HASH(creature);
    bool HASH(priority);
    HASH_ALL(name, creature, priority);
  };
  vector<Task> HASH(taskMap);
  
  struct Ransom {
    pair<ViewId, int> HASH(amount);
    string HASH(attacker);
    bool HASH(canAfford);
    HASH_ALL(amount, attacker, canAfford);
  };
  optional<Ransom> HASH(ransom);

  HASH_ALL(warning, buildings, minionButtons, libraryButtons, minionCount, minionLimit, monsterHeader, minions, minionGroups, enemyGroups, chosen, numResource, teams, nextPayout, payoutTimeRemaining, techButtons, taskMap, ransom);
};

class VillageInfo {
  public:
  struct Village {
    string HASH(name);
    string HASH(tribeName);
    enum Access { ACTIVE, INACTIVE, LOCATION, NO_LOCATION };
    Access HASH(access);
    enum State { FRIENDLY, HOSTILE, CONQUERED } HASH(state);
    struct ActionInfo {
      VillageAction HASH(action);
      optional<string> HASH(disabledReason);
      HASH_ALL(action, disabledReason);
    };
    struct TriggerInfo {
      string HASH(name);
      double HASH(value);
      HASH_ALL(name, value);
    };
    vector<ActionInfo> HASH(actions);
    vector<TriggerInfo> HASH(triggers);
    HASH_ALL(name, tribeName, access, state, actions, triggers);
  };
  vector<Village> HASH(villages);
  int HASH(numMainVillains);
  int HASH(totalMain);
  int HASH(numConquered);
  HASH_ALL(villages, numMainVillains, totalMain, numConquered);
};

class GameSunlightInfo {
  public:
  string HASH(description);
  int HASH(timeRemaining);
  HASH_ALL(description, timeRemaining);
};

/** Represents all the game information displayed around the map window.*/
class GameInfo {
  public:
  enum class InfoType { PLAYER, BAND, SPECTATOR} HASH(infoType) = InfoType::PLAYER;
  int HASH(time);
  int HASH(modifiedSquares);
  int HASH(totalSquares);

  bool HASH(singleModel);

  CollectiveInfo HASH(collectiveInfo);
  PlayerInfo HASH(playerInfo);
  VillageInfo HASH(villageInfo);
  GameSunlightInfo HASH(sunlightInfo);

  vector<PlayerMessage> HASH(messageBuffer);
  HASH_ALL(infoType, time, collectiveInfo, playerInfo, villageInfo, sunlightInfo, messageBuffer, singleModel, modifiedSquares, totalSquares);
};

#endif
