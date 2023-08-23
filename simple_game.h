#pragma once

#include "stdafx.h"
#include "creature.h"
#include "technology.h"
#include "resource_id.h"
#include "keeper_creature_info.h"
#include "immigrant_info.h"
#include "dungeon_level.h"
#include "z_level_info.h"

class MainLoop;

class SimpleGame {
  public:
  SimpleGame(ContentFactory*, MainLoop*);
  void update();

  int zLevel = 0;
  Technology technology;
  vector<PCreature> minions;
  HashMap<CollectiveResourceId, int> resources;
  vector<ImmigrantInfo> immigrants;
  vector<ZLevelType> zLevels;
  DungeonLevel dungeonLevel;
  int maxPopulation = 10;
  ContentFactory* factory;
  MainLoop* mainLoop;
  private:
  using SimpleAction = function<void()>;
  vector<pair<string, SimpleAction>> getActions();
  bool meetsRequirements(const ImmigrantInfo&);
  void addImmigrant();
  void increaseZLevel();
  void research();
  void addResourcesForLevel(int level);
  bool fightEnemy(EnemyId);
};
