#pragma once

#include "util.h"
#include "saved_game_info.h"
#include "save_file_info.h"
#include "enemy_id.h"
#include "tribe.h"
#include "biome_id.h"

class View;
class ProgressMeter;
class Options;
class RetiredGames;
class ContentFactory;

struct CampaignSetup;

struct VillainViewId {
  ViewIdList SERIAL(ids);
  SERIALIZE_ALL(ids)
  void serialize(PrettyInputArchive& ar, unsigned int);
};

class Campaign {
  public:
  struct VillainInfo {
    ViewId SERIAL(dwellingId);
    EnemyId SERIAL(enemyId);
    string SERIAL(name);
    bool SERIAL(alwaysPresent) = false;
    string getDescription() const;
    bool isEnemy() const;
    VillainType SERIAL(type);
    SERIALIZE_ALL(NAMED(dwellingId), NAMED(enemyId), NAMED(name), NAMED(type), OPTION(alwaysPresent))
  };
  struct KeeperInfo {
    ViewIdList SERIAL(viewId);
    TribeId SERIAL(tribe);
    SERIALIZE_ALL(viewId, tribe)
  };
  struct RetiredInfo {
    SavedGameInfo SERIAL(gameInfo);
    SaveFileInfo SERIAL(fileInfo);
    SERIALIZE_ALL(gameInfo, fileInfo)
  };
  struct SiteInfo {
    vector<ViewId> SERIAL(viewId);
    optional<BiomeId> SERIAL(biome);
    typedef variant<VillainInfo, RetiredInfo, KeeperInfo> Dweller;
    optional<Dweller> SERIAL(dweller);
    vector<SavedGameInfo::MinionInfo> SERIAL(inhabitants);
    optional<VillainInfo> getVillain() const;
    optional<RetiredInfo> getRetired() const;
    optional<KeeperInfo> getKeeper() const;
    bool isEnemy() const;
    bool isEmpty() const;
    bool SERIAL(blocked) = false;
    optional<ViewIdList> getDwellingViewId() const;
    optional<string> getDwellerDescription() const;
    optional<string> getDwellerName() const;
    optional<VillainType> getVillainType() const;
    SERIALIZE_ALL(viewId, biome, dweller, blocked, inhabitants)
  };

  const Table<SiteInfo>& getSites() const;
  Vec2 getPlayerPos() const;
  Vec2 getOriginalPlayerPos() const;
  bool isGoodStartPos(Vec2) const;
  BiomeId getBaseBiome() const;
  const string& getWorldName() const;
  bool isDefeated(Vec2) const;
  void setDefeated(const ContentFactory*, Vec2);
  void removeDweller(Vec2);
  bool canTravelTo(Vec2) const;
  bool isInInfluence(Vec2) const;
  int getNumNonEmpty() const;
  int getMapZoom() const;
  int getMinimapZoom() const;
  int getBaseLevelIncrease(Vec2) const;
  bool passesMaxAggressorCutOff(Vec2);
  CampaignType getType() const;
  void updateInhabitants(ContentFactory*);

  map<string, string> getParameters() const;

  SERIALIZATION_DECL(Campaign)

  private:
  friend class CampaignBuilder;
  void refreshInfluencePos(const ContentFactory*);
  void refreshMaxAggressorCutOff();
  Campaign(Table<SiteInfo>, CampaignType, const string& worldName);
  Table<SiteInfo> SERIAL(sites);
  Vec2 SERIAL(playerPos);
  string SERIAL(worldName);
  Table<bool> SERIAL(defeated);
  set<Vec2> SERIAL(influencePos);
  CampaignType SERIAL(type);
  int SERIAL(mapZoom);
  int SERIAL(minimapZoom) = 2;
  Vec2 SERIAL(originalPlayerPos);
  Table<bool> SERIAL(belowMaxAgressorCutOff);
};
