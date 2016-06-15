#ifndef _CAMPAIGN_H
#define _CAMPAIGN_H

#include "util.h"
#include "saved_game_info.h"
#include "save_file_info.h"

class View;
class ProgressMeter;
class Options;
class RetiredGames;

RICH_ENUM(EnemyId,
  KNIGHTS,
  WARRIORS,
  DWARVES,
  ELVES,
  ELEMENTALIST,
  LIZARDMEN,
  RED_DRAGON,
  GREEN_DRAGON,

  VILLAGE,
  BANDITS,
  ENTS,
  DRIADS,
  CYCLOPS,
  SHELOB,
  HYDRA,
  ANTS,
  CEMETERY,

  DARK_ELVES,
  GNOMES,
  FRIENDLY_CAVE,
  ORC_VILLAGE,
  SOKOBAN
);

class Campaign {
  public:
  struct VillainInfo {
    ViewId SERIAL(viewId);
    EnemyId SERIAL(enemyId);
    string SERIAL(name);
    enum Type { ALLY, LESSER, MAIN};
    string getDescription() const;
    bool isEnemy() const;
    Type SERIAL(type);
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

  enum Type { ADVENTURER, KEEPER};

  const Table<SiteInfo>& getSites() const;
  void clearSite(Vec2);
  static optional<Campaign> prepareCampaign(View*, Options*, RetiredGames&&, RandomGen&, Type);
  optional<Vec2> getPlayerPos() const;
  const string& getWorldName() const;
  bool isDefeated(Vec2) const;
  void setDefeated(Vec2);
  bool canTravelTo(Vec2) const;
  bool isInInfluence(Vec2) const;
  int getNumNonEmpty() const;
  bool canEmbark(Vec2) const;
  Type getType() const;

  map<string, string> getParameters() const;
  vector<OptionId> getOptions(Options*) const;
  const char* getSiteChoiceTitle() const;
  const char* getIntroText() const;

  SERIALIZATION_DECL(Campaign);

  private:
  void refreshInfluencePos();
  Campaign(Table<SiteInfo>);
  Table<SiteInfo> SERIAL(sites);
  optional<Vec2> SERIAL(playerPos);
  string SERIAL(worldName);
  Table<bool> SERIAL(defeated);
  set<Vec2> SERIAL(influencePos);
  int SERIAL(influenceSize);
  Type SERIAL(type);
};


#endif
