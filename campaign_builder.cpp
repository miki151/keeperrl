#include "stdafx.h"
#include "campaign_builder.h"
#include "options.h"
#include "campaign_type.h"
#include "player_role.h"
#include "view.h"
#include "enemy_factory.h"
#include "villain_type.h"
#include "creature.h"
#include "creature_name.h"
#include "retired_games.h"
#include "view_object.h"
#include "name_generator.h"
#include "creature_factory.h"
#include "game_config.h"
#include "tribe_alignment.h"

optional<Vec2> CampaignBuilder::considerStaticPlayerPos(const Campaign& campaign) {
  switch (campaign.type) {
    case CampaignType::CAMPAIGN:
    case CampaignType::QUICK_MAP:
    case CampaignType::SINGLE_KEEPER:
      return campaign.sites.getBounds().middle();
    default:
      return none;
  }
}

static void setCountLimits(Options* options) {
#ifdef RELEASE
  options->setLimits(OptionId::MAIN_VILLAINS, 1, 4);
#else
  options->setLimits(OptionId::MAIN_VILLAINS, 0, 4);
#endif
  options->setLimits(OptionId::LESSER_VILLAINS, 0, 6);
  options->setLimits(OptionId::ALLIES, 0, 4);
  options->setLimits(OptionId::INFLUENCE_SIZE, 3, 6);
}

vector<OptionId> CampaignBuilder::getSecondaryOptions(CampaignType type) const {
  switch (type) {
    case CampaignType::QUICK_MAP:
    case CampaignType::CAMPAIGN:
      return {};
    case CampaignType::ENDLESS:
      return {OptionId::LESSER_VILLAINS, OptionId::ALLIES};
    case CampaignType::FREE_PLAY:
      if (getPlayerRole() == PlayerRole::KEEPER)
        return {OptionId::MAIN_VILLAINS, OptionId::LESSER_VILLAINS, OptionId::ALLIES, OptionId::GENERATE_MANA};
      else
        return {OptionId::MAIN_VILLAINS, OptionId::LESSER_VILLAINS, OptionId::ALLIES};
    case CampaignType::SINGLE_KEEPER:
      return {};
  }
}

vector<OptionId> CampaignBuilder::getPrimaryOptions() const {
  return {OptionId::PLAYER_NAME};
}

static vector<string> getCampaignTypeDescription(CampaignType type) {
  switch (type) {
    case CampaignType::CAMPAIGN:
      return {
        "the main competitive mode"
      };
    case CampaignType::FREE_PLAY:
      return {
        "retired dungeons of other players",
        "custom starting point and villains",
        "highscores not recorded",
      };
    case CampaignType::SINGLE_KEEPER:
      return {
        "everyone on one big map",
        "retiring not possible"
      };
    case CampaignType::ENDLESS:
      return {
        "conquest not mandatory",
        "recurring enemy waves",
        "survive as long as possible"
      };
    default:
      return {};
  }
}

vector<CampaignType> CampaignBuilder::getAvailableTypes() const {
  switch (getPlayerRole()) {
    case PlayerRole::KEEPER:
      return {
        CampaignType::CAMPAIGN,
        CampaignType::FREE_PLAY,
        CampaignType::SINGLE_KEEPER,
        CampaignType::ENDLESS,
#ifndef RELEASE
        CampaignType::QUICK_MAP,
#endif
      };
    case PlayerRole::ADVENTURER:
      return {
        CampaignType::CAMPAIGN,
        CampaignType::FREE_PLAY,
      };
  }
}

optional<string> CampaignBuilder::getSiteChoiceTitle(CampaignType type) const {
  switch (type) {
    case CampaignType::FREE_PLAY:
    case CampaignType::ENDLESS:
      switch (getPlayerRole()) {
        case PlayerRole::KEEPER: return "Choose the location of your base:"_s;
        case PlayerRole::ADVENTURER: return "Choose a location to start your adventure:"_s;
      }
      break;
    default:
      return none;
  }
}

using VillainsTuple = std::array<vector<Campaign::VillainInfo>, 4>;

static vector<Campaign::VillainInfo> filter(vector<Campaign::VillainInfo> v, VillainType type) {
  return v.filter([type](const auto& elem){ return elem.type == type; });
}

