#include "stdafx.h"
#include "content_factory.h"
#include "name_generator.h"
#include "game_config.h"
#include "creature_inventory.h"
#include "creature_attributes.h"
#include "furniture.h"
#include "player_role.h"
#include "tribe_alignment.h"
#include "item.h"

SERIALIZE_DEF(ContentFactory, creatures, furniture, resources, zLevels, tilePaths, enemies, itemFactory, workshopGroups, immigrantsData, buildInfo, villains, gameIntros, playerCreatures, technology)

static bool isZLevel(const vector<ZLevelInfo>& levels, int depth) {
  for (auto& l : levels)
    if (l.minDepth.value_or(-100) <= depth && l.maxDepth.value_or(1000000) >= depth)
      return true;
  return false;
}

bool areResourceCounts(const vector<ResourceDistribution>& resources, int depth) {
  for (auto& l : resources)
    if (l.minDepth.value_or(-100) <= depth && l.maxDepth.value_or(1000000) >= depth)
      return true;
  return false;
}

optional<string> ContentFactory::readCreatureFactory(NameGenerator nameGenerator, const GameConfig* config) {
  map<CreatureId, CreatureAttributes> attributes;
  map<CreatureId, CreatureInventory> inventory;
  map<string, SpellSchool> spellSchools;
  vector<Spell> spells;
  if (auto res = config->readObject(attributes, GameConfigId::CREATURE_ATTRIBUTES))
    return *res;
  vector<pair<vector<CreatureId>, CreatureInventory>> input;
  if (auto res = config->readObject(input, GameConfigId::CREATURE_INVENTORY))
    return *res;
  for (auto& elem : input)
    for (auto& id : elem.first) {
      if (inventory.count(id))
        return "CreatureId appears more than once: "_s + id;
      inventory.insert(make_pair(id, elem.second));
    }
  if (auto res = config->readObject(spells, GameConfigId::SPELLS))
    return *res;
  if (auto res = config->readObject(spellSchools, GameConfigId::SPELL_SCHOOLS))
    return *res;
  auto hasSpell = [&](const string& name) {
    return std::any_of(spells.begin(), spells.end(), [&name](const Spell& sp) { return  sp.getId() == name; });
  };
  for (auto& elem : spellSchools)
    for (auto& s : elem.second.spells)
      if (!hasSpell(s.first))
        return ": unknown spell: " + s.first + " in school " + elem.first;
  for (auto& elem : attributes) {
    for (auto& school : elem.second.spellSchools)
      if (!spellSchools.count(school))
        return elem.first + ": unknown spell school: " + school;
    for (auto& spell : elem.second.spells)
      if (!hasSpell(spell))
        return elem.first + ": unknown spell: " + spell;
  }
  creatures = CreatureFactory(std::move(nameGenerator), std::move(attributes), std::move(inventory),
      std::move(spellSchools), std::move(spells));
  return none;
}

optional<string> ContentFactory::readFurnitureFactory(const GameConfig* config) {
  FurnitureType::startContentIdGeneration();
  map<FurnitureType, Furniture> elems;
  if (auto res = config->readObject(elems, GameConfigId::FURNITURE))
    return *res;
  map<FurnitureType, OwnerPointer<Furniture>> furnitureDefs;
  map<FurnitureListId, FurnitureList> furnitureLists;
  for (auto& elem : elems) {
    elem.second.setType(elem.first);
    furnitureDefs.insert(make_pair(elem.first, makeOwner<Furniture>(elem.second)));
  }
  FurnitureType::validateContentIds();
  FurnitureListId::startContentIdGeneration();
  if (auto res = config->readObject(furnitureLists, GameConfigId::FURNITURE_LISTS))
    return *res;
  FurnitureListId::validateContentIds();
  furniture = FurnitureFactory(std::move(furnitureDefs), std::move(furnitureLists));
  return none;
}
static optional<string> checkGroupCounts(const map<string, vector<ImmigrantInfo>>& immigrants) {
  for (auto& group : immigrants)
    for (auto& elem : group.second)
      if (elem.getGroupSize().isEmpty())
        return "Bad immigrant group size: " + toString(elem.getGroupSize()) +
            ". Lower bound is inclusive, upper bound exclusive.";
  return none;
}

optional<string> ContentFactory::readVillainsTuple(const GameConfig* gameConfig) {
  if (auto error = gameConfig->readObject(villains, GameConfigId::CAMPAIGN_VILLAINS))
    return "Error reading campaign villains definition"_s + *error;
  auto has = [](vector<Campaign::VillainInfo> v, VillainType type) {
    return std::any_of(v.begin(), v.end(), [type](const auto& elem){ return elem.type == type; });
  };
  for (auto villainType : {VillainType::ALLY, VillainType::MAIN, VillainType::LESSER})
    for (auto role : ENUM_ALL(PlayerRole))
      for (auto alignment : ENUM_ALL(TribeAlignment)) {
        int index = [&] {
          switch (role) {
            case PlayerRole::KEEPER:
              switch (alignment) {
                case TribeAlignment::EVIL:
                  return 0;
                case TribeAlignment::LAWFUL:
                  return 1;
              }
            case PlayerRole::ADVENTURER:
              switch (alignment) {
                case TribeAlignment::EVIL:
                  return 2;
                case TribeAlignment::LAWFUL:
                  return 3;
              }
          }
        }();
        if (!has(villains[index], villainType))
          return "Empty " + EnumInfo<VillainType>::getString(villainType) + " villain list for alignment: "
                    + EnumInfo<TribeAlignment>::getString(alignment);
      }
  return none;
}

