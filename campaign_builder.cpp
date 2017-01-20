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


optional<Vec2> CampaignBuilder::considerStaticPlayerPos(Campaign& campaign, RandomGen& random) {
  switch (campaign.type) {
    case CampaignType::CAMPAIGN: {
      Rectangle playerSpawn(campaign.sites.getBounds().topLeft(), campaign.sites.getBounds().bottomLeft() + Vec2(2, 0));
      for (Vec2 pos : random.permutation(playerSpawn.getAllSquares()))
        if (!campaign.sites[pos].blocked)
          return pos;
      return none;
    }
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
    case CampaignType::CAMPAIGN:
      return {};
    case CampaignType::ENDLESS:
      return {OptionId::LESSER_VILLAINS, OptionId::ALLIES};
    case CampaignType::FREE_PLAY:
      return {OptionId::MAIN_VILLAINS, OptionId::LESSER_VILLAINS, OptionId::ALLIES};
    case CampaignType::SINGLE_KEEPER:
      return {};
  }
}

vector<OptionId> CampaignBuilder::getPrimaryOptions() const {
  switch (playerRole) {
    case PlayerRole::KEEPER:
      return {OptionId::KEEPER_NAME, OptionId::KEEPER_TYPE};
    case PlayerRole::ADVENTURER:
      return {OptionId::ADVENTURER_NAME, OptionId::ADVENTURER_TYPE};
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
        {ViewId::AVATAR, EnemyId::KNIGHTS, "Knights", VillainType::MAIN},
        {ViewId::ELF_LORD, EnemyId::ELVES, "Elves", VillainType::MAIN},
        {ViewId::DWARF_BARON, EnemyId::DWARVES, "Dwarves", VillainType::MAIN},
        {ViewId::RED_DRAGON, EnemyId::RED_DRAGON, "Red dragon", VillainType::MAIN},
        {ViewId::ELEMENTALIST, EnemyId::ELEMENTALIST, "Elementalist", VillainType::MAIN},
        {ViewId::GREEN_DRAGON, EnemyId::GREEN_DRAGON, "Green dragon", VillainType::MAIN},
        {ViewId::LIZARDLORD, EnemyId::LIZARDMEN, "Lizardmen", VillainType::MAIN},
        {ViewId::SHAMAN, EnemyId::WARRIORS, "Warriors", VillainType::MAIN},
      };
    case PlayerRole::ADVENTURER:
      return {
        {ViewId::RED_DRAGON, EnemyId::RED_DRAGON, "Red dragon", VillainType::MAIN},
        {ViewId::GREEN_DRAGON, EnemyId::GREEN_DRAGON, "Green dragon", VillainType::MAIN},
        {ViewId::SHELOB, EnemyId::SHELOB, "Giant spider", VillainType::MAIN},
        {ViewId::HYDRA, EnemyId::HYDRA, "Hydra", VillainType::MAIN},
        {ViewId::ANT_QUEEN, EnemyId::ANTS_OPEN, "Ants", VillainType::MAIN},
        {ViewId::DARK_ELF_LORD, EnemyId::DARK_ELVES, "Dark elves", VillainType::MAIN},
        {ViewId::ORC_CAPTAIN, EnemyId::ORC_VILLAGE, "Greenskin village", VillainType::MAIN},
      };
  }
}

