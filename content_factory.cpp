#include "stdafx.h"
#include "content_factory.h"
#include "name_generator.h"
#include "game_config.h"
#include "creature_inventory.h"
#include "creature_attributes.h"
#include "furniture.h"

SERIALIZE_DEF(ContentFactory, creatures, furniture, resources, zLevels, tilePaths, enemies, itemFactory)

SERIALIZATION_CONSTRUCTOR_IMPL(ContentFactory)

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

ContentFactory::ContentFactory(NameGenerator nameGenerator, const GameConfig* config) {
  EnemyId::startContentIdGeneration();
  while (1) {
    if (auto res = config->readObject(zLevels, GameConfigId::Z_LEVELS)) {
      USER_INFO << *res;
      continue;
    }
    if (auto res = config->readObject(resources, GameConfigId::RESOURCE_COUNTS)) {
      USER_INFO << *res;
      continue;
    }
    if (auto res = config->readObject(enemies, GameConfigId::ENEMIES)) {
      USER_INFO << *res;
      continue;
    }
    if (auto res = readCreatureFactory(std::move(nameGenerator), config)) {
      USER_INFO << *res;
      continue;
    }
    if (auto res = readFurnitureFactory(config)) {
      USER_INFO << *res;
      continue;
    }
    map<ItemListId, ItemList> itemLists;
    ItemListId::startContentIdGeneration();
    if (auto res = config->readObject(itemLists, GameConfigId::ITEM_LISTS)) {
      USER_INFO << *res;
      continue;
    }
    itemFactory = ItemFactory(std::move(itemLists));
    ItemListId::validateContentIds();
    vector<TileInfo> tileDefs;
    if (auto error = config->readObject(tileDefs, GameConfigId::TILES)) {
      USER_INFO << *error;
      continue;
    }
    tilePaths = TilePaths(std::move(tileDefs), config->getModName());
    for (int alignment = 0; alignment < 2; ++alignment) {
      vector<ZLevelInfo> levels = concat<ZLevelInfo>({zLevels[0], zLevels[1 + alignment]});
      for (int depth = 0; depth < 1000; ++depth) {
        if (!isZLevel(levels, depth)) {
          USER_INFO << "No z-level found for depth " << depth << ". Please fix z-level config.";
          continue;
        }
        if (!areResourceCounts(resources, depth)) {
          USER_INFO << "No resource distribution found for depth " << depth << ". Please fix resources config.";
          continue;
        }
      }
    }
    break;
  }
  EnemyId::validateContentIds();
}

void ContentFactory::merge(ContentFactory f) {
  creatures.merge(std::move(f.creatures));
  furniture.merge(std::move(f.furniture));
  tilePaths.merge(std::move(f.tilePaths));
}
