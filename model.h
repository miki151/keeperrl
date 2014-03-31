#ifndef _MODEL_H
#define _MODEL_H

#include "util.h"
#include "level.h"
#include "action.h"
#include "level_generator.h"
#include "square_factory.h"
#include "monster.h"
#include "level_maker.h"
#include "village_control.h"
#include "collective.h"
#include "encyclopedia.h"

class Collective;

/**
  * Main class that holds all game logic.
  */
class Model : public EventListener {
  public:
  Model(View* view);
  ~Model();

  /** Generates levels and all game entities for a single player game. */
  static Model* heroModel(View* view);
 
  /** Generates levels and all game entities for a collective game. */
  static Model* collectiveModel(View* view);

  static Model* splashModel(View* view, const Table<bool>& bitmap);

  /** Makes an update to the game. This method is repeatedly called to make the game run.
    Returns the total logical time elapsed.*/
  void update(double totalTime);

  /** Removes creature from current level and puts into the next, according to direction. */
  Vec2 changeLevel(StairDirection direction, StairKey key, Creature*);

  /** Removes creature from current level and puts into the given level */
  void changeLevel(Level*, Vec2 position, Creature*);

  /** Adds new creature to the queue. Assumes this creature has already been added to a level. */
  void addCreature(PCreature);

  /** Removes creature from the queue. Assumes it has already been removed from its level. */
  void removeCreature(Creature*);

  const vector<VillageControl*> getVillageControls() const;

  bool isTurnBased();

  string getGameIdentifier() const;
  void exitAction();

  View* getView();
  void setView(View*);

  void tick(double time);
  void onKillEvent(const Creature* victim, const Creature* killer) override;
  void gameOver(const Creature* player, int numKills, const string& enemiesString, int points);
  void conquered(const string& title, const string& land, vector<const Creature*> kills, int points);
  void killedKeeper(const string& title, const string& keeper, const string& land,
    vector<const Creature*> kills, int points);
  void showHighscore(bool highlightLast = false);
  void retireCollective();

  SERIALIZATION_DECL(Model);

  Encyclopedia keeperopedia;

  private:
  PCreature makePlayer();
  const Creature* getPlayer() const;
  void landHeroPlayer();
  Level* buildLevel(Level::Builder&& b, LevelMaker*, bool surface = false);
  void addLink(StairDirection, StairKey, Level*, Level*);
  Level* prepareTopLevel(vector<SettlementInfo> settlements);
  Level* prepareTopLevel2(vector<SettlementInfo> settlements);

  vector<PLevel> SERIAL(levels);
  vector<PVillageControl> SERIAL(villageControls);
  View* view;
  TimeQueue SERIAL(timeQueue);
  vector<PCreature> SERIAL(deadCreatures);
  double SERIAL2(lastTick, -1000);
  map<tuple<StairDirection, StairKey, Level*>, Level*> SERIAL(levelLinks);
  unique_ptr<Collective> SERIAL(collective);
  bool SERIAL2(won, false);
  bool SERIAL2(addHero, false);
  bool SERIAL2(adventurer, false);
};

#endif