vector<Campaign::VillainInfo> CampaignBuilder::getVillains(TribeAlignment tribeAlignment, VillainType type) {
  VillainsTuple config;
  while (1) {
    if (auto error = gameConfig->readObject(config, GameConfigId::CAMPAIGN_VILLAINS))
      view->presentText("Error reading campaign villains definition", *error);
    else
      break;
  }
  switch (getPlayerRole()) {
    case PlayerRole::KEEPER:
      switch (tribeAlignment) {
        case TribeAlignment::EVIL:
          return filter(config[0], type);
        case TribeAlignment::LAWFUL:
          return filter(config[1], type);
      }
    case PlayerRole::ADVENTURER:
      switch (tribeAlignment) {
        case TribeAlignment::EVIL:
          return filter(config[2], type);
        case TribeAlignment::LAWFUL:
          return filter(config[3], type);
      }
  }
}

const char* CampaignBuilder::getIntroText() const {
   switch (getPlayerRole()) {
    case PlayerRole::KEEPER:
      return
        "Welcome to the campaign mode! "
        "The world, which you see below, is made up of smaller maps. You will build your base on one of them. "
        "There are hostile and friendly tribes around you. You have to conquer all villains marked as \"main\" "
        "to win the game."
        "You can travel to other sites by creating a team and using the travel command.\n\n"
        "The highlighted tribes are in your influence zone, which means that you can currently interact with them "
        "(trade, recruit, attack or be attacked). "
        "As you conquer more enemies, your influence zone will increase.\n\n";
    case PlayerRole::ADVENTURER:
      return
        "Welcome to the campaign mode! "
        "The world, which you see below, is made up of smaller maps. Your adventure will start on one of them. "
        "There are hostile and friendly tribes around you. You have to conquer all villains marked as \"main\" "
        "to win the game."
        "You can travel to other sites by using the travel command.\n\n"
        "The highlighted tribes are in your influence zone, which means that you can currently travel there. "
        "As you conquer more enemies, your influence zone will increase.\n\n";
   }
}

void CampaignBuilder::setPlayerPos(Campaign& campaign, Vec2 pos, ViewId playerViewId) {
  switch (getPlayerRole()) {
    case PlayerRole::KEEPER:
      if (campaign.playerPos)
        campaign.clearSite(*campaign.playerPos);
      campaign.playerPos = pos;
      campaign.sites[*campaign.playerPos].dweller =
          Campaign::SiteInfo::Dweller(Campaign::KeeperInfo{playerViewId});
      break;
    case PlayerRole:: ADVENTURER:
      campaign.playerPos = pos;
      break;
  }
}

static Table<Campaign::SiteInfo> getTerrain(RandomGen& random, Vec2 size, int numBlocked) {
  Table<Campaign::SiteInfo> ret(size, {});
  for (Vec2 v : ret.getBounds())
    ret[v].viewId.push_back(ViewId::GRASS);
  vector<Vec2> freePos = ret.getBounds().getAllSquares();
  for (int i : Range(numBlocked)) {
    Vec2 pos = random.choose(freePos);
    freePos.removeElement(pos);
    ret[pos].setBlocked();
  }
  return ret;
}

struct VillainCounts {
  int numMain;
  int numLesser;
  int numAllies;
  int maxRetired;
};

static VillainCounts getVillainCounts(CampaignType type, Options* options) {
  switch (type) {
    case CampaignType::FREE_PLAY: {
      return {
        options->getIntValue(OptionId::MAIN_VILLAINS),
        options->getIntValue(OptionId::LESSER_VILLAINS),
        options->getIntValue(OptionId::ALLIES),
        10000
      };
    }
    case CampaignType::CAMPAIGN:
      return {4, 6, 2, 1};
    case CampaignType::ENDLESS:
      return {
        0,
        options->getIntValue(OptionId::LESSER_VILLAINS),
        options->getIntValue(OptionId::ALLIES),
        0
      };
    case CampaignType::QUICK_MAP:
    case CampaignType::SINGLE_KEEPER:
      return {0, 0, 0, 0};
  }
}

CampaignBuilder::CampaignBuilder(View* v, RandomGen& rand, Options* o, GameConfig* gameConfig, const AvatarInfo& a)
    : view(v), random(rand), options(o), gameConfig(gameConfig), avatarInfo(a) {
}

static string getNewIdSuffix() {
  vector<char> chars;
  for (char c : Range(128))
    if (isalnum(c))
      chars.push_back(c);
  string ret;
  for (int i : Range(4))
    ret += Random.choose(chars);
  return ret;
}

struct VillainPlacement {
  function<bool(int)> xPredicate;
  optional<Vec2> firstLocation;
};

