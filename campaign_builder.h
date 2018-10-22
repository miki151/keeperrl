#pragma once

#include "campaign.h"
#include "avatar_info.h"

struct CampaignSetup;
struct VillainPlacement;
struct VillainCounts;
class TribeId;
class GameConfig;
struct KeeperCreatureInfo;
struct AdventurerCreatureInfo;

class CampaignBuilder {
  public:
  CampaignBuilder(View*, RandomGen&, Options*, PlayerRole, GameConfig*);
  optional<CampaignSetup> prepareCampaign(function<optional<RetiredGames>(CampaignType)>, CampaignType defaultType);
  static CampaignSetup getEmptyCampaign();

  private:
  optional<Vec2> considerStaticPlayerPos(const Campaign&);
  View* view;
  RandomGen& random;
  PlayerRole playerRole;
  Options* options;
  GameConfig* gameConfig;
  vector<OptionId> getSecondaryOptions(CampaignType) const;
  vector<OptionId> getPrimaryOptions() const;
  optional<string> getSiteChoiceTitle(CampaignType) const;
  vector<Campaign::VillainInfo> getVillains(TribeAlignment, VillainType);
  const char* getIntroText() const;
  void setPlayerPos(Campaign&, Vec2, ViewId playerViewId);
  AvatarInfo getAvatarInfo();
  vector<CampaignType> getAvailableTypes() const;
  VillainPlacement getVillainPlacement(const Campaign&, VillainType);
  void placeVillains(Campaign&, vector<Campaign::SiteInfo::Dweller>, const VillainPlacement&, int count);
  void placeVillains(Campaign&, const VillainCounts&, const optional<RetiredGames>&, TribeAlignment);
  OptionId getPlayerNameOptionId() const;
  OptionId getPlayerTypeOptionId() const;
  TribeId getPlayerTribeId(TribeAlignment) const;
  vector<CreatureId> getKeeperCreatures() const;
  vector<CreatureId> getAdventurerCreatures() const;
  KeeperCreatureInfo getKeeperCreatureInfo(CreatureId id) const;
  AdventurerCreatureInfo getAdventurerCreatureInfo(CreatureId id) const;
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
