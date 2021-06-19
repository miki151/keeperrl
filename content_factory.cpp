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
#include "key_verifier.h"
#include "spell_school_id.h"
#include "name_generator_id.h"
#include "build_info.h"
#include "immigrant_info.h"
#include "external_enemies.h"
#include "external_enemies.h"
#include "enemy_info.h"
#include "item_attributes.h"
#include "resource_counts.h"
#include "z_level_info.h"
#include "equipment.h"
#include "sdl.h"
#include "layout_generator.h"

template <class Archive>
void ContentFactory::serialize(Archive& ar, const unsigned int) {
  ar(creatures, furniture, resources, zLevels, tilePaths, enemies, externalEnemies, itemFactory, workshopGroups, immigrantsData, buildInfo, villains, gameIntros, adventurerCreatures, keeperCreatures, technology, items, buildingInfo, mapLayouts, biomeInfo, campaignInfo, workshopInfo, resourceInfo, resourceOrder, layoutMapping, randomLayouts);
  creatures.setContentFactory(this);
}

SERIALIZABLE(ContentFactory)

template <typename Key, typename Value>
map<Key, Value> convertKeys(const map<PrimaryId<Key>, Value>& m) {
  map<Key, Value> ret;
  for (auto& elem : m)
    ret.insert(make_pair(Key(elem.first), std::move(elem.second)));
  return ret;
}

template <typename Key, typename Value>
unordered_map<Key, Value, CustomHash<Key>> convertKeysHash(const map<PrimaryId<Key>, Value>& m) {
  unordered_map<Key, Value, CustomHash<Key>> ret;
  for (auto& elem : m)
    ret.insert(make_pair(Key(elem.first), std::move(elem.second)));
  return ret;
}

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

optional<string> ContentFactory::readCreatureFactory(const GameConfig* config, KeyVerifier* keyVerifier) {
  map<PrimaryId<CreatureId>, CreatureAttributes> attributes;
  if (auto res = config->readObject(attributes, GameConfigId::CREATURE_ATTRIBUTES, keyVerifier))
    return *res;
  map<PrimaryId<SpellSchoolId>, SpellSchool> spellSchools;
  map<PrimaryId<SpellId>, Spell> spellsTmp;
  if (auto res = config->readObject(spellsTmp, GameConfigId::SPELLS, keyVerifier))
    return *res;
  vector<Spell> spells;
  for (auto& elem : spellsTmp) {
    elem.second.setSpellId(elem.first);
    spells.push_back(elem.second);
  }
  if (auto res = config->readObject(spellSchools, GameConfigId::SPELL_SCHOOLS, keyVerifier))
    return *res;
  map<PrimaryId<NameGeneratorId>, vector<string>> firstNames;
  if (auto res = config->readObject(firstNames, GameConfigId::NAMES, keyVerifier))
    return *res;
  NameGenerator nameGenerator;
  for (auto& elem : firstNames)
    nameGenerator.setNames(elem.first, elem.second);
  keyVerifier->addKey<CreatureId>("KRAKEN");
  for (auto& elem : CreatureFactory::getSpecialParams())
    keyVerifier->addKey<CreatureId>(elem.first.data());
  creatures = CreatureFactory(std::move(nameGenerator), convertKeys(std::move(attributes)),
      convertKeys(std::move(spellSchools)), std::move(spells));
  creatures.setContentFactory(this);
  return none;
}

