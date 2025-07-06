#include "stdafx.h"
#include "campaign_builder.h"
#include "options.h"
#include "campaign_type.h"
#include "util.h"
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
#include "enemy_aggression_level.h"
#include "campaign_info.h"
#include "content_factory.h"
#include "avatar_info.h"
#include "layout_canvas.h"
#include "layout_generator.h"
#include "enemy_info.h"
#include "villain_group.h"

void CampaignBuilder::setCountLimits(const CampaignInfo& info) {
  int minMainVillains =
#ifdef RELEASE
  1;
#else
  0;
#endif
  options->setLimits(OptionId::MAIN_VILLAINS, Range(minMainVillains, info.maxMainVillains + 1));
  options->setLimits(OptionId::LESSER_VILLAINS, Range(1, info.maxLesserVillains + 1));
  options->setLimits(OptionId::MINOR_VILLAINS, Range(0, info.maxMinorVillains + 1));
  options->setLimits(OptionId::ALLIES, Range(0, info.maxAllies + 1));
}

vector<OptionId> CampaignBuilder::getCampaignOptions(CampaignType type) const {
  switch (type) {
    case CampaignType::QUICK_MAP:
      return {OptionId::LESSER_VILLAINS, OptionId::ALLIES};
    case CampaignType::FREE_PLAY:
      return {
        OptionId::MAIN_VILLAINS,
        OptionId::LESSER_VILLAINS,
        OptionId::MINOR_VILLAINS,
        OptionId::ALLIES,
        OptionId::EXP_INCREASE,
        OptionId::ENDLESS_ENEMIES,
        OptionId::ENEMY_AGGRESSION,
      };
  }
}

vector<CampaignType> CampaignBuilder::getAvailableTypes() const {
  return {
    CampaignType::FREE_PLAY,
#ifndef RELEASE
    CampaignType::QUICK_MAP,
#endif
  };
}

void CampaignBuilder::setPlayerPos(Campaign& campaign, Vec2 pos, ViewIdList playerViewId, ContentFactory* f) {
  campaign.sites[campaign.playerPos].dweller.reset();
  campaign.playerPos = pos;
  campaign.sites[campaign.playerPos].dweller =
      Campaign::SiteInfo::Dweller(Campaign::KeeperInfo{playerViewId,
          avatarInfo.playerCreature->getTribeId()});
  campaign.updateInhabitants(f);
}

static Table<Campaign::SiteInfo> getTerrain(RandomGen& random, const ContentFactory* factory,
    RandomLayoutId worldMapId, Vec2 size) {
  unordered_set<Token> allViewIds;
  for (auto& def : factory->tilePaths.definitions)
    allViewIds.insert(def.viewId.data());
  LayoutCanvas::Map map{Table<vector<Token>>(Rectangle(Vec2(0, 0), size))};
  LayoutCanvas canvas{map.elems.getBounds(), &map};
  bool generated = false;
  for (int i : Range(20))
    if (factory->randomLayouts.at(worldMapId).make(canvas, random)) {
      generated = true;
      break;
    }
  CHECK(generated) << "Failed to generate world map";
  Table<Campaign::SiteInfo> ret(size, {});
  for (Vec2 v : ret.getBounds())
    for (auto& token : map.elems[v]) {
      if (token == "blocked")
        ret[v].blocked = true;
      else if (allViewIds.count(token))
        ret[v].viewId.push_back(ViewId(token.data()));
      else
        for (auto& biome : factory->biomeInfo)
          if (token == biome.first.data())
            ret[v].biome = biome.first;
    }
  return ret;
}

struct VillainCounts {
  int numMain;
  int numLesser;
  int numMinor;
  int numAllies;
  int maxRetired;
};

