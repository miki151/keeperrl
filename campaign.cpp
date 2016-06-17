#include "stdafx.h"
#include "campaign.h"
#include "view.h"
#include "view_id.h"
#include "model.h"
#include "progress_meter.h"
#include "options.h"
#include "name_generator.h"
#include "retired_games.h"
#include "villain_type.h"
#include "enemy_factory.h"


SERIALIZATION_CONSTRUCTOR_IMPL(Campaign);

const Table<Campaign::SiteInfo>& Campaign::getSites() const {
  return sites;
}

bool Campaign::canEmbark(Vec2 pos) const {
  switch (type) {
    case ADVENTURER: return !!sites[pos].dweller;
    case KEEPER: return !sites[pos].dweller && !sites[pos].blocked;
  }
}

bool Campaign::canTravelTo(Vec2 pos) const {
  return (isInInfluence(pos) || playerPos == pos) && !sites[pos].isEmpty();
}

optional<Vec2> Campaign::getPlayerPos() const {
  return playerPos;
}

Campaign::Campaign(Table<SiteInfo> s) : sites(s), defeated(sites.getBounds(), false) {
}

const string& Campaign::getWorldName() const {
  return worldName;
}

void Campaign::clearSite(Vec2 v) {
  sites[v] = SiteInfo{};
  sites[v].viewId = {ViewId::GRASS};
}

