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
#include "encyclopedia.h"
#include "time_queue.h"
#include "level_maker.h"

class PlayerControl;
class Level;
class ProgressMeter;
class Options;
class CreatureView;
class Trigger;

/**
  * Main class that holds all game logic.
  */
class Model {
  public:
  ~Model();

  /** Generates levels and all game entities for a collective game. */
  static Model* collectiveModel(ProgressMeter&, Options*, View*, const string& worldName);

  static Model* splashModel(ProgressMeter&, View*, const string& splashPath);

  enum class GameType { ADVENTURER, KEEPER, RETIRED_KEEPER };

  class ExitInfo {
    public:
    static ExitInfo saveGame(GameType);
    static ExitInfo abandonGame();

    bool isAbandoned() const;
    GameType getGameType() const;
    private:
    GameType type;
    bool abandon;
  };

  /** Makes an update to the game. This method is repeatedly called to make the game run.
    Returns the total logical time elapsed.*/
  optional<ExitInfo> update(double totalTime);

  /** Removes creature from current level and puts into the next, according to direction. */
  Vec2 changeLevel(StairDirection direction, StairKey key, Creature*);

  /** Removes creature from current level and puts into the given level */
  void changeLevel(Level*, Vec2 position, Creature*);

  /** Adds new creature to the queue. Assumes this creature has already been added to a level. */
  void addCreature(PCreature);

  /** Removes creature from the queue. Assumes it has already been removed from its level. */
  void removeCreature(Creature*);

  const vector<Collective*> getMainVillains() const;

  bool isTurnBased();

  string getGameIdentifier() const;
  string getShortGameIdentifier() const;
  void exitAction();
  double getTime() const;
  MusicType getCurrentMusic() const;
  void setCurrentMusic(MusicType);

  View* getView();
  void setView(View*);
  Options* getOptions();
  void setOptions(Options*);

  void tick(double time);
  void gameOver(const Creature* player, int numKills, const string& enemiesString, int points);
  void conquered(const string& title, const string& land, vector<const Creature*> kills, int points);
  void killedKeeper(const string& title, const string& keeper, const string& land,
    vector<const Creature*> kills, int points);
  static void showHighscore(View*, bool highlightLast = false);
  static void showCredits(View*);
  void retireCollective();

  struct SunlightInfo {
    double lightAmount;
    double timeRemaining;
    enum State { DAY, NIGHT } state;
    const char* getText();
  };
  const SunlightInfo& getSunlightInfo() const;

  const string& getWorldName() const;

  SERIALIZATION_DECL(Model);

  Encyclopedia keeperopedia;

  Trigger* getDanglingPortal();
  void setDanglingPortal(Trigger*);

  void addWoodCount(int);
  int getWoodCount() const;

  private:
  REGISTER_HANDLER(KilledLeaderEvent, const Collective*, const Creature*);

  Model(View* view, const string& worldName);

  void updateSunlightInfo();
  PCreature makePlayer(int handicap);
  const Creature* getPlayer() const;
  void landHeroPlayer();
  Level* buildLevel(Level::Builder&&, LevelMaker*);
  void addLink(StairDirection, StairKey, Level*, Level*);
  Level* prepareTopLevel(ProgressMeter&, vector<SettlementInfo> settlements);

  vector<PLevel> SERIAL(levels);
  vector<PCollective> SERIAL(collectives);
  Collective* SERIAL(playerCollective);
  vector<Collective*> SERIAL(mainVillains);
  View* view;
  TimeQueue SERIAL(timeQueue);
  vector<PCreature> SERIAL(deadCreatures);
  double SERIAL2(lastTick, -1000);
  map<tuple<StairDirection, StairKey, Level*>, Level*> SERIAL(levelLinks);
  PlayerControl* SERIAL2(playerControl, nullptr);
  bool SERIAL2(won, false);
  bool SERIAL2(addHero, false);
  bool SERIAL2(adventurer, false);
  double SERIAL2(currentTime, 0);
  SunlightInfo sunlightInfo;
  double lastUpdate = -10;
  Options* options;
  string SERIAL(worldName);
  MusicType SERIAL(musicType);
  optional<ExitInfo> exitInfo;
  unique_ptr<CreatureView> SERIAL(spectator);
  Trigger* SERIAL2(danglingPortal, nullptr);
  int SERIAL2(woodCount, 0);
};

#endif
