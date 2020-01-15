#pragma once

#include "util.h"
#include "sunlight_info.h"
#include "tribe.h"
#include "enum_variant.h"
#include "position.h"
#include "exit_info.h"
#include "game_time.h"

class Options;
class Highscores;
class View;
class Statistics;
class PlayerControl;
class CreatureView;
class FileSharing;
class Technology;
class GameEvent;
class Campaign;
class SavedGameInfo;
struct CampaignSetup;
class AvatarInfo;
class ContentFactory;
class NameGenerator;

class Game : public OwnedObject<Game> {
  public:
  static PGame campaignGame(Table<PModel>&&, CampaignSetup&, AvatarInfo, ContentFactory);
  static PGame splashScreen(PModel&&, const CampaignSetup&, ContentFactory);

  optional<ExitInfo> update(double timeDiff);
  void setExitInfo(ExitInfo);
  Options* getOptions();
  void initialize(Options*, Highscores*, View*, FileSharing*);
  View* getView() const;
  ContentFactory* getContentFactory();
  ContentFactory removeContentFactory();
  void exitAction();
  void transferAction(vector<Creature*>);
  void presentWorldmap();
  // if destinations are empty then creature is placed on the edge of the map
  void transferCreature(Creature*, WModel to, const vector<Position>& destinations = {});
  bool canTransferCreature(Creature*, WModel to);
  Position getTransferPos(WModel from, WModel to) const;
  string getGameIdentifier() const;
  string getGameDisplayName() const;
  MusicType getCurrentMusic() const;
  void setCurrentMusic(MusicType);
  void setDefaultMusic();
  Statistics& getStatistics();
  const Statistics& getStatistics() const;
  Tribe* getTribe(TribeId) const;
  GlobalTime getGlobalTime() const;
  WCollective getPlayerCollective() const;
  WPlayerControl getPlayerControl() const;
  void addPlayer(Creature*);
  void removePlayer(Creature*);
  const vector<Creature*>& getPlayerCreatures() const;

  int getModelDistance(WConstCollective c1, WConstCollective c2) const;

  const vector<WCollective>& getVillains(VillainType) const;
  const vector<WCollective>& getCollectives() const;

  const SunlightInfo& getSunlightInfo() const;
  const string& getWorldName() const;
  bool gameWon() const;

  void gameOver(const Creature* player, int numKills, const string& enemiesString, int points);
  void conquered(const string& title, int numKills, int points);
  void retired(const string& title, int numKills, int points);

  bool isGameOver() const;
  bool isTurnBased();
  bool isVillainActive(WConstCollective);
  SavedGameInfo getSavedGameInfo(vector<string> spriteMods) const;

  /** Removes creature from the queue. Assumes it has already been removed from its level. */
  void killCreature(Creature*, Creature* attacker);

  void handleMessageBoard(Position, Creature*);

  PModel& getMainModel();
  vector<WModel> getAllModels() const;
  bool isSingleModel() const;
  int getSaveProgressCount() const;
  WModel getCurrentModel() const;

  void prepareSiteRetirement();
  void doneRetirement();
  void addCollective(WCollective);

  void addEvent(const GameEvent&);

  ~Game();

  SERIALIZATION_DECL(Game)

  Game(Table<PModel>&&, Vec2 basePos, const CampaignSetup&, ContentFactory);

  private:
  optional<ExitInfo> update();
  void tick(GlobalTime);
  Vec2 getModelCoords(const WModel) const;
  bool updateModel(WModel, double timeDiff);
  string getPlayerName() const;
  void uploadEvent(const string& name, const map<string, string>&);

  SunlightInfo sunlightInfo;
  Table<PModel> SERIAL(models);
  Table<bool> SERIAL(visited);
  map<LevelId, double> SERIAL(localTime);
  Vec2 SERIAL(baseModel);
  View* view = nullptr;
  double SERIAL(currentTime) = 0;
  optional<ExitInfo> exitInfo;
  Tribe::Map SERIAL(tribes);
  optional<int> SERIAL(lastTick);
  string SERIAL(gameIdentifier);
  string SERIAL(gameDisplayName);
  map<VillainType, vector<WCollective>> SERIAL(villainsByType);
  vector<WCollective> SERIAL(collectives);
  MusicType SERIAL(musicType);
  unique_ptr<CreatureView> SERIAL(spectator);
  HeapAllocated<Statistics> SERIAL(statistics);
  Options* options = nullptr;
  Highscores* highscores = nullptr;
  optional<milliseconds> lastUpdate;
  WPlayerControl SERIAL(playerControl) = nullptr;
  WCollective SERIAL(playerCollective) = nullptr;
  HeapAllocated<Campaign> SERIAL(campaign);
  bool wasTransfered = false;
  vector<Creature*> SERIAL(players);
  FileSharing* fileSharing = nullptr;
  set<int> SERIAL(turnEvents);
  TimeInterval SERIAL(sunlightTimeOffset);
  friend class GameListener;
  void considerRealTimeRender();
  void considerRetiredLoadedEvent(Vec2 coord);
  optional<ExitInfo> updateInput();
  void initializeModels();
  void increaseTime(double diff);
  void spawnKeeper(AvatarInfo, vector<string> introText);
  HeapAllocated<ContentFactory> SERIAL(contentFactory);
};

CEREAL_CLASS_VERSION(Game, 1);
