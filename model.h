/* Copyright (C) 2013-2014 Michal Brzozowski (rusolis@poczta.fm)

   This file is part of KeeperRL.

   KeeperRL is free software; you can redistribute it and/or modify it under the terms of the
   GNU General Public License as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   KeeperRL is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
   even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along with this program.
   If not, see http://www.gnu.org/licenses/ . */

#ifndef _MODEL_H
#define _MODEL_H

#include "util.h"
#include "stair_key.h"
#include "position.h"
#include "tribe.h"
#include "enum_variant.h"

class PlayerControl;
class Level;
class ProgressMeter;
class Options;
class CreatureView;
class Trigger;
class Highscores;
class Technology;
class View;
class LevelMaker;
struct SettlementInfo;
class LevelBuilder;
class TimeQueue;
class Statistics;
struct TribeSet;
class StairKey;
class Game;

/**
  * Main class that holds all game logic.
  */
class Model {
  public:
  ~Model();
  
  /** Makes an update to the game. This method is repeatedly called to make the game run.
    Returns the total logical time elapsed.*/
  void update(double totalTime);

  /** Returns the level that the stairs lead to. */
  Level* getLinkedLevel(Level* from, StairKey) const;

  optional<Position> getStairs(const Level* from, const Level* to);

  /** Adds new creature to the queue. Assumes this creature has already been added to a level. */
  void addCreature(PCreature, double delay = 0);

  bool isTurnBased();

  double getTime() const;

  void setGame(Game*);
  Game* getGame() const;
  void tick(double time);
  vector<Collective*> getCollectives() const;
  vector<Level*> getLevels() const;

  void addWoodCount(int);
  int getWoodCount() const;

  void killCreature(Creature* victim, Creature* attacker);
  void updateSunlightMovement();
  const Creature* getPlayer() const;

  PCreature extractCreature(Creature*);
  void transferCreature(PCreature&&);

  Model();

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version);

  private:

  friend class ModelBuilder;

  PCreature makePlayer(int handicap);
  void landHeroPlayer();
  Level* buildLevel(LevelBuilder&&, PLevelMaker);
  Level* prepareTopLevel(ProgressMeter&, vector<SettlementInfo> settlements);

  vector<PLevel> SERIAL(levels);
  PLevel SERIAL(cemetery);
  vector<PCollective> SERIAL(collectives);
  Game* SERIAL(game);
  double SERIAL(lastTick) = 0;
  HeapAllocated<TimeQueue> SERIAL(timeQueue);
  vector<PCreature> SERIAL(deadCreatures);
  double SERIAL(currentTime) = 0;
  int SERIAL(woodCount) = 0;
  void calculateStairNavigation();
  optional<StairKey> getStairsBetween(const Level* from, const Level* to);
  map<pair<const Level*, const Level*>, StairKey> SERIAL(stairNavigation);
};

#endif