vector<Campaign::VillainInfo> Campaign::getMainVillains() {
  switch (type) {
    case KEEPER:
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
    case ADVENTURER:
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

vector<Campaign::VillainInfo> Campaign::getLesserVillains() {
  switch (type) {
    case KEEPER:
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
    case ADVENTURER:
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

vector<Campaign::VillainInfo> Campaign::getAllies() {
  switch (type) {
    case KEEPER:
      return {
        {ViewId::UNKNOWN_MONSTER, EnemyId::FRIENDLY_CAVE, "Unknown", VillainType::ALLY},
        {ViewId::UNKNOWN_MONSTER, EnemyId::SOKOBAN, "Unknown", VillainType::ALLY},
        {ViewId::DARK_ELF_LORD, EnemyId::DARK_ELVES, "Dark elves", VillainType::ALLY},
        {ViewId::GNOME_BOSS, EnemyId::GNOMES, "Gnomes", VillainType::ALLY},
        {ViewId::ORC_CAPTAIN, EnemyId::ORC_VILLAGE, "Greenskin village", VillainType::ALLY},
      };
    case ADVENTURER:
      return {
        {ViewId::AVATAR, EnemyId::KNIGHTS, "Knights", VillainType::ALLY},
        {ViewId::ELF_LORD, EnemyId::ELVES, "Elves", VillainType::ALLY},
        {ViewId::DWARF_BARON, EnemyId::DWARVES, "Dwarves", VillainType::ALLY},
        {ViewId::LIZARDLORD, EnemyId::LIZARDMEN, "Lizardmen", VillainType::ALLY},
      };
  }
}

bool Campaign::isDefeated(Vec2 pos) const {
  return defeated[pos];
}

void Campaign::setDefeated(Vec2 pos) {
  defeated[pos] = true;
  refreshInfluencePos();
}

bool Campaign::VillainInfo::isEnemy() const {
  return type != VillainType::ALLY;
}

string Campaign::VillainInfo::getDescription() const {
  switch (type) {
    case VillainType::ALLY: return "ally";
    case VillainType::MAIN: return "main villain";
    case VillainType::LESSER: return "lesser villain";
  }
}

optional<Campaign::VillainInfo> Campaign::SiteInfo::getVillain() const {
  if (dweller)
    if (const VillainInfo* info = boost::get<VillainInfo>(&(*dweller)))
      return *info;
  return none;
}

optional<Campaign::KeeperInfo> Campaign::SiteInfo::getKeeper() const {
  if (dweller)
    if (const KeeperInfo* info = boost::get<KeeperInfo>(&(*dweller)))
      return *info;
  return none;
}

optional<Campaign::RetiredInfo> Campaign::SiteInfo::getRetired() const {
  if (dweller)
    if (const RetiredInfo* info = boost::get<RetiredInfo>(&(*dweller)))
      return *info;
  return none;
}
 
bool Campaign::SiteInfo::isEmpty() const {
  return !dweller;
}

optional<string> Campaign::SiteInfo::getDwellerDescription() const {
  if (!dweller)
    return none;
  if (const KeeperInfo* info = boost::get<KeeperInfo>(&(*dweller)))
    return string("This is your home site");
  if (const VillainInfo* info = boost::get<VillainInfo>(&(*dweller)))
    return info->name + " (" + info->getDescription() + ")";
  if (const RetiredInfo* info = boost::get<RetiredInfo>(&(*dweller)))
    return info->gameInfo.getName() + " (hostile)";
  return none;
}

optional<ViewId> Campaign::SiteInfo::getDwellerViewId() const {
  if (!dweller)
    return none;
  if (const KeeperInfo* info = boost::get<KeeperInfo>(&(*dweller)))
    return info->viewId;
  if (const VillainInfo* info = boost::get<VillainInfo>(&(*dweller)))
    return info->viewId;
  if (const RetiredInfo* info = boost::get<RetiredInfo>(&(*dweller)))
    return info->gameInfo.getViewId();
  return none;
}

bool Campaign::SiteInfo::isEnemy() const {
  return getRetired() || (getVillain() && getVillain()->isEnemy());
}

void Campaign::SiteInfo::setBlocked() {
  blocked = true;
  viewId.push_back(Random.choose(ViewId::MAP_MOUNTAIN1, ViewId::MAP_MOUNTAIN2, ViewId::MAP_MOUNTAIN3,
        ViewId::CANIF_TREE, ViewId::DECID_TREE));
}

bool Campaign::isInInfluence(Vec2 pos) const {
  return influencePos.count(pos);
}

void Campaign::refreshInfluencePos() {
  influencePos.clear();
  if (!playerPos)
    return;
  for (double r = 1; r <= max(sites.getWidth(), sites.getHeight()); r += 0.1) {
    for (Vec2 v : sites.getBounds())
      if ((sites[v].getVillain() || sites[v].getRetired()) && v.distD(*playerPos) <= r)
        influencePos.insert(v);
    int numEnemies = 0;
    for (Vec2 v : influencePos)
      if (sites[v].isEnemy() && !defeated[v])
        ++numEnemies;
    if (numEnemies >= influenceSize)
      break;
  }
}

int Campaign::getNumNonEmpty() const {
  int ret = 0;
  for (Vec2 v : sites.getBounds())
    if (!sites[v].isEmpty())
      ++ret;
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

optional<Campaign> Campaign::prepareCampaign(View* view, Options* options, RetiredGames&& retired,
    RandomGen& random, Type type) {
  Vec2 size(16, 9);
  int numBlocked = 0.6 * size.x * size.y;
  Table<SiteInfo> terrain = getTerrain(random, size, numBlocked);
  string worldName = NameGenerator::get(NameGeneratorId::WORLD)->getNext();
  options->setDefaultString(OptionId::KEEPER_NAME, NameGenerator::get(NameGeneratorId::FIRST)->getNext());
  while (1) {
    //options->setLimits(OptionId::RETIRED_VILLAINS, 0, min<int>(retired.size(), 4)); 
#ifdef RELEASE
    options->setLimits(OptionId::MAIN_VILLAINS, 1, 4); 
#else
    options->setLimits(OptionId::MAIN_VILLAINS, 0, 4); 
#endif
    options->setLimits(OptionId::LESSER_VILLAINS, 0, 6); 
    options->setLimits(OptionId::ALLIES, 0, 4); 
    options->setLimits(OptionId::INFLUENCE_SIZE, 3, 6); 
    int numRetired = min(retired.getNumActive(), options->getIntValue(OptionId::MAIN_VILLAINS));
    int numMain = options->getIntValue(OptionId::MAIN_VILLAINS) - numRetired;
    int numLesser = options->getIntValue(OptionId::LESSER_VILLAINS);
    int numAllies = options->getIntValue(OptionId::ALLIES);
    vector<VillainInfo> mainVillains;
    Campaign campaign(terrain);
    while (mainVillains.size() < numMain)
      append(mainVillains, random.permutation(campaign.getMainVillains()));
    mainVillains.resize(numMain);
    vector<VillainInfo> lesserVillains;
    while (lesserVillains.size() < numLesser)
      append(lesserVillains, random.permutation(campaign.getLesserVillains()));
    lesserVillains.resize(numLesser);
    vector<VillainInfo> allies;
    while (allies.size() < numAllies)
      append(allies, random.permutation(campaign.getAllies()));
    allies.resize(numAllies);
    campaign.type = type;
    campaign.worldName = worldName;
    vector<Vec2> freePos;
    for (Vec2 v : Rectangle(size))
      if (!campaign.sites[v].blocked)
        freePos.push_back(v);
    for (int i : All(mainVillains)) {
      Vec2 pos = random.choose(freePos);
      removeElement(freePos, pos);
      campaign.sites[pos].dweller = mainVillains[i];
    }
    vector<RetiredGames::RetiredGame> activeGames = retired.getActiveGames();
    for (int i : Range(numRetired)) {
      Vec2 pos = random.choose(freePos);
      removeElement(freePos, pos);
      campaign.sites[pos].dweller = RetiredInfo{activeGames[i].gameInfo, activeGames[i].fileInfo};
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
      CampaignAction action = view->prepareCampaign(campaign, options, retired);
      switch (action.getId()) {
        case CampaignActionId::REROLL_MAP:
            terrain = getTerrain(random, size, numBlocked);
            worldName = NameGenerator::get(NameGeneratorId::WORLD)->getNext();
            options->setDefaultString(OptionId::KEEPER_NAME, NameGenerator::get(NameGeneratorId::FIRST)->getNext());
        case CampaignActionId::UPDATE_MAP:
            updateMap = true; break;
        case CampaignActionId::UPDATE_OPTION:
            switch (action.get<OptionId>()) {
              case OptionId::KEEPER_NAME:
              case OptionId::INFLUENCE_SIZE: break;
              default: updateMap = true; break;
            }
            break;
        case CampaignActionId::CANCEL:
            return none;
        case CampaignActionId::CHOOSE_SITE:
            switch (type) {
              case KEEPER:
                if (campaign.playerPos)
                  campaign.clearSite(*campaign.playerPos);
                campaign.playerPos = action.get<Vec2>();
                campaign.sites[*campaign.playerPos].dweller = KeeperInfo{ViewId::KEEPER};
                break;
              case ADVENTURER:
                campaign.playerPos = action.get<Vec2>();
                break;
            }
            break;
        case CampaignActionId::CONFIRM:
            if (numRetired > 0 || type != KEEPER ||
                retired.getAllGames().empty() ||
                view->yesOrNoPrompt("Imps are going to be sad if you don't add any retired dungeons. Continue?"))
              return campaign;
      }
      if (updateMap)
        break;
    }
  }
}

map<string, string> Campaign::getParameters() const {
  int numMain = 0;
  int numLesser = 0;
  int numAlly = 0;
  int numRetired = 0;
  for (Vec2 v : sites.getBounds())
    if (sites[v].getRetired())
      ++numRetired;
    else if (auto villain = sites[v].getVillain())
      switch (villain->type) {
        case VillainType::ALLY: ++numAlly; break;
        case VillainType::MAIN: ++numMain; break;
        case VillainType::LESSER: ++numLesser; break;
      }
  return {
    {"main", toString(numMain)},
    {"lesser", toString(numLesser)},
    {"allies", toString(numAlly)},
    {"retired", toString(numRetired)},
    {"type", type == KEEPER ? "keeper" : "adventurer"}
  };
}

Campaign::Type Campaign::getType() const {
  return type;
}

vector<OptionId> Campaign::getOptions(Options* options) const {
  switch (type) {
    case KEEPER: return options->getOptions(OptionSet::CAMPAIGN);
    case ADVENTURER: return options->getOptions(OptionSet::ADVENTURER_CAMPAIGN);
  }
}

const char* Campaign::getSiteChoiceTitle() const {
   switch (type) {
    case KEEPER: return "Choose the location of your base:";
    case ADVENTURER: return "Choose a location to start your adventure:";
  }
}

const char* Campaign::getIntroText() const {
   switch (type) {
    case KEEPER:
      return
        "Welcome to the campaign mode! "
        "The world, which you see below, is made up of smaller maps. Pick one, and build your base there. "
        "There are hostile and friendly tribes around you. You have to conquer all villains marked as \"main\" "
        "to win the game. Make sure you add a few retired dungeons created by other players."
        "You can travel to other sites by creating a team and using the travel command.\n\n"
        "The highlighted tribes are in your influence zone, which means that you can currently interact with them "
        "(trade, recruit, attack or be attacked). "
        "As you conquer more enemies, your influence zone grows.\n\n";
    case ADVENTURER:
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
