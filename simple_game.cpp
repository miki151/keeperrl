#include "stdafx.h"
#include "simple_game.h"
#include "creature_name.h"
#include "creature_attributes.h"
#include "collective_config.h"
#include "resource_info.h"
#include "content_factory.h"
#include "resource_counts.h"
#include "furniture.h"
#include "item.h"
#include "enemy_info.h"
#include "main_loop.h"

static PCreature getCreature(ContentFactory* factory, CreatureId id) {
  return factory->getCreatures().fromId(id, TribeId::getDarkKeeper());
}

void SimpleGame::addResourcesForLevel(int level) {
  auto resourceCounts = chooseResourceCounts(Random, factory->resources, level);
  for (auto& count : resourceCounts->elems)
    if (auto drop = factory->furniture.getData(count.type).getItemDrop()) {
      for (int patch : Range(count.countFurther + count.countStartingPos))
        for (int i : Range(Random.get(count.size)))
          for (auto& item : drop->random(factory))
            ++resources[*item->getResourceId()];
    }
}

SimpleGame::SimpleGame(ContentFactory* factory, MainLoop* mainLoop) : factory(factory), mainLoop(mainLoop) {
  auto& info = factory->keeperCreatures[0].second;
  for (auto elem : info.immigrantGroups)
    append(immigrants, factory->immigrantsData.at(elem));
  minions.push_back(getCreature(factory, info.creatureId[0]));
  auto allZLevels = factory->zLevels.at(ZLevelGroup::EVIL);
  allZLevels.append(factory->zLevels.at(ZLevelGroup::ALL));
  for (int i : Range(1, 100))
    zLevels.push_back(*chooseZLevel(Random, allZLevels, i));
  addResourcesForLevel(0);
}

bool SimpleGame::meetsRequirements(const ImmigrantInfo& info) {
  if (!info.getTraits().contains(MinionTrait::FIGHTER) || minions.size() >= maxPopulation)
    return false;
  for (auto& req : info.requirements)
    if (!req.type.visit<bool>(
          [&](TechId id) { return technology.researched.count(id); },
          [](const auto&) { return true; }
    ))
      return false;
  return true;
}

void SimpleGame::addImmigrant() {
}

bool SimpleGame::fightEnemy(EnemyId id) {
  auto& enemy = factory->enemies.at(id).settlement;
  const int numTries = 10;
  int res = mainLoop->battleTest(numTries, DirectoryPath::current().file("battle2.txt"), getWeakPointers(minions),
      {enemy.inhabitants.fighters, enemy.inhabitants.leader});
  std::cout << "Fighting " << enemy.race.value_or("unknown enemy") << ": " << res << "/" << numTries;
  return true;
}

void SimpleGame::increaseZLevel() {
  auto level = zLevels[zLevel + 1];
  auto result = level.type.visit(
        [&](const FullZLevel& l) {
          if (l.enemy)
            return fightEnemy(*l.enemy);
          return true;
        },
        [](const auto&) { return true; }
  );
  if (result)
    addResourcesForLevel(++zLevel);
}

void SimpleGame::research() {

}

vector<pair<string, SimpleGame::SimpleAction>> SimpleGame::getActions() {
  vector<pair<string, function<void()>>> ret;
  if (maxPopulation > minions.size())
    ret.push_back(make_pair("Add immigrant", [this]{ addImmigrant(); }));
  ret.push_back(make_pair("Increase z-level", [this]{ increaseZLevel(); }));
  if (dungeonLevel.numResearchAvailable() > 0)
    ret.push_back(make_pair("Research", [this]{ research(); }));
  return ret;
}

void SimpleGame::update() {
  std::cout << "Minions: ";
  for (auto& c : minions)
    std::cout << c->getName().bare() << " " << c->getBestAttack().value << ", ";
  std::cout << "\n";
  std::cout << "Researched technologies: ";
  for (auto& tech : technology.researched)
    std::cout << tech.data() << ", ";
  if (dungeonLevel.numResearchAvailable() > 0) {
    std::cout << "Available technologies (" << dungeonLevel.numResearchAvailable() << "): ";
    for (auto& tech : technology.getNextTechs())
      std::cout << tech.data() << ", ";
  }
  std::cout << "\n";
  std::cout << "Current z-level: " << zLevel << "\n";
  std::cout << "Resources: ";
  for (auto& resource : factory->resourceInfo)
    std::cout << resources[resource.first] << " " << resource.second.name << ", ";
  std::cout << "\n";
  std::cout << "Population: " << minions.size() << "/" << maxPopulation << "\n";
  std::cout << "Immigrants: ";
  for (int i : All(immigrants))
    if (meetsRequirements(immigrants[i])) {
      auto c = getCreature(factory, immigrants[i].getId(0));
      std::cout << i << ". " << c->getName().bare() << "(" << c->getBestAttack().value << "), ";
    }
  std::cout << "\n";
  auto actions = getActions();
  std::cout << "Actions:\n";
  for (int i : All(actions))
    std::cout << i << ". " << actions[i].first << "\n";
  int index;
  std::cin >> index;
  if (index >= 0 && index < actions.size())
    actions[index].second();
  else
    std::cout << "Bad index\n";
}
