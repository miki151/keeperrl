#pragma once

#include "campaign.h"
#include "biome_id.h"
#include "retired_games.h"

struct CampaignSetup;
struct VillainCounts;
struct CampaignInfo;
class GameConfig;
struct AvatarInfo;
class ContentFactory;

using VillainsTuple = map<VillainGroup, vector<Campaign::VillainInfo>>;
using GameIntros = vector<string>;

class CampaignBuilder {
  public:
  CampaignBuilder(View*, RandomGen&, Options*, VillainsTuple, GameIntros, const AvatarInfo&);
  optional<CampaignSetup> prepareCampaign(ContentFactory*, function<optional<RetiredGames>(CampaignType)>,
      CampaignType defaultType, string worldName);
  static CampaignSetup getEmptyCampaign();
  static CampaignSetup getWarlordCampaign(const vector<RetiredGames::RetiredGame>&,
      const string& gameName);

  private:
  View* view;
  RandomGen& random;
  Options* options;
  VillainsTuple villains;
  GameIntros gameIntros;
  const AvatarInfo& avatarInfo;
  vector<OptionId> getCampaignOptions(CampaignType) const;
  vector<OptionId> getPrimaryOptions() const;
  const char* getIntroText() const;
  void setPlayerPos(Campaign&, Vec2, ViewIdList, ContentFactory*);
  vector<CampaignType> getAvailableTypes() const;
  bool placeVillains(const ContentFactory*, Campaign&, Table<bool>&, vector<Campaign::SiteInfo::Dweller>, int count,
      Range playerDist);
  vector<Campaign::VillainInfo> getVillains(const vector<VillainGroup>&, VillainType);
  bool placeVillains(const ContentFactory*, Campaign&, const VillainCounts&, const optional<RetiredGames>&,
      const vector<VillainGroup>&);
  const vector<string>& getIntroMessages(CampaignType) const;
  void setCountLimits(const CampaignInfo&);
};

struct CampaignSetup {
  Campaign campaign;
  string gameIdentifier;
  string gameDisplayName;
  vector<string> introMessages;
  optional<ExternalEnemiesType> externalEnemies;
  EnemyAggressionLevel enemyAggressionLevel;
};
