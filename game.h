#pragma once

#include "util.h"
#include "sunlight_info.h"
#include "tribe.h"
#include "enum_variant.h"
#include "position.h"
#include "exit_info.h"
#include "game_time.h"
#include "t_string.h"

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
class Encyclopedia;
class Unlocks;
class SteamAchievements;
class ProgressMeter;

struct WarlordInfoWithReference {
  vector<shared_ptr<Creature>> SERIAL(creatures);
  ContentFactory* SERIAL(contentFactory);
  string SERIAL(gameIdentifier);
  SERIALIZE_ALL_NO_VERSION(creatures, serializeAsValue(contentFactory), gameIdentifier)
};

class Game : public OwnedObject<Game> {
  public:
  static PGame campaignGame(Table<PModel>&&, CampaignSetup, AvatarInfo, ContentFactory, map<string, string> analytics);
  static PGame splashScreen(PModel&&, const CampaignSetup&, ContentFactory, View*);
  static PGame warlordGame(Table<PModel>, CampaignSetup, vector<PCreature>, ContentFactory, string avatarId);

  optional<ExitInfo> update(double timeDiff, milliseconds endTime);
  void setExitInfo(ExitInfo);
  Options* getOptions();
  Encyclopedia* getEncyclopedia();
  Unlocks* getUnlocks() const;
  EnemyAggressionLevel getEnemyAggressionLevel() const;
  void initialize(Options*, Highscores*, View*, FileSharing*, Encyclopedia*, Unlocks*, SteamAchievements*);
  void initializeModels(ProgressMeter&);
  View* getView() const;
  ContentFactory* getContentFactory();
  WarlordInfoWithReference getWarlordInfo();
  void exitAction();
  Model* chooseSite(const string& message, Model* current) const;
  void presentWorldmap();
  // if destinations are empty then creature is placed on the edge of the map
  void transferCreature(Creature*, Model* to, const vector<Position>& destinations = {});
  bool canTransferCreature(Creature*, Model* to);
  Position getTransferPos(Model* from, Model* to) const;
  string getGameIdentifier() const;
  string getGameOrRetiredIdentifier(Position) const;
  TString getGameDisplayName() const;
  MusicType getCurrentMusic() const;
  void setCurrentMusic(MusicType);
  void setDefaultMusic();
  Statistics& getStatistics();
  const Statistics& getStatistics() const;
  Tribe* getTribe(TribeId) const;
  GlobalTime getGlobalTime() const;
  Collective* getPlayerCollective() const;
  PlayerControl* getPlayerControl() const;
  void addPlayer(Creature*);
  void removePlayer(Creature*);
  const vector<Creature*>& getPlayerCreatures() const;

  int getModelDistance(const Collective* c1, const Collective* c2) const;

  const vector<Collective*>& getVillains(VillainType) const;
  const vector<Collective*>& getCollectives() const;

  const SunlightInfo& getSunlightInfo() const;
  const string& getWorldName() const;
  bool gameWon() const;

  void gameOver(const Creature* player, int numKills, int points);
  void conquered(const TString& title, int numKills, int points);
  void retired(const TString& title, int numKills, int points);

  bool isGameOver() const;
  bool isTurnBased();
  bool isVillainActive(const Collective*);
  SavedGameInfo getSavedGameInfo(vector<string> spriteMods) const;

  /** Removes creature from the queue. Assumes it has already been removed from its level. */
  void killCreature(Creature*, Creature* attacker);

  void handleMessageBoard(Position, Creature*);

  PModel& getMainModel();
  vector<Model*> getAllModels() const;
  bool isSingleModel() const;
  int getSaveProgressCount() const;
  Model* getCurrentModel() const;
  int getModelDifficulty(const Model*) const;
  bool passesMaxAggressorCutOff(const Model*);
  int getNumLesserVillainsDefeated() const;

  void prepareSiteRetirement();
  void doneRetirement();
  void addCollective(Collective*);

  void addEvent(const GameEvent&);
  void addAnalytics(const string& name, const string& value);
  void achieve(AchievementId) const;
  void setWasTransfered();

  ~Game();

  SERIALIZATION_DECL(Game)

  Game(Table<PModel>&&, Vec2 basePos, const CampaignSetup&, ContentFactory);

  unordered_set<string> SERIAL(effectFlags);
  vector<string> SERIAL(zLevelGroups);

  void clearPlayerControl();

  private:
  void tick(GlobalTime);
  bool updateModel(Model*, double timeDiff, optional<milliseconds> endTime);
  void uploadEvent(const string& name, const map<string, string>&);
  void considerAchievement(const GameEvent&);

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
  TString SERIAL(gameDisplayName);
  map<VillainType, vector<Collective*>> SERIAL(villainsByType);
  vector<Collective*> SERIAL(collectives);
  MusicType SERIAL(musicType);
  OwnerPointer<CreatureView> spectator;
  HeapAllocated<Statistics> SERIAL(statistics);
  Options* options = nullptr;
  Highscores* highscores = nullptr;
  Encyclopedia* encyclopedia = nullptr;
  optional<milliseconds> lastUpdate;
  PlayerControl* SERIAL(playerControl) = nullptr;
  Collective* SERIAL(playerCollective) = nullptr;
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
  void increaseTime(double diff);
  void spawnKeeper(AvatarInfo, vector<TString> introText);
  HeapAllocated<ContentFactory> SERIAL(contentFactory);
  void updateSunlightMovement();
  string SERIAL(avatarId);
  map<string, string> analytics;
  Unlocks* unlocks = nullptr;
  SteamAchievements* steamAchievements = nullptr;
  void considerAllianceAttack();
  bool SERIAL(allianceAttackPossible) = true;
  EnemyAggressionLevel SERIAL(enemyAggressionLevel);
  int SERIAL(numLesserVillainsDefeated) = 0;
};
