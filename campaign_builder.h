#pragma once

#include "campaign.h"

struct CampaignSetup;

class CampaignBuilder {
  public:
  CampaignBuilder(Options*, PlayerRole);
  optional<CampaignSetup> prepareCampaign(View*, function<RetiredGames()>, RandomGen&);
  static CampaignSetup getEmptyCampaign();

  private:
  optional<Vec2> considerStaticPlayerPos(Campaign&, RandomGen&);
  bool isStaticPlayerPos(const Campaign&);
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
};

struct CampaignSetup {
  Campaign campaign;
  PCreature player;
  string gameIdentifier;
  string gameDisplayName;
};