void CampaignBuilder::placeVillains(Campaign& campaign, vector<Campaign::SiteInfo::Dweller> villains,
    const VillainPlacement& placement, int count) {
  random.shuffle(villains.begin(), villains.end());
  if (villains.size() > count)
    villains.resize(count);
  vector<Vec2> freePos;
  for (Vec2 v : campaign.sites.getBounds())
    if (!campaign.sites[v].blocked && campaign.sites[v].isEmpty() && placement.xPredicate(v.x))
      freePos.push_back(v);
  freePos = random.permutation(freePos);
  if (auto& pos = placement.firstLocation)
    freePos = concat({*pos}, freePos);
  for (int i : All(villains))
    campaign.sites[freePos[i]].dweller = villains[i];
}

VillainPlacement CampaignBuilder::getVillainPlacement(const Campaign& campaign, VillainType type) {
  VillainPlacement ret { [&campaign](int x) { return campaign.sites.getBounds().getXRange().contains(x);}, none };
  switch (campaign.getType()) {
    case CampaignType::CAMPAIGN:
      switch (type) {
        case VillainType::LESSER:
          ret.xPredicate = [](int x) { return x >= 5 && x < 12; };
          break;
        case VillainType::MAIN:
          ret.xPredicate = [](int x) { return (x >= 1 && x < 5) || (x >= 12 && x < 16) ; };
          break;
        case VillainType::ALLY:
          if (campaign.getPlayerRole() == PlayerRole::ADVENTURER)
            ret.firstLocation = *considerStaticPlayerPos(campaign);
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
  return ret;
}

using Dweller = Campaign::SiteInfo::Dweller;

template <typename T>
vector<Dweller> shuffle(RandomGen& random, vector<T> v) {
  random.shuffle(v.begin(), v.end());
  return v.transform([](const T& t) { return Dweller(t); });
}

void CampaignBuilder::placeVillains(Campaign& campaign, const VillainCounts& counts,
    const optional<RetiredGames>& retired, TribeAlignment tribeAlignment) {
  int numRetired = retired ? min(retired->getNumActive(), min(counts.numMain, counts.maxRetired)) : 0;
  placeVillains(campaign, shuffle(random, getVillains(tribeAlignment, VillainType::MAIN)),
      getVillainPlacement(campaign, VillainType::MAIN), counts.numMain - numRetired);
  placeVillains(campaign, shuffle(random, getVillains(tribeAlignment, VillainType::LESSER)),
      getVillainPlacement(campaign, VillainType::LESSER), counts.numLesser);
  placeVillains(campaign, shuffle(random, getVillains(tribeAlignment, VillainType::ALLY)),
      getVillainPlacement(campaign, VillainType::ALLY), counts.numAllies);
  if (retired) {
    placeVillains(campaign, retired->getActiveGames().transform(
        [](const RetiredGames::RetiredGame& game) -> Dweller {
          return Campaign::RetiredInfo{game.gameInfo, game.fileInfo};
        }), getVillainPlacement(campaign, VillainType::MAIN), numRetired);
  }
}

PlayerRole CampaignBuilder::getPlayerRole() const {
  return avatarInfo.creatureInfo.contains<KeeperCreatureInfo>() ? PlayerRole::KEEPER : PlayerRole::ADVENTURER;
}

static optional<View::CampaignOptions::WarningType> getMenuWarning(CampaignType type) {
  switch (type) {
    case CampaignType::CAMPAIGN:
      return View::CampaignOptions::OTHER_MODES;
    case CampaignType::SINGLE_KEEPER:
      return View::CampaignOptions::NO_RETIRE;
    default:
      return none;
  }
}

static bool autoConfirm(CampaignType type) {
  switch (type) {
    case CampaignType::QUICK_MAP:
      return true;
    default:
      return false;
  }
}

static vector<string> getIntroMessages(CampaignType type, string worldName) {
  vector<string> ret = {
    "Welcome to KeeperRL Alpha 26! This patch hasn't been officially released yet might be very unstable! "
/*    "Many new gameplay features have arrived, "
    "so if you are a returning player, we encourage you to check out the "
    "change log at www.keeperrl.com.\n \n"*/
    "If this is your first time playing KeeperRL, please play the official Alpha 25 build."
  };
  if (type == CampaignType::ENDLESS)
    ret.push_back(
        "Welcome to the endless mode! Your task here is to survive as long as possible, while "
        "defending your dungeon from incoming enemy waves. The enemies don't come from any specific place and "
        "will just appear at the edge of the map. You will get mana for defeating each wave. "
        "Note that there are also traditional enemy villages scattered around and they may also attack you.\n \n"
        "The endless mode is a completely new feature and we are very interested in your feedback on how "
        "it can be developed further. Please drop by on the forums at keeperrl.com or on Steam and let us know!"
    );

  return ret;
}

optional<CampaignSetup> CampaignBuilder::prepareCampaign(function<optional<RetiredGames>(CampaignType)> genRetired,
    CampaignType type) {
  Vec2 size(17, 9);
  int numBlocked = 0.6 * size.x * size.y;
  Table<Campaign::SiteInfo> terrain = getTerrain(random, size, numBlocked);
  auto retired = genRetired(type);
  View::CampaignMenuState menuState { true, false};
  setCountLimits(options);
  const auto playerRole = getPlayerRole();
  while (1) {
    Campaign campaign(terrain, type, playerRole, NameGenerator::get(NameGeneratorId::WORLD)->getNext());
    if (auto pos = considerStaticPlayerPos(campaign)) {
      campaign.clearSite(*pos);
      setPlayerPos(campaign, *pos, avatarInfo.playerCreature->getMaxViewIdUpgrade());
    }
    placeVillains(campaign, getVillainCounts(type, options), retired, avatarInfo.tribeAlignment);
    while (1) {
      bool updateMap = false;
      campaign.influenceSize = options->getIntValue(OptionId::INFLUENCE_SIZE);
      campaign.refreshInfluencePos();
      CampaignAction action = autoConfirm(type) ? CampaignActionId::CONFIRM
          : view->prepareCampaign({
              campaign,
              (retired && type == CampaignType::FREE_PLAY) ? optional<RetiredGames&>(*retired) : none,
              avatarInfo.playerCreature.get(),
              getPrimaryOptions(),
              getSecondaryOptions(type),
              getSiteChoiceTitle(type),
              getIntroText(),
              getAvailableTypes().transform([](CampaignType t) -> View::CampaignOptions::CampaignTypeInfo {
                  return {t, getCampaignTypeDescription(t)};}),
              getMenuWarning(type)
              }, options, menuState);
      switch (action.getId()) {
        case CampaignActionId::REROLL_MAP:
          terrain = getTerrain(random, size, numBlocked);
          updateMap = true;
          break;
        case CampaignActionId::UPDATE_MAP:
          updateMap = true;
          break;
        case CampaignActionId::CHANGE_TYPE:
          type = action.get<CampaignType>();
          retired = genRetired(type);
          updateMap = true;
          break;
        case CampaignActionId::UPDATE_OPTION:
          switch (action.get<OptionId>()) {
            case OptionId::PLAYER_NAME:
            case OptionId::GENERATE_MANA:
            case OptionId::INFLUENCE_SIZE:
              break;
            default:
              updateMap = true;
              break;
          }
          break;
        case CampaignActionId::CANCEL:
          return none;
        case CampaignActionId::CHOOSE_SITE:
          if (!considerStaticPlayerPos(campaign))
            setPlayerPos(campaign, action.get<Vec2>(), avatarInfo.playerCreature->getMaxViewIdUpgrade());
          break;
        case CampaignActionId::CONFIRM:
          if (!retired || retired->getNumActive() > 0 || playerRole != PlayerRole::KEEPER ||
              retired->getAllGames().empty() ||
              view->yesOrNoPrompt("The imps are going to be sad if you don't add any retired dungeons. Continue?")) {
            string name = *avatarInfo.playerCreature->getName().first();
            string gameIdentifier = name + "_" + campaign.worldName + getNewIdSuffix();
            string gameDisplayName = name + " of " + campaign.worldName;
            return CampaignSetup{campaign, gameIdentifier, gameDisplayName,
                options->getBoolValue(OptionId::GENERATE_MANA) &&
                getSecondaryOptions(type).contains(OptionId::GENERATE_MANA),
                getIntroMessages(type, campaign.getWorldName())};
          }
      }
      if (updateMap)
        break;
    }
  }
}

CampaignSetup CampaignBuilder::getEmptyCampaign() {
  Campaign ret(Table<Campaign::SiteInfo>(1, 1), CampaignType::SINGLE_KEEPER, PlayerRole::KEEPER, "");
  return CampaignSetup{ret, "", "", false, {}};
}
