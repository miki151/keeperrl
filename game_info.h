#ifndef _GAME_INFO_H
#define _GAME_INFO_H

#include "view_object.h"

/** Represents all the game information displayed around the map window.*/
class GameInfo {
  public:
  enum class InfoType { PLAYER, BAND} infoType;

  struct CreatureInfo {
    CreatureInfo(const Creature* c);
    ViewObject viewObject;
    UniqueId uniqueId;
    string name;
    string speciesName;
    int expLevel;
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
    vector<Button> workshop;
    vector<Button> minionButtons;
    vector<Button> libraryButtons;
    string monsterHeader;
    vector<CreatureInfo> creatures;
    vector<CreatureInfo> enemies;
    map<UniqueId, string> tasks;
    struct Resource {
      ViewObject viewObject;
      int count;
      string name;
    };
    vector<Resource> numResource;
    double time;
    bool gatheringTeam = false;
    vector<UniqueId> team;

    struct TechButton {
      ViewId viewId;
      string name;
      char hotkey;
    };
    vector<TechButton> techButtons;
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
    bool possessed;
    bool spellcaster;
    double time;
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
};

#endif
