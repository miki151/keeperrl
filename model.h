#ifndef _MODEL_H
#define _MODEL_H

#include "util.h"
#include "level.h"
#include "action.h"
#include "level_generator.h"
#include "square_factory.h"
#include "monster.h"
#include "level_maker.h"

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

  bool isTurnBased();

  string getGameIdentifier() const;
  enum GameType { ADVENTURER, KEEPER };
  GameType getGameType() const;

  View* getView();
  void setView(View*);

  void onKillEvent(const Creature* victim, const Creature* killer) override;
  void gameOver(const Creature* player, int numKills, const string& enemiesString, int points);
  void conquered(const string& title, const string& land, vector<const Creature*> kills, int points);
  void showHighscore(bool highlightLast = false);

  SERIALIZATION_DECL(Model);

  private:
  void dwarvesMessage();
  Level* buildLevel(Level::Builder&& b, LevelMaker*, bool surface = false);
  void addLink(StairDirection, StairKey, Level*, Level*);
  Level* prepareTopLevel(vector<SettlementInfo> settlements);
  Level* prepareTopLevel2(vector<SettlementInfo> settlements);

  vector<PLevel> levels;
  View* view;
  TimeQueue timeQueue;
  vector<PCreature> deadCreatures;
  double lastTick = -1000;
  map<tuple<StairDirection, StairKey, Level*>, Level*> levelLinks;
  Collective* collective = nullptr;
  bool elvesDead = false;
  bool humansDead = false;
  bool dwarvesDead = false;
  bool won = false;
};

#endif
