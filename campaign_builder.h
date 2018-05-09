#pragma once

#include "campaign.h"
#include "avatar_info.h"

struct CampaignSetup;
struct VillainPlacement;
struct VillainCounts;
class TribeId;

class CampaignBuilder {
  public:
  CampaignBuilder(View*, RandomGen&, Options*, PlayerRole);
  optional<CampaignSetup> prepareCampaign(function<optional<RetiredGames>(CampaignType)>, CampaignType defaultType);
  static CampaignSetup getEmptyCampaign();

  private:
  optional<Vec2> considerStaticPlayerPos(const Campaign&);
  View* view;
  RandomGen& random;
  PlayerRole playerRole;
  Options* options;
  vector<OptionId> getSecondaryOptions(CampaignType) const;
  vector<OptionId> getPrimaryOptions() const;
  optional<string> getSiteChoiceTitle(CampaignType) const;
  vector<Campaign::VillainInfo> getMainVillains();
  vector<Campaign::VillainInfo> getLesserVillains();
  vector<Campaign::VillainInfo> getAllies();
  const char* getIntroText() const;
  void setPlayerPos(Campaign&, Vec2, ViewId playerViewId);
  AvatarInfo getAvatarInfo();
  vector<CampaignType> getAvailableTypes() const;
  VillainPlacement getVillainPlacement(const Campaign&, VillainType);
  void placeVillains(Campaign&, vector<Campaign::SiteInfo::Dweller>, const VillainPlacement&, int count);
  void placeVillains(Campaign&, const VillainCounts&, const optional<RetiredGames>&);
  OptionId getPlayerNameOptionId() const;
  OptionId getPlayerTypeOptionId() const;
  TribeId getPlayerTribeId() const;
};

struct CampaignSetup {
  Campaign campaign;
  AvatarInfo avatarInfo;
  string gameIdentifier;
  string gameDisplayName;
  bool regenerateMana;
  vector<string> introMessages;
  enum ImpVariant {
    IMPS,
    GOBLINS,
  };
};
