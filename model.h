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
#include "statistics.h"

class PlayerControl;
class Level;
class ProgressMeter;
class Options;
class CreatureView;
class Trigger;
class Highscores;
class Technology;

/**
  * Main class that holds all game logic.
  */
class Model {
  public:
  ~Model();

  enum class GameType { ADVENTURER, KEEPER, RETIRED_KEEPER, AUTOSAVE };

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
  string getGameDisplayName() const;
  void exitAction();
  double getTime() const;
  MusicType getCurrentMusic() const;
  void setCurrentMusic(MusicType, bool now);
  bool changeMusicNow() const;

  View* getView();
  void setView(View*);
  Options* getOptions();
  void setOptions(Options*);
  void setHighscores(Highscores*);

  Statistics& getStatistics();
  const Statistics& getStatistics() const;

  void tick(double time);
  void gameOver(const Creature* player, int numKills, const string& enemiesString, int points);
  void conquered(const string& title, vector<const Creature*> kills, int points);
  void killedKeeper(const string& title, const string& keeper, const string& land,
    vector<const Creature*> kills, int points);
  bool isGameOver() const;
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

  struct PortalInfo : public NamedTupleBase<Level*, Vec2> {
    NAMED_TUPLE_STUFF(PortalInfo);
    NAME_ELEM(0, level);
    NAME_ELEM(1, position);
  };
  optional<PortalInfo> getDanglingPortal();
  void setDanglingPortal(PortalInfo);
  void resetDanglingPortal();

  void addWoodCount(int);
  int getWoodCount() const;

  Tribe* getPestTribe();
  Tribe* getKillEveryoneTribe();
  Tribe* getPeacefulTribe();

  void onTechBookRead(Technology*);

  private:
  REGISTER_HANDLER(KilledLeaderEvent, const Collective*, const Creature*);

  Model(View* view, const string& worldName, Tribe::Set&&);

  friend class ModelBuilder;

  void updateSunlightInfo();
  PCreature makePlayer(int handicap);
  const Creature* getPlayer() const;
  void landHeroPlayer();
  Level* buildLevel(Level::Builder&&, LevelMaker*);
  void addLink(StairDirection, StairKey, Level*, Level*);
  Level* prepareTopLevel(ProgressMeter&, vector<SettlementInfo> settlements);

  Tribe::Set SERIAL(tribeSet);
  vector<PLevel> SERIAL(levels);
  vector<PCollective> SERIAL(collectives);
  Collective* SERIAL(playerCollective);
  vector<Collective*> SERIAL(mainVillains);
  View* view;
  TimeQueue SERIAL(timeQueue);
  vector<PCreature> SERIAL(deadCreatures);
  double SERIAL(lastTick) = -1000;
  map<tuple<StairDirection, StairKey, Level*>, Level*> SERIAL(levelLinks);
  PlayerControl* SERIAL(playerControl) = nullptr;
  bool SERIAL(won) = false;
  bool SERIAL(addHero) = false;
  bool SERIAL(adventurer) = false;
  double SERIAL(currentTime) = 0;
  SunlightInfo sunlightInfo;
  double lastUpdate = -10;
  Options* options;
  Highscores* highscores;
  string SERIAL(worldName);
  MusicType SERIAL(musicType);
  bool SERIAL(finishCurrentMusic) = false;
  optional<ExitInfo> exitInfo;
  unique_ptr<CreatureView> SERIAL(spectator);
  optional<PortalInfo> SERIAL(danglingPortal);
  int SERIAL(woodCount) = 0;
  Statistics SERIAL(statistics);
  string SERIAL(gameIdentifier);
  string SERIAL(gameDisplayName);
};

BOOST_CLASS_VERSION(Model, 3)

#endif
