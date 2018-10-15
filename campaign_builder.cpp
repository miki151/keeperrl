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
      if (playerRole == PlayerRole::KEEPER)
        return {OptionId::MAIN_VILLAINS, OptionId::LESSER_VILLAINS, OptionId::ALLIES, OptionId::GENERATE_MANA};
      else
        return {OptionId::MAIN_VILLAINS, OptionId::LESSER_VILLAINS, OptionId::ALLIES};
    case CampaignType::SINGLE_KEEPER:
      return {};
  }
}

OptionId CampaignBuilder::getPlayerNameOptionId() const {
  switch (playerRole) {
    case PlayerRole::KEEPER:
      return OptionId::KEEPER_NAME;
    case PlayerRole::ADVENTURER:
      return OptionId::ADVENTURER_NAME;
  }
}

OptionId CampaignBuilder::getPlayerTypeOptionId() const {
  switch (playerRole) {
    case PlayerRole::KEEPER:
      return OptionId::KEEPER_TYPE;
    case PlayerRole::ADVENTURER:
      return OptionId::ADVENTURER_TYPE;
  }
}

TribeId CampaignBuilder::getPlayerTribeId() const {
  switch (playerRole) {
    case PlayerRole::KEEPER:
      return TribeId::getKeeper();
    case PlayerRole::ADVENTURER:
      return TribeId::getAdventurer();
  }
}

vector<OptionId> CampaignBuilder::getPrimaryOptions() const {
  return {getPlayerTypeOptionId(), getPlayerNameOptionId()};
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
  switch (playerRole) {
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
      switch (playerRole) {
        case PlayerRole::KEEPER: return "Choose the location of your base:"_s;
        case PlayerRole::ADVENTURER: return "Choose a location to start your adventure:"_s;
      }
      break;
    default:
      return none;
  }
}

vector<Campaign::VillainInfo> CampaignBuilder::getMainVillains() {
  switch (playerRole) {
    case PlayerRole::KEEPER:
      return {
        {ViewId::DUKE, EnemyId::KNIGHTS, "Knights", VillainType::MAIN},
        {ViewId::ELF_LORD, EnemyId::ELVES, "Elves", VillainType::MAIN},
        {ViewId::DWARF_BARON, EnemyId::DWARVES, "Dwarves", VillainType::MAIN},
        {ViewId::RED_DRAGON, EnemyId::RED_DRAGON, "Red dragon", VillainType::MAIN},
        {ViewId::ELEMENTALIST, EnemyId::ELEMENTALIST, "Elementalist", VillainType::MAIN},
        {ViewId::GREEN_DRAGON, EnemyId::GREEN_DRAGON, "Green dragon", VillainType::MAIN},
        {ViewId::LIZARDLORD, EnemyId::LIZARDMEN, "Lizardmen", VillainType::MAIN},
        {ViewId::SHAMAN, EnemyId::WARRIORS, "Warriors", VillainType::MAIN},
        {ViewId::DEMON_LORD, EnemyId::DEMON_DEN, "Demon den", VillainType::MAIN},
      };
    case PlayerRole::ADVENTURER:
      return {
        {ViewId::RED_DRAGON, EnemyId::RED_DRAGON, "Red dragon", VillainType::MAIN},
        {ViewId::GREEN_DRAGON, EnemyId::GREEN_DRAGON, "Green dragon", VillainType::MAIN},
        {ViewId::SHELOB, EnemyId::SHELOB, "Giant spider", VillainType::MAIN},
        {ViewId::ANT_QUEEN, EnemyId::ANTS_OPEN, "Ants", VillainType::MAIN},
        {ViewId::DARK_ELF_LORD, EnemyId::DARK_ELVES, "Dark elves", VillainType::MAIN},
        {ViewId::ORC_CAPTAIN, EnemyId::ORC_VILLAGE, "Greenskin village", VillainType::MAIN},
        {ViewId::DEMON_LORD, EnemyId::DEMON_DEN, "Demon den", VillainType::MAIN},
      };
  }
}

