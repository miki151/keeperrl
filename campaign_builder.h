#pragma once

#include "campaign.h"
#include "avatar_info.h"

struct CampaignSetup;
struct VillainPlacement;
struct VillainCounts;
class GameConfig;

using VillainsTuple = std::array<vector<Campaign::VillainInfo>, 4>;
using GameIntros = pair<vector<string>, map<CampaignType, vector<string>>>;

class CampaignBuilder {
  public:
  CampaignBuilder(View*, RandomGen&, Options*, VillainsTuple, GameIntros, const AvatarInfo&);
  optional<CampaignSetup> prepareCampaign(function<optional<RetiredGames>(CampaignType)>, CampaignType defaultType,
      string worldName);
  static CampaignSetup getEmptyCampaign();

  private:
  optional<Vec2> considerStaticPlayerPos(const Campaign&);
  View* view;
  RandomGen& random;
  Options* options;
  VillainsTuple villains;
  GameIntros gameIntros;
  const AvatarInfo& avatarInfo;
  vector<OptionId> getSecondaryOptions(CampaignType) const;
  vector<OptionId> getPrimaryOptions() const;
  optional<string> getSiteChoiceTitle(CampaignType) const;
  vector<Campaign::VillainInfo> getVillains(TribeAlignment, VillainType);
  const char* getIntroText() const;
  void setPlayerPos(Campaign&, Vec2, ViewId playerViewId);
  vector<CampaignType> getAvailableTypes() const;
  VillainPlacement getVillainPlacement(const Campaign&, VillainType);
  void placeVillains(Campaign&, vector<Campaign::SiteInfo::Dweller>, const VillainPlacement&, int count);
  void placeVillains(Campaign&, const VillainCounts&, const optional<RetiredGames>&, TribeAlignment);
  PlayerRole getPlayerRole() const;
  vector<string> getIntroMessages(CampaignType) const;
};

struct CampaignSetup {
  Campaign campaign;
  string gameIdentifier;
  string gameDisplayName;
  vector<string> introMessages;
};
