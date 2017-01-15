#pragma once

#include "util.h"
#include "saved_game_info.h"
#include "save_file_info.h"

class View;
class ProgressMeter;
class Options;
class RetiredGames;

RICH_ENUM(CampaignType,
  CAMPAIGN, FREE_PLAY, ENDLESS
);

class Campaign {
  public:
  struct VillainInfo {
    ViewId SERIAL(viewId);
    EnemyId SERIAL(enemyId);
    string SERIAL(name);
    string getDescription() const;
    bool isEnemy() const;
    VillainType SERIAL(type);
    SERIALIZE_ALL(viewId, name, enemyId, type);
  };
  struct KeeperInfo {
    ViewId SERIAL(viewId);
    SERIALIZE_ALL(viewId);
  };
  struct RetiredInfo {
    SavedGameInfo SERIAL(gameInfo);
    SaveFileInfo SERIAL(fileInfo);
    SERIALIZE_ALL(gameInfo, fileInfo);
  };
  struct SiteInfo {
    vector<ViewId> SERIAL(viewId);
    optional<variant<VillainInfo, RetiredInfo, KeeperInfo>> SERIAL(dweller);
    optional<VillainInfo> getVillain() const;
    optional<RetiredInfo> getRetired() const;
    optional<KeeperInfo> getKeeper() const;
    bool isEnemy() const;
    bool isEmpty() const;
    bool SERIAL(blocked) = false;
    void setBlocked();
    optional<ViewId> getDwellerViewId() const;
    optional<string> getDwellerDescription() const;
    SERIALIZE_ALL(viewId, dweller, blocked);
  };

  enum PlayerRole { ADVENTURER, KEEPER};

  const Table<SiteInfo>& getSites() const;
  void clearSite(Vec2);
  static optional<Campaign> prepareCampaign(View*, Options*, function<RetiredGames()>, RandomGen&, PlayerRole);
  optional<Vec2> getPlayerPos() const;
  const string& getWorldName() const;
  bool isDefeated(Vec2) const;
  void setDefeated(Vec2);
  bool canTravelTo(Vec2) const;
  bool isInInfluence(Vec2) const;
  int getNumNonEmpty() const;
  bool canEmbark(Vec2) const;
  CampaignType getType() const;
  PlayerRole getPlayerRole() const;

  map<string, string> getParameters() const;
  vector<OptionId> getPrimaryOptions() const;
  vector<OptionId> getSecondaryOptions() const;

  const char* getSiteChoiceTitle() const;
  const char* getIntroText() const;

  SERIALIZATION_DECL(Campaign);

  private:
  void refreshInfluencePos();
  Campaign(Table<SiteInfo>, CampaignType);
  vector<VillainInfo> getMainVillains();
  vector<VillainInfo> getLesserVillains();
  vector<VillainInfo> getAllies();
  Table<SiteInfo> SERIAL(sites);
  optional<Vec2> SERIAL(playerPos);
  string SERIAL(worldName);
  Table<bool> SERIAL(defeated);
  set<Vec2> SERIAL(influencePos);
  int SERIAL(influenceSize);
  PlayerRole SERIAL(playerRole);
  void setPlayerPos(Vec2, Options* options);
  CampaignType SERIAL(type);
};


