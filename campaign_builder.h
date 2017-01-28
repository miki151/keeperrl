#pragma once

#include "campaign.h"

struct CampaignSetup;

class CampaignBuilder {
  public:
  CampaignBuilder(View*, RandomGen&, Options*, PlayerRole);
  optional<CampaignSetup> prepareCampaign(function<RetiredGames()>, CampaignType defaultType);
  static CampaignSetup getEmptyCampaign();

  private:
  optional<Vec2> considerStaticPlayerPos(Campaign&, RandomGen&);
  bool isStaticPlayerPos(const Campaign&);
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
  void setPlayerPos(Campaign&, Vec2, const Creature* player);
  PCreature getPlayerCreature();
  vector<CampaignType> getAvailableTypes() const;
  void placeVillains(Campaign&, const vector<Campaign::SiteInfo::Dweller>&);
  vector<Campaign::SiteInfo::Dweller> getRandomVillains(const vector<Campaign::VillainInfo>& source, int count);
};

struct CampaignSetup {
  Campaign campaign;
  PCreature player;
  string gameIdentifier;
  string gameDisplayName;
};