static VillainCounts getVillainCounts(CampaignType type, Options* options) {
  switch (type) {
    case CampaignType::FREE_PLAY: {
      return {
        options->getIntValue(OptionId::MAIN_VILLAINS),
        options->getIntValue(OptionId::LESSER_VILLAINS),
        options->getIntValue(OptionId::MINOR_VILLAINS),
        options->getIntValue(OptionId::ALLIES),
        10
      };
    }
    case CampaignType::QUICK_MAP:
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

constexpr auto mapMargin = 10;

bool CampaignBuilder::placeVillains(const ContentFactory* contentFactory, Campaign& campaign, Table<bool>& blocked,
    vector<Campaign::SiteInfo::Dweller> villains, int count, Range playerDist) {
  if (villains.empty())
    return true;
  for (int i = 0; villains.size() < count; ++i)
    villains.push_back(villains[i]);
  CHECK(count >= 0);
  if (villains.size() > count)
    villains.resize(count);
  for (int i : All(villains)) {
    auto biome = [&] {
      return villains[i].match(
          [&](const Campaign::VillainInfo& info) { return contentFactory->enemies.at(info.enemyId).getBiome(); },
          [](const Campaign::RetiredInfo& info) -> optional<BiomeId> { return none; },
          [](const Campaign::KeeperInfo& info) -> optional<BiomeId> { return none; }
      );
    }();
    auto placed = [&] {
      for (Vec2 v : random.permutation(campaign.sites.getBounds().minusMargin(mapMargin).getAllSquares())) {
        if (!blocked[v] && playerDist.contains((int) v.distD(campaign.getPlayerPos())) &&
            !!campaign.sites[v].biome && (!biome || campaign.sites[v].biome == biome)) {
          campaign.sites[v].dweller = villains[i];
          for (auto v2 : Rectangle::centered(v, 5).intersection(blocked.getBounds()))
            blocked[v2] = true;
          return true;
        }
      }
      return false;
    }();
    if (!placed)
      return false;
  }
  return true;
}

using Dweller = Campaign::SiteInfo::Dweller;

vector<Dweller> shuffle(RandomGen& random, vector<Campaign::VillainInfo> v) {
  int numAlways = 0;
  for (auto& elem : v)
    if (elem.alwaysPresent) {
      swap(v[numAlways], elem);
      ++numAlways;
    }
  random.shuffle(v.begin() + numAlways, v.end());
  return v.transform([](auto& t) { return Dweller(t); });
}

vector<Campaign::VillainInfo> CampaignBuilder::getVillains(const vector<VillainGroup>& groups, VillainType type) {
  vector<Campaign::VillainInfo> ret;
  for (auto& group : groups)
    for (auto& elem : villains[group])
      if (elem.type == type)
        ret.push_back(elem);
  return ret;
}

bool CampaignBuilder::placeVillains(const ContentFactory* contentFactory, Campaign& campaign,
    const VillainCounts& counts, const optional<RetiredGames>& retired, const vector<VillainGroup>& villainGroups) {
  int retiredLimit = counts.numMain;
  auto allMainVillains = getVillains(villainGroups, VillainType::MAIN);
  auto regularMainVillains = allMainVillains.filter([](auto& v) { return !v.alwaysPresent; });
  auto endGameVillains = allMainVillains.filter([](auto& v) { return v.alwaysPresent; })
      .transform([](auto& t) { return Dweller(t); });
  const int numAlwaysPresent = endGameVillains.size();
  if (retired) {
    int numRetired = min(retired->getNumActive(), min(retiredLimit, counts.maxRetired));
    endGameVillains.append(retired->getActiveGames().transform(
          [](const RetiredGames::RetiredGame& game) -> Dweller {
            return Campaign::RetiredInfo{game.gameInfo, game.fileInfo};
          }).getPrefix(numRetired));
  }
  Table<bool> blocked(campaign.sites.getBounds(), false);
  for (auto v : campaign.sites.getBounds())
    if (campaign.sites[v].blocked)
      blocked[v] = true;
  for (auto v : Rectangle::centered(campaign.playerPos, 5).intersection(blocked.getBounds()))
    blocked[v] = true;
  auto initialRadius = contentFactory->campaignInfo.initialRadius;
  if (!placeVillains(contentFactory, campaign, blocked, shuffle(random, regularMainVillains),
      max(0, counts.numMain - numAlwaysPresent), Range(0, 1000)))
    return false;
  auto allLesser = shuffle(random, getVillains(villainGroups, VillainType::LESSER));
  if (!placeVillains(contentFactory, campaign, blocked, allLesser.getPrefix(3), min(3, counts.numLesser), Range(1, initialRadius)))
    return false;
  if (allLesser.size() > 3 && counts.numLesser > 3)
    if (!placeVillains(contentFactory, campaign, blocked, allLesser.getSubsequence(3), counts.numLesser - 3, Range(initialRadius + 2, 1000)))
      return false;
  if (!placeVillains(contentFactory, campaign, blocked, shuffle(random, getVillains(villainGroups, VillainType::MINOR)),
          counts.numMinor, Range(0, 1000)) ||
      !placeVillains(contentFactory, campaign, blocked, shuffle(random, getVillains(villainGroups, VillainType::ALLY)),
          counts.numAllies, Range(0, 1000)))
    return false;
  if (!placeVillains(contentFactory, campaign, blocked, endGameVillains, endGameVillains.size(),
      Range((campaign.getSites().getBounds().width() - mapMargin) / 2 - 5, 1000)))
    return false;
  return true;
}

static bool autoConfirm(CampaignType type) {
  switch (type) {
    case CampaignType::QUICK_MAP:
      return true;
    default:
      return false;
  }
}

const vector<TString>& CampaignBuilder::getIntroMessages(CampaignType type) const {
  return gameIntros;
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

static EnemyAggressionLevel getAggressionLevel(Options* options) {
  auto v = options->getIntValue(OptionId::ENEMY_AGGRESSION);
  if (v == 0)
    return EnemyAggressionLevel::NONE;
  if (v == 1)
    return EnemyAggressionLevel::MODERATE;
  if (v == 2)
    return EnemyAggressionLevel::EXTREME;
  FATAL << "Bad enemy aggression value " << v;
  fail();
}

static int getExpIncrease(Options* options) {
  auto v = options->getIntValue(OptionId::EXP_INCREASE);
  if (v == 0)
    return 2;
  if (v == 1)
    return 3;
  if (v == 2)
    return 5;
  FATAL << "Bad exp increase value " << v;
  fail();
}

optional<CampaignSetup> CampaignBuilder::prepareCampaign(ContentFactory* contentFactory,
    function<optional<RetiredGames>(CampaignType)> genRetired,
    CampaignType type, string worldName) {
  auto& campaignInfo = contentFactory->campaignInfo;
  Vec2 size = campaignInfo.size;
  int numBlocked = 0.6 * size.x * size.y;
  auto retired = genRetired(type);
  View::CampaignMenuState menuState { true, CampaignMenuIndex{CampaignMenuElems::None{}} };
  options->setChoices(OptionId::ENDLESS_ENEMIES, {
      view->translate(TStringId("ENDLESS_ENEMIES_NONE")),
      view->translate(TStringId("ENDLESS_ENEMIES_FROM_START")),
      view->translate(TStringId("ENDLESS_ENEMIES_AFTER_WINNING"))});
  options->setChoices(OptionId::ENEMY_AGGRESSION, {
      view->translate(TStringId("ENEMY_AGGRESSION_NONE")),
      view->translate(TStringId("ENEMY_AGGRESSION_MODERATE")),
      view->translate(TStringId("ENEMY_AGGRESSION_EXTREME"))});
  options->setChoices(OptionId::EXP_INCREASE, {
      view->translate(TStringId("EXP_INCREASE_MILD")),
      view->translate(TStringId("EXP_INCREASE_NORMAL")),
      view->translate(TStringId("EXP_INCREASE_EXTREME"))});
  int worldMapIndex = 0;
  auto worldMapId = [&] {
    return contentFactory->worldMaps[worldMapIndex].layout;
  };
  int failedPlaceVillains = 0;
  while (1) {
    setCountLimits(campaignInfo);
    Table<Campaign::SiteInfo> terrain = getTerrain(random, contentFactory, worldMapId(), size);
    Campaign campaign(terrain, type, worldName, getExpIncrease(options));
    campaign.mapZoom = campaignInfo.mapZoom;
    campaign.minimapZoom = campaignInfo.minimapZoom;
    for (auto pos : Random.permutation(campaign.getSites().getBounds()
        .minusMargin(campaignInfo.initialRadius + 1).getAllSquares())) {
      if (campaign.isGoodStartPos(pos) || type == CampaignType::QUICK_MAP) {
        setPlayerPos(campaign, pos, avatarInfo.playerCreature->getMaxViewIdUpgrade(), contentFactory);
        campaign.originalPlayerPos = pos;
        break;
      }
    }
    const auto villainCounts = getVillainCounts(type, options);
    if (!placeVillains(contentFactory, campaign, villainCounts, retired, avatarInfo.villainGroups)) {
      if (++failedPlaceVillains > 300)
        USER_FATAL << "Failed to place all villains on the world map";
      continue;
    }
    failedPlaceVillains = 0;
    campaign.updateInhabitants(contentFactory);
    while (1) {
      bool updateMap = false;
      campaign.refreshInfluencePos(contentFactory);
      CampaignAction action = autoConfirm(type) ? CampaignActionId::CONFIRM
          : view->prepareCampaign(View::CampaignOptions {
              campaign,
              (retired && type == CampaignType::FREE_PLAY) ? optional<RetiredGames&>(*retired) : none,
              getCampaignOptions(type),
              TStringId("CAMPAIGN_HELP_TEXT"),
              contentFactory->biomeInfo.at(*campaign.getSites()[campaign.getPlayerPos()].biome).name,
              contentFactory->worldMaps.transform([](auto& elem) { return elem.name; }),
              worldMapIndex
            },
              menuState);
      switch (action.getId()) {
        case CampaignActionId::REROLL_MAP:
          terrain = getTerrain(random, contentFactory, worldMapId(), size);
          updateMap = true;
          break;
        case CampaignActionId::UPDATE_MAP:
          updateMap = true;
          break;
        case CampaignActionId::SET_POSITION:
          setPlayerPos(campaign, action.get<Vec2>(), avatarInfo.playerCreature->getMaxViewIdUpgrade(),
              contentFactory);
          break;
        case CampaignActionId::CHANGE_WORLD_MAP:
          worldMapIndex = action.get<int>();
          retired = genRetired(type);
          updateMap = true;
          break;
        case CampaignActionId::UPDATE_OPTION:
          switch (action.get<OptionId>()) {
            case OptionId::PLAYER_NAME:
            case OptionId::ENDLESS_ENEMIES:
            case OptionId::ENEMY_AGGRESSION:
              break;
            default:
              updateMap = true;
              break;
          }
          break;
        case CampaignActionId::CANCEL:
          return none;
        case CampaignActionId::CONFIRM:
          if (!retired || retired->getNumActive() > 0 || retired->getAllGames().empty() ||
              view->yesOrNoPrompt(TStringId("CONFIRM_NO_RETIRED_DUNGEONS"))) {
            string gameIdentifier;
            TString gameDisplayName;
            if (avatarInfo.chosenBaseName) {
              auto name = avatarInfo.playerCreature->getName().firstOrBare();
              gameIdentifier = *avatarInfo.chosenBaseName + "_" + campaign.worldName + getNewIdSuffix();
              gameDisplayName = capitalFirst(TSentence("OF",
                  avatarInfo.playerCreature->getName().plural(), TString(*avatarInfo.chosenBaseName)));
            } else {
              string name = avatarInfo.playerCreature->getName().first().value_or(
                  view->translate(avatarInfo.playerCreature->getName().bare()));
              gameIdentifier = name + "_" + campaign.worldName + getNewIdSuffix();
              gameDisplayName = TSentence("OF", TString(name), TString(campaign.worldName));
            }
            gameIdentifier = stripFilename(std::move(gameIdentifier));
            auto aggressionLevel = avatarInfo.creatureInfo.enemyAggression
                ? getAggressionLevel(options)
                : EnemyAggressionLevel::NONE;
            campaign.refreshMaxAggressorCutOff();
            return CampaignSetup{campaign, gameIdentifier, gameDisplayName,
                getIntroMessages(type), getExternalEnemies(options), aggressionLevel};
          }
      }
      if (updateMap)
        break;
    }
  }
}

CampaignSetup CampaignBuilder::getEmptyCampaign() {
  Campaign ret(Table<Campaign::SiteInfo>(1, 1), CampaignType::QUICK_MAP, "", 1);
  return CampaignSetup{ret, "", TString(), {}, none, EnemyAggressionLevel::MODERATE};
}