optional<string> ContentFactory::readPlayerCreatures(const GameConfig* config) {
  if (auto error = config->readObject(playerCreatures, GameConfigId::PLAYER_CREATURES))
    return "Error reading player creature definitions"_s + *error;
  if (playerCreatures.first.empty() || playerCreatures.second.empty() || playerCreatures.first.size() > 10 ||
      playerCreatures.second.size() > 10)
    return "Keeper and adventurer lists must each contain between 1 and 10 entries."_s;
  for (auto& keeperInfo : playerCreatures.first) {
    bool hotkeys[128] = {0};
    vector<BuildInfo> buildInfoTmp;
    set<string> allDataGroups;
    for (auto& group : buildInfo) {
      allDataGroups.insert(group.first);
      if (keeperInfo.buildingGroups.contains(group.first))
        buildInfoTmp.append(group.second);
    }
    for (auto& tech : keeperInfo.technology)
      if (!technology.techs.count(tech))
        return "Technology not found: " + tech;
    for (auto& tech : keeperInfo.initialTech)
      if (!technology.techs.count(tech) || !keeperInfo.technology.contains(tech))
        return "Technology not found: " + tech;
    for (auto& info : buildInfoTmp)
      for (auto& requirement : info.requirements)
        if (auto tech = requirement.getReferenceMaybe<TechId>())
          if (!keeperInfo.technology.contains(*tech))
            return "Technology prerequisite \"" + *tech + "\" of build item \"" + info.name + "\" is not available";
    WorkshopArray merged;
    set<string> allWorkshopGroups;
    for (auto& group : workshopGroups) {
      allWorkshopGroups.insert(group.first);
      if (keeperInfo.workshopGroups.contains(group.first))
        for (int i : Range(EnumInfo<WorkshopType>::size))
          merged[i].append(group.second[i]);
    }
    for (auto& elem : merged)
      for (auto& item : elem)
        if (item.tech && !technology.techs.count(*item.tech))
          return "Technology prerequisite \"" + *item.tech + "\" of workshop item \"" + item.item.get()->getName()
              + "\" is not available";
    for (auto elem : keeperInfo.immigrantGroups)
      if (!immigrantsData.count(elem))
        return "Undefined immigrant group: \"" + elem + "\"";
    for (auto& group : keeperInfo.buildingGroups)
      if (!allDataGroups.count(group))
        return "Building menu group \"" + group + "\" not found";
    for (auto& info : buildInfoTmp) {
      if (info.hotkey != '\0') {
        if (hotkeys[int(info.hotkey)])
          return "Hotkey \'" + string(1, info.hotkey) + "\' is used more than once in building menu";
        hotkeys[int(info.hotkey)] = true;
      }
    }
  }
  return none;
}

optional<string> ContentFactory::readData(NameGenerator nameGenerator, const GameConfig* config) {
  if (auto error = config->readObject(technology, GameConfigId::TECHNOLOGY))
    return *error;
  for (auto& tech : technology.techs)
    for (auto& preq : tech.second.prerequisites)
      if (!technology.techs.count(preq))
        return "Technology prerequisite \"" + preq + "\" of \"" + tech.first + "\" is not available";
  if (auto error = config->readObject(workshopGroups, GameConfigId::WORKSHOPS_MENU))
    return *error;
  if (auto error = config->readObject(immigrantsData, GameConfigId::IMMIGRATION))
    return *error;
  if (auto error = checkGroupCounts(immigrantsData))
    return *error;
  if (auto error = config->readObject(buildInfo, GameConfigId::BUILD_MENU))
    return *error;
  if (auto error = readVillainsTuple(config))
    return *error;
  if (auto error = config->readObject(gameIntros, GameConfigId::GAME_INTRO_TEXT))
    return *error;
  if (auto error = readPlayerCreatures(config))
    return *error;
  EnemyId::startContentIdGeneration();
  if (auto res = config->readObject(zLevels, GameConfigId::Z_LEVELS))
    return *res;
  if (auto res = config->readObject(resources, GameConfigId::RESOURCE_COUNTS))
    return *res;
  if (auto res = config->readObject(enemies, GameConfigId::ENEMIES))
    return *res;
  if (auto res = readCreatureFactory(std::move(nameGenerator), config))
    return *res;
  if (auto res = readFurnitureFactory(config))
    return *res;
  map<ItemListId, ItemList> itemLists;
  ItemListId::startContentIdGeneration();
  if (auto res = config->readObject(itemLists, GameConfigId::ITEM_LISTS))
    return *res;
  itemFactory = ItemFactory(std::move(itemLists));
  ItemListId::validateContentIds();
  vector<TileInfo> tileDefs;
  if (auto res = config->readObject(tileDefs, GameConfigId::TILES))
    return *res;
  tilePaths = TilePaths(std::move(tileDefs), config->getModName());
  for (int alignment = 0; alignment < 2; ++alignment) {
    vector<ZLevelInfo> levels = concat<ZLevelInfo>({zLevels[0], zLevels[1 + alignment]});
    for (int depth = 0; depth < 1000; ++depth) {
      if (!isZLevel(levels, depth))
        return "No z-level found for depth " + toString(depth) + ". Please fix z-level config.";
      if (!areResourceCounts(resources, depth))
        return "No resource distribution found for depth " + toString(depth) + ". Please fix resources config.";
    }
  }
  EnemyId::validateContentIds();
  return none;
}

void ContentFactory::merge(ContentFactory f) {
  creatures.merge(std::move(f.creatures));
  furniture.merge(std::move(f.furniture));
  tilePaths.merge(std::move(f.tilePaths));
}