vector<Campaign::VillainInfo> CampaignBuilder::getLesserVillains() {
  switch (playerRole) {
    case PlayerRole::KEEPER:
      return {
        {ViewId::BANDIT, EnemyId::BANDITS, "Bandits", VillainType::LESSER},
        {ViewId::ENT, EnemyId::ENTS, "Tree spirits", VillainType::LESSER},
        {ViewId::DRIAD, EnemyId::DRIADS, "Driads", VillainType::LESSER},
        {ViewId::CYCLOPS, EnemyId::CYCLOPS, "Cyclops", VillainType::LESSER},
        {ViewId::SHELOB, EnemyId::SHELOB, "Giant spider", VillainType::LESSER},
        {ViewId::HYDRA, EnemyId::HYDRA, "Hydra", VillainType::LESSER},
        {ViewId::ANT_QUEEN, EnemyId::ANTS_OPEN, "Ants", VillainType::LESSER},
        {ViewId::ZOMBIE, EnemyId::CEMETERY, "Zombies", VillainType::LESSER},
      };
    case PlayerRole::ADVENTURER:
      return {
        {ViewId::BANDIT, EnemyId::BANDITS, "Bandits", VillainType::LESSER},
        {ViewId::CYCLOPS, EnemyId::CYCLOPS, "Cyclops", VillainType::LESSER},
        {ViewId::SHELOB, EnemyId::SHELOB, "Giant spider", VillainType::LESSER},
        {ViewId::HYDRA, EnemyId::HYDRA, "Hydra", VillainType::LESSER},
        {ViewId::ANT_QUEEN, EnemyId::ANTS_OPEN, "Ants", VillainType::LESSER},
        {ViewId::ZOMBIE, EnemyId::CEMETERY, "Zombies", VillainType::LESSER},
      };
  }
}