optional<string> ContentFactory::readFurnitureFactory(const GameConfig* config, KeyVerifier* keyVerifier) {
  map<PrimaryId<FurnitureType>, Furniture> elems;
  if (auto res = config->readObject(elems, GameConfigId::FURNITURE, keyVerifier))
    return *res;
  map<FurnitureType, unique_ptr<Furniture>> furnitureDefs;
  map<PrimaryId<FurnitureListId>, FurnitureList> furnitureLists;
  for (auto& elem : elems) {
    elem.second.setType(elem.first);
    furnitureDefs.insert(make_pair(elem.first, unique<Furniture>(elem.second)));
  }
  if (auto res = config->readObject(furnitureLists, GameConfigId::FURNITURE_LISTS, keyVerifier))
    return *res;
  furniture = FurnitureFactory(std::move(furnitureDefs), convertKeys(std::move(furnitureLists)));
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

optional<string> ContentFactory::readVillainsTuple(const GameConfig* gameConfig, KeyVerifier* keyVerifier) {
  if (auto error = gameConfig->readObject(villains, GameConfigId::CAMPAIGN_VILLAINS, keyVerifier))
    return "Error reading campaign villains definition"_s + *error;
  auto has = [](vector<Campaign::VillainInfo> v, VillainType type) {
    return std::any_of(v.begin(), v.end(), [type](const auto& elem){ return elem.type == type; });
  };
  for (auto villainType : {VillainType::ALLY, VillainType::MAIN, VillainType::LESSER})
    for (auto role : ENUM_ALL(PlayerRole))
      for (auto alignment : ENUM_ALL(TribeAlignment)) {
        auto index = [&] {
          switch (role) {
            case PlayerRole::KEEPER:
              switch (alignment) {
                case TribeAlignment::EVIL:
                  return VillainGroup::EVIL_KEEPER;
                case TribeAlignment::LAWFUL:
                  return VillainGroup::LAWFUL_KEEPER;
              }
            case PlayerRole::ADVENTURER:
              switch (alignment) {
                case TribeAlignment::EVIL:
                  return VillainGroup::EVIL_ADVENTURER;
                case TribeAlignment::LAWFUL:
                  return VillainGroup::LAWFUL_ADVENTURER;
              }
          }
        }();
        if (!has(villains[index], villainType))
          return "Empty " + EnumInfo<VillainType>::getString(villainType) + " villain list for alignment: "
                    + EnumInfo<TribeAlignment>::getString(alignment);
      }
  return none;
}

optional<string> ContentFactory::readPlayerCreatures(const GameConfig* config, KeyVerifier* keyVerifier) {
  map<string, KeeperCreatureInfo> keeperCreaturesTmp;
  map<string, AdventurerCreatureInfo> adventurerCreaturesTmp;
  if (auto error = config->readObject(adventurerCreaturesTmp, GameConfigId::ADVENTURER_CREATURES, keyVerifier))
    return "Error reading player creature definitions: "_s + *error;
  if (auto error = config->readObject(keeperCreaturesTmp, GameConfigId::KEEPER_CREATURES, keyVerifier))
    return "Error reading player creature definitions: "_s + *error;
  keeperCreatures = vector<pair<string, KeeperCreatureInfo>>(keeperCreaturesTmp.begin(), keeperCreaturesTmp.end());
  adventurerCreatures = vector<pair<string, AdventurerCreatureInfo>>(adventurerCreaturesTmp.begin(), adventurerCreaturesTmp.end());
  if (keeperCreatures.empty() || adventurerCreatures.empty())
    return "Keeper and adventurer lists must each contain at least 1 entry."_s;
  for (auto& keeperInfo : keeperCreatures) {
    bool hotkeys[128] = {0};
    vector<BuildInfo> buildInfoTmp;
    set<string> allDataGroups;
    for (auto& group : buildInfo) {
      allDataGroups.insert(group.first);
      if (keeperInfo.second.buildingGroups.contains(group.first))
        buildInfoTmp.append(group.second);
    }
    for (auto& tech : keeperInfo.second.initialTech)
      if (!keeperInfo.second.technology.contains(tech))
        return "Technology not found: "_s + tech.data();
    for (auto& info : buildInfoTmp)
      for (auto& requirement : info.requirements)
        if (auto tech = requirement.getReferenceMaybe<TechId>())
          if (!keeperInfo.second.technology.contains(*tech))
            return "Technology prerequisite \""_s + tech->data() + "\" of build item \"" + info.name + "\" is not available "
                + " for keeper " + keeperInfo.first;
    WorkshopArray merged;
    for (auto& group : keeperInfo.second.workshopGroups)
      if (!workshopGroups.count(group))
        return "Undefined workshop group \"" + group + "\"";
    for (auto& group : workshopGroups) {
      if (keeperInfo.second.workshopGroups.contains(group.first))
        for (auto elem : group.second)
          merged[elem.first].append(elem.second);
    }
    for (auto& elem : merged)
      for (auto& item : elem.second)
        if (item.tech && !technology.techs.count(*item.tech))
          return "Technology prerequisite \""_s + item.tech->data() + "\" of workshop item \"" + item.item.get(this)->getName()
              + "\" is not available " + " for keeper " + keeperInfo.first;
    for (auto elem : keeperInfo.second.immigrantGroups)
      if (!immigrantsData.count(elem))
        return "Undefined immigrant group: \"" + elem + "\"";
    for (auto& group : keeperInfo.second.buildingGroups)
      if (!allDataGroups.count(group))
        return "Building menu group \"" + group + "\" not found";
    for (auto& info : buildInfoTmp) {
      if (info.hotkey != '\0') {
        if (hotkeys[int(info.hotkey)])
          return "Hotkey \'" + string(1, info.hotkey) + "\' is used more than once in building menu";
        hotkeys[int(info.hotkey)] = true;
      }
    }
    for (auto elem : keeperInfo.second.endlessEnemyGroups)
      if (!externalEnemies.count(elem))
        return "Undefined endless enemy group: \"" + elem + "\"";
  }
  return none;
}

optional<string> ContentFactory::readItems(const GameConfig* config, KeyVerifier* keyVerifier) {
  map<PrimaryId<CustomItemId>, ItemAttributes> itemsTmp;
  if (auto res = config->readObject(itemsTmp, GameConfigId::ITEMS, keyVerifier))
    return *res;
  items = convertKeys(itemsTmp);
  return none;
}

optional<string> ContentFactory::readBuildingInfo(const GameConfig* config, KeyVerifier* keyVerifier) {
  map<PrimaryId<BuildingId>, BuildingInfo> buildingsTmp;
  if (auto res = config->readObject(buildingsTmp, GameConfigId::BUILDING_INFO, keyVerifier))
    return *res;
  buildingInfo = convertKeys(buildingsTmp);
  return none;
}

static Color getpixel(SDL::SDL_Surface *surface, int x, int y) {
  auto get = [&]() -> SDL::Uint32 {
    int bpp = surface->format->BytesPerPixel;
    Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;
    switch (bpp) {
      case 1:
        return *p;
      case 2:
        return *(SDL::Uint16*)p;
      case 3:
        if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
          return p[0] << 16 | p[1] << 8 | p[2];
        else
          return p[0] | p[1] << 8 | p[2] << 16;
        case 4:
          return *(SDL::Uint32 *)p;
        default:
          fail();
    }
  };
  Color ret;
  SDL::SDL_GetRGB(get(), surface->format, &ret.r, &ret.g, &ret.b);
  return ret;
}

static optional<string> readMapLayouts(MapLayouts& layouts, KeyVerifier& keyVerifier, const DirectoryPath& path) {
  if (path.exists())
    for (auto subdir : path.getSubDirs()) {
      for (auto file : path.subdirectory(subdir).getFiles()) {
        SDL::SDL_Surface* im = SDL::IMG_Load(file.getPath());
        USER_CHECK(!!im) << "Error loading " << file.getPath() << ": " << SDL::IMG_GetError();
        MapLayouts::Layout layout(im->w, im->h - 1);
        for (auto v : layout.getBounds()) {
          auto color = getpixel(im, v.x, v.y);
          if (color == Color(255, 255, 255))
            layout[v] = LayoutPiece::CORRIDOR;
          else if (color == Color(0, 0, 0))
            layout[v] = LayoutPiece::WALL;
          else if (color == Color(0, 255, 0))
            layout[v] = LayoutPiece::FLOOR_INSIDE;
          else if (color == Color(80, 40, 0))
            layout[v] = LayoutPiece::FLOOR_OUTSIDE;
          else if (color == Color(0, 0, 255))
            layout[v] = LayoutPiece::WATER;
          else if (color == Color(255, 0, 255))
            layout[v] = LayoutPiece::PRETTY_FLOOR;
          else if (color == Color(160, 80, 0))
            layout[v] = LayoutPiece::BRIDGE;
          else if (color == Color(255, 0, 0))
            layout[v] = LayoutPiece::DOOR;
          else if (color == Color(255, 255, 0))
            layout[v] = LayoutPiece::UP_STAIRS;
          else if (color == Color(0, 255, 255))
            layout[v] = LayoutPiece::DOWN_STAIRS;
          else if (color != Color(128, 128, 128))
            return "Unrecognized color in "_s +  file.getPath() + ": " + toString(color);
        }
        SDL::SDL_FreeSurface(im);
        auto id = MapLayoutId(subdir.data());
        keyVerifier.addKey<MapLayoutId>(subdir.data());
        if (auto error = layouts.addLayout(id, std::move(layout)))
          return "Error reading map layout "_s + file.getPath() + ": " + *error;
      }
    }
  return none;
}

optional<string> ContentFactory::readWorkshopInfo(const GameConfig* config, KeyVerifier* keyVerifier) {
  map<PrimaryId<WorkshopType>, WorkshopInfo> tmp;
  if (auto error = config->readObject(tmp, GameConfigId::WORKSHOP_INFO, keyVerifier))
    return *error;
  workshopInfo = convertKeys(tmp);
  return none;
}

optional<string> ContentFactory::readResourceInfo(const GameConfig* config, KeyVerifier* keyVerifier) {
  vector<pair<PrimaryId<CollectiveResourceId>, ResourceInfo>> tmp;
  if (auto error = config->readObject(tmp, GameConfigId::RESOURCE_INFO, keyVerifier))
    return *error;
  for (int i : All(tmp)) {
    resourceOrder.push_back(tmp[i].first);
    resourceInfo.insert(make_pair(CollectiveResourceId(tmp[i].first), std::move(tmp[i].second)));
  }
  return none;
}

optional<string> ContentFactory::readCampaignInfo(const GameConfig* config, KeyVerifier* keyVerifier) {
  map<string, CampaignInfo> campaignTmp;
  if (auto res = config->readObject(campaignTmp, GameConfigId::CAMPAIGN_INFO, keyVerifier))
    return *res;
  string elemName = "default";
  if (campaignTmp.size() != 1 || !campaignTmp.count(elemName))
    return "Campaign info table should contain exactly one element named \"" + elemName + "\""_s;
  campaignInfo = campaignTmp.at(elemName);
  for (auto& l : {campaignInfo.maxAllies, campaignInfo.maxMainVillains, campaignInfo.maxLesserVillains})
    if (l < 1 || l > 20)
      return "Campaign villain limits must be between 1 and 20"_s;
  if (campaignInfo.influenceSize < 1)
    return "Campaign influence size must be 1 or higher"_s;
  if (campaignInfo.mapZoom < 1 || campaignInfo.mapZoom > 3)
    return "Campaign map zoom must be between 1 and 3"_s;
  return none;
}

optional<string> ContentFactory::readZLevels(const GameConfig* config, KeyVerifier* keyVerifier) {
  if (auto res = config->readObject(zLevels, GameConfigId::Z_LEVELS, keyVerifier))
    return *res;
  for (auto& keeperInfo : keeperCreatures) {
    vector<ZLevelInfo> levels;
    for (auto group : keeperInfo.second.zLevelGroups) {
      if (!zLevels.count(group))
        return "Unknown z-level group: \"" + group + "\" in the definition of keeper \"" + keeperInfo.first + "\"";
      append(levels, zLevels.at(group));
    }
    for (int depth = 0; depth < 1000; ++depth) {
      if (!isZLevel(levels, depth))
        return "No z-level found for keeper \"" + keeperInfo.first + "\" at depth " + toString(depth) +
            ". Please fix z-level config.";
      if (!areResourceCounts(resources, depth))
        return "No resource distribution found for keeper \"" + keeperInfo.first + "\" at depth " + toString(depth) +
            ". Please fix resources config.";
    }
  }
  return none;
}

optional<string> ContentFactory::readData(const GameConfig* config, const vector<string>& modNames) {
  KeyVerifier keyVerifier;
  vector<PrimaryId<StorageId>> storageIds;
  if (auto error = config->readObject(storageIds, GameConfigId::STORAGE_IDS, &keyVerifier))
    return *error;
  map<PrimaryId<TechId>, Technology::TechDefinition> techsTmp;
  if (auto error = config->readObject(techsTmp, GameConfigId::TECHNOLOGY, &keyVerifier))
    return *error;
  technology = Technology(convertKeys(techsTmp));
  for (auto& dir : config->dirs)
    if (auto error = readMapLayouts(mapLayouts, keyVerifier, dir.subdirectory("map_layouts")))
      return *error;
  if (auto error = config->readObject(workshopGroups, GameConfigId::WORKSHOPS_MENU, &keyVerifier))
    return *error;
  if (auto error = config->readObject(immigrantsData, GameConfigId::IMMIGRATION, &keyVerifier))
    return *error;
  if (auto error = checkGroupCounts(immigrantsData))
    return *error;
  if (auto error = config->readObject(buildInfo, GameConfigId::BUILD_MENU, &keyVerifier))
    return *error;
  if (auto error = readVillainsTuple(config, &keyVerifier))
    return *error;
  if (auto error = config->readObject(gameIntros, GameConfigId::GAME_INTRO_TEXT, &keyVerifier))
    return *error;
  if (auto res = config->readObject(externalEnemies, GameConfigId::EXTERNAL_ENEMIES, &keyVerifier))
    return *res;
  if (auto error = readPlayerCreatures(config, &keyVerifier))
    return *error;
  if (auto error = readWorkshopInfo(config, &keyVerifier))
    return *error;
  if (auto res = readItems(config, &keyVerifier))
    return *res;
  if (auto error = readResourceInfo(config, &keyVerifier))
    return *error;
  if (auto error = readCampaignInfo(config, &keyVerifier))
    return *error;
  if (auto res = config->readObject(resources, GameConfigId::RESOURCE_COUNTS, &keyVerifier))
    return *res;
  if (auto error = readZLevels(config, &keyVerifier))
    return *error;
  for (auto& elem : resources)
    for (auto& count : elem.counts.elems)
      if (count.size.isEmpty() || count.size.getStart() < 1)
        return "Invalid resource count "_s + count.type.data() + ": " + toString(count.size);
  if (auto res = readBuildingInfo(config, &keyVerifier))
    return *res;
  map<PrimaryId<EnemyId>, EnemyInfo> enemiesTmp;
  if (auto res = config->readObject(enemiesTmp, GameConfigId::ENEMIES, &keyVerifier))
    return *res;
  enemies = convertKeys(enemiesTmp);
  if (auto res = readCreatureFactory(config, &keyVerifier))
    return *res;
  if (auto res = readFurnitureFactory(config, &keyVerifier))
    return *res;
  map<PrimaryId<BiomeId>, BiomeInfo> biomesTmp;
  if (auto res = config->readObject(biomesTmp, GameConfigId::BIOMES, &keyVerifier))
    return *res;
  biomeInfo = convertKeys(biomesTmp);
  map<PrimaryId<ItemListId>, ItemList> itemLists;
  if (auto res = config->readObject(itemLists, GameConfigId::ITEM_LISTS, &keyVerifier))
    return *res;
  itemFactory = ItemFactory(convertKeys(std::move(itemLists)));
  vector<TileInfo> tileDefs;
  for (auto id : {"bridge", "tutorial_entrance", "accept_immigrant", "reject_immigrant", "fog_of_war_corner"})
    keyVerifier.addKey<ViewId>(id);
  if (auto res = config->readObject(tileDefs, GameConfigId::TILES, &keyVerifier))
    return *res;
  tilePaths = TilePaths(std::move(tileDefs), modNames);
  map<PrimaryId<LayoutMappingId>, LayoutMapping> layoutTmp;
  if (auto res = config->readObject(layoutTmp, GameConfigId::LAYOUT_MAPPING, &keyVerifier))
    return *res;
  layoutMapping = convertKeys(layoutTmp);
  map<PrimaryId<RandomLayoutId>, LayoutGenerator> layoutTmp2;
  if (auto res = config->readObject(layoutTmp2, GameConfigId::RANDOM_LAYOUTS, &keyVerifier))
    return *res;
  randomLayouts = convertKeys(layoutTmp2);
  auto errors = keyVerifier.verify();
  if (!errors.empty())
    return errors.front();
  for (auto& group : zLevels)
    for (int i : All(group.second)) {
      auto& level = group.second[i];
      if (auto fullLevel = level.type.getReferenceMaybe<FullZLevel>())
        if (fullLevel->attackChance > 0.0 && (!fullLevel->enemy || !enemies.at(*fullLevel->enemy).behaviour))
            return "Z-level enemy " + group.first + " no. " + toString(i)
                + " has positive attack chance, but no attack behaviour defined"_s;
    }
  furniture.initializeInfos();
  for (auto& elem : items)
    if (auto id = elem.second.resourceId) {
      auto& info = resourceInfo.at(*id);
      info.itemId = ItemType(elem.first);
      for (auto storageId : elem.second.storageIds)
        if (!info.storage.contains(storageId))
          info.storage.push_back(storageId);
    }
  for (auto& enemy : enemies)
    enemy.second.updateBuildingInfo(buildingInfo);
  return none;
}



void ContentFactory::merge(ContentFactory f) {
  creatures.merge(std::move(f.creatures));
  furniture.merge(std::move(f.furniture));
  tilePaths.merge(std::move(f.tilePaths));
  mergeMap(std::move(f.items), items);
  mergeMap(std::move(f.workshopInfo), workshopInfo);
  mergeMap(std::move(f.resourceInfo), resourceInfo);
}

CreatureFactory& ContentFactory::getCreatures() {
  creatures.setContentFactory(this);
  return creatures;
}

const CreatureFactory& ContentFactory::getCreatures() const {
  creatures.setContentFactory(this);
  return creatures;
}

optional<WorkshopType> ContentFactory::getWorkshopType(FurnitureType type) const {
  for (auto& elem : workshopInfo)
    if (elem.second.furniture == type)
      return elem.first;
  return none;
}

ContentFactory::ContentFactory() {}
ContentFactory::~ContentFactory() {}
ContentFactory::ContentFactory(ContentFactory&&) noexcept = default;
ContentFactory& ContentFactory::operator = (ContentFactory&&) = default;
