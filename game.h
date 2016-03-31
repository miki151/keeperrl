#ifndef _GAME_H
#define _GAME_H

#include "util.h"
#include "sunlight_info.h"
#include "tribe.h"
#include "enum_variant.h"
#include "campaign.h"

class Options;
class Highscores;
class View;
class Statistics;
class PlayerControl;
class CreatureView;

enum class VillainType { MAIN, LESSER, PLAYER };
enum class GameSaveType { ADVENTURER, KEEPER, RETIRED_SINGLE, RETIRED_SITE, AUTOSAVE };

class Game {
  public:
  static PGame singleMapGame(const string& worldName, const string& playerName, PModel&&);
  static PGame campaignGame(Table<PModel>&&, Vec2 basePos, const string& playerName, const Campaign&);
  static PGame splashScreen(PModel&&);


  enum class ExitId { SAVE, QUIT };

  class ExitInfo : public EnumVariant<ExitId, TYPES(GameSaveType), ASSIGN(GameSaveType, ExitId::SAVE)> {
    using EnumVariant::EnumVariant;
  };

  optional<ExitInfo> update(double timeDiff);
  Options* getOptions();
  void setOptions(Options*);
  void setHighscores(Highscores*);
  void setView(View*);
  View* getView() const;
  void exitAction();
  void transferAction(const vector<Creature*>&);
  void transferCreatures(vector<Creature*>, Model* to);
  Position getTransferPos(Model* from, Model* to) const;
  string getGameIdentifier() const;
  string getGameDisplayName() const;
  MusicType getCurrentMusic() const;
  void setCurrentMusic(MusicType, bool now);
  bool changeMusicNow() const;
  Statistics& getStatistics();
  const Statistics& getStatistics() const;
  Tribe* getTribe(TribeId) const;
  double getGlobalTime() const;
  void landHeroPlayer();
  Collective* getPlayerCollective() const;
  void setPlayer(Creature*);
  Creature* getPlayer() const;
  void cancelPlayer(Creature*);

  const vector<Collective*>& getVillains(VillainType) const;
  const vector<Collective*>& getCollectives() const;

  const SunlightInfo& getSunlightInfo() const;
  const string& getWorldName() const;

  void gameOver(const Creature* player, int numKills, const string& enemiesString, int points);
  void conquered(const string& title, int numKills, int points);
  void killedKeeper(const string& title, const string& keeper, const string& land, int numKills, int points);
  bool isGameOver() const;
  bool isTurnBased();
  bool isSingleModel() const;
  bool isVillainActive(const Collective*);

  /** Removes creature from the queue. Assumes it has already been removed from its level. */
  void killCreature(Creature*, Creature* attacker);

  optional<Position> getOtherPortal(Position) const;
  void registerPortal(Position);

  void onTechBookRead(Technology*);
  void onAlarm(Position);
  void onKilledLeader(const Collective*, const Creature*);
  void onTorture(const Creature* who, const Creature* torturer);
  void onSurrender(Creature* who, const Creature* to);
  void onTrapTrigger(Position);
  void onTrapDisarm(Position, const Creature*);
  void onSquareDestroyed(Position);
  void onEquip(const Creature*, const Item*);

  PModel& getMainModel();
  void prepareRetirement();
  void doneRetirement();

  ~Game();

  SERIALIZATION_DECL(Game);

  private:
  Game(const string& worldName, const string& playerName, Table<PModel>&&, Vec2 basePos, optional<Campaign> = none);
  void updateSunlightInfo();
  void tick(double time);
  PCreature makeAdventurer(int handicap);
  Model* getCurrentModel() const;
  Vec2 getModelCoords(const Model*) const;
  optional<ExitInfo> updateModel(Model*, double totalTime);

  string SERIAL(worldName);
  SunlightInfo sunlightInfo;
  Table<PModel> SERIAL(models);
  map<Model*, double> SERIAL(localTime);
  Vec2 SERIAL(baseModel);
  View* view;
  double SERIAL(currentTime) = 0;
  optional<ExitInfo> exitInfo;
  Tribe::Map SERIAL(tribes);
  double SERIAL(lastTick) = 0;
  string SERIAL(gameIdentifier);
  string SERIAL(gameDisplayName);
  bool SERIAL(won) = false;
  map<VillainType, vector<Collective*>> SERIAL(villainsByType);
  vector<Collective*> SERIAL(collectives);
  MusicType SERIAL(musicType);
  bool SERIAL(finishCurrentMusic) = true;
  unique_ptr<CreatureView> SERIAL(spectator);
  vector<Position> SERIAL(portals);
  HeapAllocated<Statistics> SERIAL(statistics);
  Options* options;
  Highscores* highscores;
  double lastUpdate = -10;
  PlayerControl* SERIAL(playerControl) = nullptr;
  Collective* SERIAL(playerCollective) = nullptr;
  optional<Campaign> SERIAL(campaign);
  bool wasTransfered = false;
  Creature* SERIAL(player) = nullptr;
};


#endif
