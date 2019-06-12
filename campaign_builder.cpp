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
#include "tribe_alignment.h"
#include "external_enemies_type.h"

optional<Vec2> CampaignBuilder::considerStaticPlayerPos(const Campaign& campaign) {
  if (campaign.getPlayerRole() == PlayerRole::ADVENTURER && options->getIntValue(OptionId::ALLIES) == 0)
    return none;
  return campaign.sites.getBounds().middle();
}

void CampaignBuilder::setCountLimits() {
#ifdef RELEASE
  options->setLimits(OptionId::MAIN_VILLAINS, 1, 4);
#else
  options->setLimits(OptionId::MAIN_VILLAINS, 0, 4);
#endif
  options->setLimits(OptionId::LESSER_VILLAINS, 0, 6);
  options->setLimits(OptionId::ALLIES, getPlayerRole() == PlayerRole::ADVENTURER ? 1 : 0, 4);
  options->setLimits(OptionId::INFLUENCE_SIZE, 3, 6);
}

vector<OptionId> CampaignBuilder::getCampaignOptions(CampaignType type) const {
  switch (type) {
    case CampaignType::QUICK_MAP:
      return {OptionId::LESSER_VILLAINS, OptionId::ALLIES};
    case CampaignType::FREE_PLAY:
      if (getPlayerRole() == PlayerRole::ADVENTURER)
        return {OptionId::MAIN_VILLAINS, OptionId::LESSER_VILLAINS, OptionId::ALLIES};
      else
        return {OptionId::MAIN_VILLAINS, OptionId::LESSER_VILLAINS, OptionId::ALLIES, OptionId::ENDLESS_ENEMIES};
    case CampaignType::SINGLE_KEEPER:
      return {};
  }
}

static vector<string> getCampaignTypeDescription(CampaignType type) {
  switch (type) {
    case CampaignType::FREE_PLAY:
      return {
        "the main game mode"
      };
    case CampaignType::SINGLE_KEEPER:
      return {
        "everyone on one big map",
        "retiring not possible"
      };
    default:
      return {};
  }
}

vector<CampaignType> CampaignBuilder::getAvailableTypes() const {
  switch (getPlayerRole()) {
    case PlayerRole::KEEPER:
      return {
        CampaignType::FREE_PLAY,
        CampaignType::SINGLE_KEEPER,
#ifndef RELEASE
        CampaignType::QUICK_MAP,
#endif
      };
    case PlayerRole::ADVENTURER:
      return {
        CampaignType::FREE_PLAY,
      };
  }
}

static vector<Campaign::VillainInfo> filter(vector<Campaign::VillainInfo> v, VillainType type) {
  return v.filter([type](const auto& elem){ return elem.type == type; });
}

vector<Campaign::VillainInfo> CampaignBuilder::getVillains(TribeAlignment tribeAlignment, VillainType type) {
  switch (getPlayerRole()) {
    case PlayerRole::KEEPER:
      switch (tribeAlignment) {
        case TribeAlignment::EVIL:
          return filter(villains[0], type);
        case TribeAlignment::LAWFUL:
          return filter(villains[1], type);
      }
    case PlayerRole::ADVENTURER:
      switch (tribeAlignment) {
        case TribeAlignment::EVIL:
          return filter(villains[2], type);
        case TribeAlignment::LAWFUL:
          return filter(villains[3], type);
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
    ret[v].viewId.push_back(ViewId("grass"));
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
    case CampaignType::QUICK_MAP:
    case CampaignType::SINGLE_KEEPER:
      return {0, 0, 0, 0};
  }
}

CampaignBuilder::CampaignBuilder(View* v, RandomGen& rand, Options* o, VillainsTuple villains, GameIntros intros, const AvatarInfo& a)
    : view(v), random(rand), options(o), villains(std::move(villains)), gameIntros(intros), avatarInfo(a) {
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
  for (int i = 0; villains.size() < count; ++i)
    villains.push_back(villains[i]);
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
    case CampaignType::FREE_PLAY:
      switch (type) {
        case VillainType::LESSER:
          ret.xPredicate = [](int x) { return x >= 5 && x < 12; };
          break;
        case VillainType::MAIN:
          ret.xPredicate = [](int x) { return (x >= 1 && x < 5) || (x >= 12 && x < 16) ; };
          break;
        case VillainType::ALLY:
          if (campaign.getPlayerRole() == PlayerRole::ADVENTURER)
            ret.firstLocation = considerStaticPlayerPos(campaign);
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

vector<string> CampaignBuilder::getIntroMessages(CampaignType type) const {
  return concat(gameIntros.first, getReferenceMaybe(gameIntros.second, type).value_or(vector<string>()));
}

static optional<ExternalEnemiesType> getExternalEnemies(Options* options) {
  auto v = options->getIntValue(OptionId::ENDLESS_ENEMIES);
  if (v == 0)
    return none;
  if (v == 1)
    return ExternalEnemiesType::FROM_START;
  if (v == 2)
    return ExternalEnemiesType::AFTER_WINNING;
  FATAL << "Bad endless enemies value " << v;
  fail();
}

optional<CampaignSetup> CampaignBuilder::prepareCampaign(function<optional<RetiredGames>(CampaignType)> genRetired,
    CampaignType type, string worldName) {
  Vec2 size(17, 9);
  int numBlocked = 0.6 * size.x * size.y;
  Table<Campaign::SiteInfo> terrain = getTerrain(random, size, numBlocked);
  auto retired = genRetired(type);
  View::CampaignMenuState menuState { true, false, false};
  string searchString;
  const auto playerRole = getPlayerRole();
  options->setChoices(OptionId::ENDLESS_ENEMIES, {"none", "from the start", "after winning"});
  while (1) {
    setCountLimits();
    Campaign campaign(terrain, type, playerRole, worldName);
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
              getCampaignOptions(type),
              getIntroText(),
              getAvailableTypes().transform([](CampaignType t) -> View::CampaignOptions::CampaignTypeInfo {
                  return {t, getCampaignTypeDescription(t)};}),
              getMenuWarning(type),
              searchString}, options, menuState);
      switch (action.getId()) {
        case CampaignActionId::SEARCH_RETIRED:
          searchString = action.get<string>();
          break;
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
            case OptionId::ENDLESS_ENEMIES:
              break;
            default:
              updateMap = true;
              break;
          }
          break;
        case CampaignActionId::CANCEL:
          return none;
        case CampaignActionId::CONFIRM:
          if (!retired || retired->getNumActive() > 0 || playerRole != PlayerRole::KEEPER ||
              retired->getAllGames().empty() ||
              view->yesOrNoPrompt("The imps are going to be sad if you don't add any retired dungeons. Continue?")) {
            string name = avatarInfo.playerCreature->getName().firstOrBare();
            string gameIdentifier = name + "_" + campaign.worldName + getNewIdSuffix();
            string gameDisplayName = name + " of " + campaign.worldName;
            return CampaignSetup{campaign, gameIdentifier, gameDisplayName,
                getIntroMessages(type), getExternalEnemies(options)};
          }
      }
      if (updateMap)
        break;
    }
  }
}

CampaignSetup CampaignBuilder::getEmptyCampaign() {
  Campaign ret(Table<Campaign::SiteInfo>(1, 1), CampaignType::SINGLE_KEEPER, PlayerRole::KEEPER, "");
  return CampaignSetup{ret, "", "", {}, none};
}
