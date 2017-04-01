#pragma once

#include "util.h"
#include "sunlight_info.h"
#include "tribe.h"
#include "enum_variant.h"
#include "position.h"
#include "exit_info.h"

class Options;
class Highscores;
class View;
class Statistics;
class PlayerControl;
class CreatureView;
class FileSharing;
class Technology;
class EventListener;
class GameEvent;
class Campaign;
class SavedGameInfo;
struct CampaignSetup;

class Game : public std::enable_shared_from_this<Game> {
  public:
  static PGame campaignGame(Table<PModel>&&, CampaignSetup&);
  static PGame splashScreen(PModel&&, const CampaignSetup&);

  optional<ExitInfo> update(double timeDiff);
  Options* getOptions();
  void initialize(Options*, Highscores*, View*, FileSharing*);
  View* getView() const;
  void exitAction();
  void transferAction(vector<WCreature>);
  void presentWorldmap();
  void transferCreature(WCreature, Model* to);
  bool canTransferCreature(WCreature, Model* to);
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
  WCollective getPlayerCollective() const;
  PlayerControl* getPlayerControl() const;
  void setPlayer(WCreature);
  WCreature getPlayer() const;
  void clearPlayer();

  int getModelDistance(WConstCollective c1, WConstCollective c2) const;

  const vector<WCollective>& getVillains(VillainType) const;
  const vector<WCollective>& getCollectives() const;

  const SunlightInfo& getSunlightInfo() const;
  const string& getWorldName() const;
  bool gameWon() const;

  void gameOver(WConstCreature player, int numKills, const string& enemiesString, int points);
  void conquered(const string& title, int numKills, int points);
  void killedKeeper(const string& title, const string& keeper, const string& land, int numKills, int points);
  bool isGameOver() const;
  bool isTurnBased();
  bool isVillainActive(WConstCollective);
  SavedGameInfo getSavedGameInfo() const;

  /** Removes creature from the queue. Assumes it has already been removed from its level. */
  void killCreature(WCreature, WCreature attacker);

  optional<Position> getOtherPortal(Position) const;
  void registerPortal(Position);
  void handleMessageBoard(Position, WCreature);

  PModel& getMainModel();
  vector<Model*> getAllModels() const;
  bool isSingleModel() const;
  int getSaveProgressCount() const;

  void prepareSiteRetirement();
  void doneRetirement();

  typedef function<void(EventListener*)> EventFun;
  void addEvent(const GameEvent&);

  ~Game();

  SERIALIZATION_DECL(Game)

  Game(Table<PModel>&&, Vec2 basePos, const CampaignSetup&);

  private:
  void updateSunlightInfo();
  void tick(double time);
  PCreature makeAdventurer(int handicap);
  Model* getCurrentModel() const;
  Vec2 getModelCoords(const Model*) const;
  optional<ExitInfo> updateModel(Model*, double totalTime);
  string getPlayerName() const;
  void uploadEvent(const string& name, const map<string, string>&);

  SunlightInfo sunlightInfo;
  Table<PModel> SERIAL(models);
  Table<bool> SERIAL(visited);
  map<Model*, double> SERIAL(localTime);
  Vec2 SERIAL(baseModel);
  View* view;
  double SERIAL(currentTime) = 0;
  optional<ExitInfo> exitInfo;
  Tribe::Map SERIAL(tribes);
  optional<double> SERIAL(lastTick);
  string SERIAL(gameIdentifier);
  string SERIAL(gameDisplayName);
  map<VillainType, vector<WCollective>> SERIAL(villainsByType);
  vector<WCollective> SERIAL(collectives);
  MusicType SERIAL(musicType);
  bool SERIAL(finishCurrentMusic) = true;
  unique_ptr<CreatureView> SERIAL(spectator);
  vector<Position> SERIAL(portals);
  HeapAllocated<Statistics> SERIAL(statistics);
  Options* options;
  Highscores* highscores;
  optional<milliseconds> lastUpdate;
  PlayerControl* SERIAL(playerControl) = nullptr;
  WCollective SERIAL(playerCollective) = nullptr;
  HeapAllocated<Campaign> SERIAL(campaign);
  bool wasTransfered = false;
  WCreature SERIAL(player) = nullptr;
  FileSharing* fileSharing;
  set<int> SERIAL(turnEvents);
  friend class GameListener;
  void considerRealTimeRender();
  void considerRetiredLoadedEvent(Vec2 coord);
};


