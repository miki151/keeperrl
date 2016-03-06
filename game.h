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

class Game {
  public:
  static PGame singleMapGame(const string& worldName, const string& playerName, PModel&&);
  static PGame campaignGame(Table<PModel>&&, Vec2 basePos, const string& playerName, const Campaign&);
  static PGame splashScreen(PModel&&);

  enum class SaveType { ADVENTURER, KEEPER, RETIRED_KEEPER, AUTOSAVE };

  enum class ExitId { SAVE, QUIT, SWITCH_MODEL };

  class ExitInfo : public EnumVariant<ExitId, TYPES(SaveType), ASSIGN(SaveType, ExitId::SAVE)> {
    using EnumVariant::EnumVariant;
  };

  optional<ExitInfo> update(double timeDiff);
  optional<ExitInfo> updateModel(Model*, double totalTime);
  Options* getOptions();
  void setOptions(Options*);
  void setHighscores(Highscores*);
  void setView(View*);
  View* getView() const;
  void exitAction();
  void transferAction(const vector<Creature*>&);
  void transferCreature(Creature*, Model* to);
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
  void conquered(const string& title, vector<const Creature*> kills, int points);
  void killedKeeper(const string& title, const string& keeper, const string& land,
    vector<const Creature*> kills, int points);
  bool isGameOver() const;
  bool isTurnBased();

  /** Removes creature from the queue. Assumes it has already been removed from its level. */
  void killCreature(Creature*, Creature* attacker);

  optional<Position> getDanglingPortal();
  void setDanglingPortal(Position);
  void resetDanglingPortal();

  void onTechBookRead(Technology*);
  void onAlarm(Position);
  void onKilledLeader(const Collective*, const Creature*);
  void onTorture(const Creature* who, const Creature* torturer);
  void onSurrender(Creature* who, const Creature* to);
  void onAttack(Creature* victim, Creature* attacker);
  void onTrapTrigger(Position);
  void onTrapDisarm(Position, const Creature*);
  void onSquareDestroyed(Position);
  void onEquip(const Creature*, const Item*);

  ~Game();

  SERIALIZATION_DECL(Game);

  private:
  Game(const string& worldName, const string& playerName, Table<PModel>&&, Vec2 basePos, optional<Campaign> = none);
  void updateSunlightInfo();
  void tick(double time);
  PCreature makeAdventurer(int handicap);
  Model* getCurrentModel() const;

  string SERIAL(worldName);
  SunlightInfo sunlightInfo;
  Table<PModel> SERIAL(models);
  map<Model*, double> SERIAL(localTime);
  Vec2 SERIAL(baseModel);
  View* view;
  double SERIAL(currentTime) = 0;
  optional<ExitInfo> exitInfo;
  EnumMap<TribeId, PTribe> SERIAL(tribes);
  double SERIAL(lastTick) = -1000;
  string SERIAL(gameIdentifier);
  string SERIAL(gameDisplayName);
  bool SERIAL(won) = false;
  map<VillainType, vector<Collective*>> SERIAL(villainsByType);
  vector<Collective*> SERIAL(collectives);
  MusicType SERIAL(musicType);
  bool SERIAL(finishCurrentMusic) = true;
  unique_ptr<CreatureView> SERIAL(spectator);
  optional<Position> SERIAL(danglingPortal);
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
