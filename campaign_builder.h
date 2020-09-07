#pragma once

#include "campaign.h"
#include "biome_id.h"
#include "retired_games.h"

struct CampaignSetup;
struct VillainPlacement;
struct VillainCounts;
struct CampaignInfo;
class GameConfig;
struct AvatarInfo;
class ContentFactory;

RICH_ENUM(
  VillainGroup,
  EVIL_KEEPER,
  LAWFUL_KEEPER,
  EVIL_ADVENTURER,
  LAWFUL_ADVENTURER
);

using VillainsTuple = map<VillainGroup, vector<Campaign::VillainInfo>>;
using GameIntros = vector<string>;

class CampaignBuilder {
  public:
  CampaignBuilder(View*, RandomGen&, Options*, VillainsTuple, GameIntros, const AvatarInfo&);
  optional<CampaignSetup> prepareCampaign(const ContentFactory*, function<optional<RetiredGames>(CampaignType)>,
      CampaignType defaultType, string worldName);
  static CampaignSetup getEmptyCampaign();
  static CampaignSetup getWarlordCampaign(const vector<RetiredGames::RetiredGame>&,
      const string& gameName);

  private:
  optional<Vec2> considerStaticPlayerPos(const Campaign&);
  View* view;
  RandomGen& random;
  Options* options;
  VillainsTuple villains;
  GameIntros gameIntros;
  const AvatarInfo& avatarInfo;
  vector<OptionId> getCampaignOptions(CampaignType) const;
  vector<OptionId> getPrimaryOptions() const;
  vector<Campaign::VillainInfo> getVillains(TribeAlignment, VillainType);
  const char* getIntroText() const;
  void setPlayerPos(Campaign&, Vec2, ViewIdList);
  vector<CampaignType> getAvailableTypes() const;
  VillainPlacement getVillainPlacement(const Campaign&, VillainType);
  void placeVillains(Campaign&, vector<Campaign::SiteInfo::Dweller>, const VillainPlacement&, int count);
  void placeVillains(Campaign&, const VillainCounts&, const optional<RetiredGames>&, TribeAlignment);
  PlayerRole getPlayerRole() const;
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
  BiomeId startingBiome;
};