vector<Campaign::VillainInfo> CampaignBuilder::getLesserVillains() {
  switch (playerRole) {
    case PlayerRole::KEEPER:
      return {
        {ViewId::ENT, EnemyId::ENTS, "Tree spirits", VillainType::LESSER},
        {ViewId::DRIAD, EnemyId::DRIADS, "Driads", VillainType::LESSER},
        {ViewId::CYCLOPS, EnemyId::CYCLOPS, "Cyclops", VillainType::LESSER},
        {ViewId::SHELOB, EnemyId::SHELOB, "Giant spider", VillainType::LESSER},
        {ViewId::HYDRA, EnemyId::HYDRA, "Hydra", VillainType::LESSER},
        {ViewId::UNICORN, EnemyId::UNICORN_HERD, "Unicorn herd", VillainType::LESSER},
        {ViewId::ANT_QUEEN, EnemyId::ANTS_OPEN, "Ants", VillainType::LESSER},
        {ViewId::ZOMBIE, EnemyId::CEMETERY, "Zombies", VillainType::LESSER},
      };
    case PlayerRole::ADVENTURER:
      return {
        {ViewId::BANDIT, EnemyId::BANDITS, "Bandits", VillainType::LESSER},
        {ViewId::CYCLOPS, EnemyId::CYCLOPS, "Cyclops", VillainType::LESSER},
        {ViewId::HYDRA, EnemyId::HYDRA, "Hydra", VillainType::LESSER},
        {ViewId::ZOMBIE, EnemyId::CEMETERY, "Zombies", VillainType::LESSER},
        {ViewId::OGRE, EnemyId::OGRE_CAVE, "Ogres", VillainType::LESSER},
        {ViewId::HARPY, EnemyId::HARPY_CAVE, "Harpies", VillainType::LESSER},
      };
  }
}

vector<Campaign::VillainInfo> CampaignBuilder::getAllies() {
  switch (playerRole) {
    case PlayerRole::KEEPER:
      return {
//        {ViewId::UNKNOWN_MONSTER, EnemyId::OGRE_CAVE, "Unknown", VillainType::ALLY},
//        {ViewId::UNKNOWN_MONSTER, EnemyId::HARPY_CAVE, "Unknown", VillainType::ALLY},
        {ViewId::UNKNOWN_MONSTER, EnemyId::SOKOBAN, "Unknown", VillainType::ALLY},
        {ViewId::DARK_ELF_LORD, EnemyId::DARK_ELVES, "Dark elves", VillainType::ALLY},
        {ViewId::GNOME_BOSS, EnemyId::GNOMES, "Gnomes", VillainType::ALLY},
        {ViewId::ORC_CAPTAIN, EnemyId::ORC_VILLAGE, "Greenskin village", VillainType::ALLY},
      };
    case PlayerRole::ADVENTURER:
      return {
        {ViewId::DUKE, EnemyId::KNIGHTS, "Knights", VillainType::ALLY},
        {ViewId::ELF_LORD, EnemyId::ELVES, "Elves", VillainType::ALLY},
        {ViewId::DWARF_BARON, EnemyId::DWARVES, "Dwarves", VillainType::ALLY},
        {ViewId::LIZARDLORD, EnemyId::LIZARDMEN, "Lizardmen", VillainType::ALLY},
      };
  }
}