vector<Campaign::VillainInfo> CampaignBuilder::getAllies() {
  switch (playerRole) {
    case PlayerRole::KEEPER:
      return {
        {ViewId::UNKNOWN_MONSTER, EnemyId::OGRE_CAVE, "Unknown", VillainType::ALLY},
        {ViewId::UNKNOWN_MONSTER, EnemyId::HARPY_CAVE, "Unknown", VillainType::ALLY},
        {ViewId::UNKNOWN_MONSTER, EnemyId::SOKOBAN, "Unknown", VillainType::ALLY},
        {ViewId::DARK_ELF_LORD, EnemyId::DARK_ELVES, "Dark elves", VillainType::ALLY},
        {ViewId::GNOME_BOSS, EnemyId::GNOMES, "Gnomes", VillainType::ALLY},
        {ViewId::ORC_CAPTAIN, EnemyId::ORC_VILLAGE, "Greenskin village", VillainType::ALLY},
      };
    case PlayerRole::ADVENTURER:
      return {
        {ViewId::AVATAR, EnemyId::KNIGHTS, "Knights", VillainType::ALLY},
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
        "The world, which you see below, is made up of smaller maps. Pick one, and build your base there. "
        "There are hostile and friendly tribes around you. You have to conquer all villains marked as \"main\" "
        "to win the game. Make sure you add a few retired dungeons created by other players."
        "You can travel to other sites by creating a team and using the travel command.\n\n"
        "The highlighted tribes are in your influence zone, which means that you can currently interact with them "
        "(trade, recruit, attack or be attacked). "
        "As you conquer more enemies, your influence zone grows.\n\n";
    case PlayerRole::ADVENTURER:
      return
        "Welcome to the campaign mode! "
        "The world, which you see below, is made up of smaller maps. Pick one to start your adventure there. "
        "There are hostile and friendly tribes around you. You have to conquer all villains marked as \"main\" "
        "to win the game. Make sure you add a few retired dungeons created by other players."
        "You can travel to other sites by using the travel command.\n\n"
        "The highlighted tribes are in your influence zone, which means that you can currently travel there. "
        "As you conquer more enemies, your influence zone grows.\n\n";
   }
}


bool CampaignBuilder::isStaticPlayerPos(const Campaign& campaign) {
  switch (campaign.type) {
    case CampaignType::CAMPAIGN:
    case CampaignType::SINGLE_KEEPER:
      return true;
    default:
      return false;
  }
}

void CampaignBuilder::setPlayerPos(Campaign& campaign, Vec2 pos, const Creature* player) {
  switch (playerRole) {
    case PlayerRole::KEEPER:
      if (campaign.playerPos)
        campaign.clearSite(*campaign.playerPos);
      campaign.playerPos = pos;
      campaign.sites[*campaign.playerPos].dweller = Campaign::KeeperInfo{player->getViewObject().id()};
      break;
    case PlayerRole:: ADVENTURER:
      campaign.playerPos = pos;
      break;
  }

}

PCreature CampaignBuilder::getPlayerCreature() {
  PCreature ret;
  switch (playerRole) {
    case PlayerRole::KEEPER:
      ret = CreatureFactory::fromId(options->getCreatureId(OptionId::KEEPER_TYPE), TribeId::getKeeper());
      break;
    case PlayerRole::ADVENTURER:
      ret = CreatureFactory::fromId(options->getCreatureId(OptionId::ADVENTURER_TYPE), TribeId::getAdventurer());
      break;
  }
  ret->getName().useFullTitle();
  return ret;
}


static Table<Campaign::SiteInfo> getTerrain(RandomGen& random, Vec2 size, int numBlocked) {
  Table<Campaign::SiteInfo> ret(size, {});
  for (Vec2 v : ret.getBounds())
    ret[v].viewId.push_back(ViewId::GRASS);
  vector<Vec2> freePos = ret.getBounds().getAllSquares();
  for (int i : Range(numBlocked)) {
    Vec2 pos = random.choose(freePos);
    removeElement(freePos, pos);
    ret[pos].setBlocked();
  }
  return ret;
}

struct VillainCounts {
  int numMain;
  int numLesser;
  int numAllies;
};

static VillainCounts getVillainCounts(CampaignType type, Options* options) {
  switch (type) {
    case CampaignType::FREE_PLAY: {
      return {
        options->getIntValue(OptionId::MAIN_VILLAINS),
        options->getIntValue(OptionId::LESSER_VILLAINS),
        options->getIntValue(OptionId::ALLIES)
      };
    }
    case CampaignType::CAMPAIGN:
      return {4, 3, 2};
    case CampaignType::ENDLESS:
      return {
        0,
        options->getIntValue(OptionId::LESSER_VILLAINS),
        options->getIntValue(OptionId::ALLIES)
      };
    case CampaignType::SINGLE_KEEPER:
      return {0, 0, 0};
  }
}

CampaignBuilder::CampaignBuilder(Options* o, PlayerRole r) : playerRole(r), options(o) {
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

optional<CampaignSetup> CampaignBuilder::prepareCampaign(View* view, function<RetiredGames()> genRetired,
    RandomGen& random) {
  Vec2 size(16, 9);
  int numBlocked = 0.6 * size.x * size.y;
  Table<Campaign::SiteInfo> terrain = getTerrain(random, size, numBlocked);
  optional<RetiredGames> retiredCache;
  static optional<RetiredGames> noRetired;
  CampaignType type = CampaignType::CAMPAIGN;
  View::CampaignMenuState menuState {};
  setCountLimits(options);
  options->setChoices(OptionId::KEEPER_TYPE, {CreatureId::KEEPER, CreatureId::KEEPER_F});
  options->setChoices(OptionId::ADVENTURER_TYPE, {CreatureId::ADVENTURER, CreatureId::ADVENTURER_F});
  while (1) {
    if (type == CampaignType::FREE_PLAY && !retiredCache)
      retiredCache = genRetired();
    auto& retired = type == CampaignType::FREE_PLAY ? retiredCache : noRetired;
    string worldName = NameGenerator::get(NameGeneratorId::WORLD)->getNext();
    PCreature player = getPlayerCreature();
    auto limits = getVillainCounts(type, options);
    vector<Campaign::VillainInfo> mainVillains;
    Campaign campaign(terrain, type);
    campaign.playerRole = playerRole;
    campaign.worldName = worldName;
    int numRetired = retired ? min(limits.numMain, retired->getNumActive()) : 0;
    while (mainVillains.size() < limits.numMain - numRetired)
      append(mainVillains, random.permutation(getMainVillains()));
    mainVillains.resize(limits.numMain - numRetired);
    vector<Campaign::VillainInfo> lesserVillains;
    while (lesserVillains.size() < limits.numLesser)
      append(lesserVillains, random.permutation(getLesserVillains()));
    lesserVillains.resize(limits.numLesser);
    vector<Campaign::VillainInfo> allies;
    while (allies.size() < limits.numAllies)
      append(allies, random.permutation(getAllies()));
    allies.resize(limits.numAllies);
    vector<Vec2> freePos;
    for (Vec2 v : Rectangle(size))
      if (!campaign.sites[v].blocked)
        freePos.push_back(v);
    if (auto pos = considerStaticPlayerPos(campaign, random)) {
      setPlayerPos(campaign, *pos, player.get());
      removeElementMaybe(freePos, *pos);
    }
    for (int i : All(mainVillains)) {
      Vec2 pos = random.choose(freePos);
      removeElement(freePos, pos);
      campaign.sites[pos].dweller = mainVillains[i];
    }
    if (retired) {
      vector<RetiredGames::RetiredGame> activeGames = retired->getActiveGames();
      for (int i : Range(numRetired)) {
        Vec2 pos = random.choose(freePos);
        removeElement(freePos, pos);
        campaign.sites[pos].dweller = Campaign::RetiredInfo{activeGames[i].gameInfo, activeGames[i].fileInfo};
      }
    }
    for (int i : All(lesserVillains)) {
      Vec2 pos = random.choose(freePos);
      removeElement(freePos, pos);
      campaign.sites[pos].dweller = lesserVillains[i];
    }
    for (int i : All(allies)) {
      Vec2 pos = random.choose(freePos);
      removeElement(freePos, pos);
      campaign.sites[pos].dweller = allies[i];
    }
    while (1) {
      bool updateMap = false;
      campaign.influenceSize = options->getIntValue(OptionId::INFLUENCE_SIZE);
      campaign.refreshInfluencePos();
      bool hasRetired = type == CampaignType::FREE_PLAY;
      CampaignAction action = view->prepareCampaign({
          campaign,
          retired,
          player.get(),
          getPrimaryOptions(),
          getSecondaryOptions(type),
          getSiteChoiceTitle(type),
          getIntroText()
          }, options, menuState);
      switch (action.getId()) {
        case CampaignActionId::REROLL_MAP:
            terrain = getTerrain(random, size, numBlocked);
          FALLTHROUGH;
        case CampaignActionId::UPDATE_MAP:
            updateMap = true;
            break;
        case CampaignActionId::CHANGE_TYPE:
            type = action.get<CampaignType>();
            updateMap = true;
            break;
        case CampaignActionId::UPDATE_OPTION:
            switch (action.get<OptionId>()) {
              case OptionId::KEEPER_TYPE:
              case OptionId::ADVENTURER_TYPE:
                player = getPlayerCreature();
                if (campaign.playerPos) {
                  setPlayerPos(campaign, *campaign.playerPos, player.get());
                }
                break;
              case OptionId::KEEPER_NAME:
              case OptionId::ADVENTURER_NAME:
              case OptionId::INFLUENCE_SIZE: break;
              default: updateMap = true; break;
            }
            break;
        case CampaignActionId::CANCEL:
            return none;
        case CampaignActionId::CHOOSE_SITE:
            if (!isStaticPlayerPos(campaign))
              setPlayerPos(campaign, action.get<Vec2>(), player.get());
            break;
        case CampaignActionId::CONFIRM:
            if (!retired || numRetired > 0 || playerRole != PlayerRole::KEEPER ||
                retired->getAllGames().empty() ||
                view->yesOrNoPrompt("The imps are going to be sad if you don't add any retired dungeons. Continue?")) {
              string name = *player->getName().first();
              string gameIdentifier = name + "_" + worldName + getNewIdSuffix();
              string gameDisplayName = name + " of " + worldName;
              return CampaignSetup{campaign, std::move(player), gameIdentifier, gameDisplayName};
            }
      }
      if (updateMap)
        break;
    }
  }
}

CampaignSetup CampaignBuilder::getEmptyCampaign() {
  Campaign ret(Table<Campaign::SiteInfo>(1, 1), CampaignType::SINGLE_KEEPER);
  return CampaignSetup{ret, PCreature(nullptr), "", ""};
}