const char* CampaignBuilder::getIntroText() const {
   switch (playerRole) {
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
  switch (playerRole) {
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

static AvatarInfo::ImmigrationVariant getImmigrationVariant(CreatureId id) {
  switch (id) {
    case CreatureId::KEEPER_KNIGHT:
    case CreatureId::KEEPER_KNIGHT_F:
      return AvatarInfo::DARK_KNIGHT;
    case CreatureId::KEEPER_MAGE:
    case CreatureId::KEEPER_MAGE_F:
      return AvatarInfo::DARK_MAGE;
    case CreatureId::DUKE_PLAYER:
      return AvatarInfo::WHITE_KNIGHT;
    default:
      FATAL << "Immigration variant not handled for CreatureId = " << EnumInfo<CreatureId>::getString(id);
      return {};
  }
}

AvatarInfo CampaignBuilder::getAvatarInfo() {
  auto id = options->getCreatureId(getPlayerTypeOptionId());
  PCreature ret = CreatureFactory::fromId(id, getPlayerTribeId());
  auto name = options->getStringValue(getPlayerNameOptionId());
  if (!name.empty())
    ret->getName().setFirst(name);
  ret->getName().useFullTitle();
  return {std::move(ret), getImmigrationVariant(id)};
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

CampaignBuilder::CampaignBuilder(View* v, RandomGen& rand, Options* o, PlayerRole r)
    : view(v), random(rand), playerRole(r), options(o) {
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
    const optional<RetiredGames>& retired) {
  int numRetired = retired ? min(retired->getNumActive(), min(counts.numMain, counts.maxRetired)) : 0;
  placeVillains(campaign, shuffle(random, getMainVillains()), getVillainPlacement(campaign, VillainType::MAIN),
      counts.numMain - numRetired);
  placeVillains(campaign, shuffle(random, getLesserVillains()), getVillainPlacement(campaign, VillainType::LESSER),
      counts.numLesser);
  placeVillains(campaign, shuffle(random, getAllies()), getVillainPlacement(campaign, VillainType::ALLY),
      counts.numAllies);
  if (retired) {
    placeVillains(campaign, retired->getActiveGames().transform(
        [](const RetiredGames::RetiredGame& game) -> Dweller {
          return Campaign::RetiredInfo{game.gameInfo, game.fileInfo};
        }), getVillainPlacement(campaign, VillainType::MAIN), numRetired);
  }
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
    "Welcome to KeeperRL Alpha25! This patch was released on July 30, 2018. "
    "Many new gameplay features have arrived, "
    "so if you are a returning player, we encourage you to check out the "
    "change log at www.keeperrl.com.\n \n"
    "If this is your first time playing KeeperRL, remember to start with the tutorial!"
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
  options->setChoices(OptionId::KEEPER_TYPE, {
      CreatureId::KEEPER_MAGE,
      CreatureId::KEEPER_MAGE_F,
      CreatureId::KEEPER_KNIGHT,
      CreatureId::KEEPER_KNIGHT_F,
      CreatureId::DUKE_PLAYER
  });
  options->setChoices(OptionId::ADVENTURER_TYPE, {CreatureId::ADVENTURER, CreatureId::ADVENTURER_F});
  while (1) {
    AvatarInfo avatarInfo = getAvatarInfo();
    Campaign campaign(terrain, type, playerRole, NameGenerator::get(NameGeneratorId::WORLD)->getNext());
    if (auto pos = considerStaticPlayerPos(campaign)) {
      campaign.clearSite(*pos);
      setPlayerPos(campaign, *pos, avatarInfo.playerCreature->getViewObject().id());
    }
    placeVillains(campaign, getVillainCounts(type, options), retired);
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
              case OptionId::KEEPER_NAME:
              case OptionId::ADVENTURER_NAME:
              case OptionId::KEEPER_TYPE:
              case OptionId::ADVENTURER_TYPE:
                avatarInfo = getAvatarInfo();
                if (campaign.playerPos) {
                  setPlayerPos(campaign, *campaign.playerPos, avatarInfo.playerCreature->getViewObject().id());
                }
                break;
              case OptionId::GENERATE_MANA:
              case OptionId::INFLUENCE_SIZE: break;
              default: updateMap = true; break;
            }
            break;
        case CampaignActionId::CANCEL:
            return none;
        case CampaignActionId::CHOOSE_SITE:
            if (!considerStaticPlayerPos(campaign))
              setPlayerPos(campaign, action.get<Vec2>(), avatarInfo.playerCreature->getViewObject().id());
            break;
        case CampaignActionId::CONFIRM:
            if (!retired || retired->getNumActive() > 0 || playerRole != PlayerRole::KEEPER ||
                retired->getAllGames().empty() ||
                view->yesOrNoPrompt("The imps are going to be sad if you don't add any retired dungeons. Continue?")) {
              string name = *avatarInfo.playerCreature->getName().first();
              string gameIdentifier = name + "_" + campaign.worldName + getNewIdSuffix();
              string gameDisplayName = name + " of " + campaign.worldName;
              return CampaignSetup{campaign, std::move(avatarInfo), gameIdentifier, gameDisplayName,
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
  return CampaignSetup{ret, {PCreature(nullptr), AvatarInfo::DARK_MAGE}, "", "", false, {}};
}
