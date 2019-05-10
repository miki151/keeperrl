#include "stdafx.h"
#include "furniture_factory.h"
#include "furniture.h"
#include "furniture_type.h"
#include "view_id.h"
#include "view_layer.h"
#include "view_object.h"
#include "tribe.h"
#include "item_type.h"
#include "level.h"
#include "creature.h"
#include "creature_factory.h"
#include "movement_set.h"
#include "lasting_effect.h"
#include "furniture_usage.h"
#include "furniture_click.h"
#include "furniture_tick.h"
#include "furniture_entry.h"
#include "furniture_dropped_items.h"
#include "furniture_on_built.h"
#include "game_config.h"

SERIALIZE_DEF(FurnitureFactory, furniture)
SERIALIZATION_CONSTRUCTOR_IMPL(FurnitureFactory)

bool FurnitureParams::operator == (const FurnitureParams& p) const {
  return type == p.type && tribe == p.tribe;
}

const string& FurnitureFactory::getName(FurnitureType type, int count) const {
  static EnumMap<FurnitureType, string> names(
      [this] (FurnitureType type) { return getFurniture(type, TribeId::getHostile())->getName(1); });
  static EnumMap<FurnitureType, string> pluralNames(
      [this] (FurnitureType type) { return getFurniture(type, TribeId::getHostile())->getName(2); });
  if (count == 1)
    return names[type];
  else
    return pluralNames[type];
}

FurnitureLayer FurnitureFactory::getLayer(FurnitureType type) const {
  static EnumMap<FurnitureType, FurnitureLayer> layers(
      [this] (FurnitureType type) { return getFurniture(type, TribeId::getHostile())->getLayer(); });
  return layers[type];
}


bool FurnitureFactory::isWall(FurnitureType type) const {
  static EnumMap<FurnitureType, bool> layers(
      [this] (FurnitureType type) { return getFurniture(type, TribeId::getHostile())->isWall(); });
  return layers[type];
}

LuxuryInfo FurnitureFactory::getLuxuryInfo(FurnitureType type) const {
  static EnumMap<FurnitureType, LuxuryInfo> luxury(
      [this] (FurnitureType type) { return getFurniture(type, TribeId::getHostile())->getLuxuryInfo(); });
  return luxury[type];
}

pair<double, optional<int>> getPopulationIncreaseInfo(FurnitureType type) {
  switch (type) {
    case FurnitureType::PIGSTY:
      return {0.25, 4};
    case FurnitureType::MINION_STATUE:
      return {1, none};
    case FurnitureType::STONE_MINION_STATUE:
      return {1, 4};
    case FurnitureType::THRONE:
      return {10, none};
    default:
      return {0, none};
  }
}

optional<string> FurnitureFactory::getPopulationIncreaseDescription(FurnitureType type) const {
  auto info = getPopulationIncreaseInfo(type);
  if (info.first > 0) {
    auto ret = "Increases population limit by " + toString(info.first);
    if (auto limit = info.second)
      ret += ", up to " + toString(*limit);
    ret += ".";
    return ret;
  }
  return none;
}

FurnitureList FurnitureFactory::getFurnitureList(FurnitureListId id) const {
  if (id == "roomFurniture")
    return FurnitureList({
        {FurnitureType::BED1, 2},
        {FurnitureType::GROUND_TORCH, 1},
        {FurnitureType::CHEST, 2}
    });
  if (id == "castleFurniture")
    return FurnitureList({
        {FurnitureType::BED1, 2},
        {FurnitureType::GROUND_TORCH, 1},
        {FurnitureType::FOUNTAIN, 1},
        {FurnitureType::CHEST, 2}
    });
  if (id == "dungeonOutside")
    return FurnitureList({
        {FurnitureType::GROUND_TORCH, 1},
    });
  if (id == "castleOutside")
    return FurnitureList({
        {FurnitureType::INVISIBLE_ALARM, 10},
        {FurnitureType::GROUND_TORCH, 1},
        {FurnitureType::WELL, 1},
    });
  if (id == "villageOutside")
    return FurnitureList({
        {FurnitureType::GROUND_TORCH, 1},
        {FurnitureType::WELL, 1},
    });
  if (id == "vegetationLow")
    return FurnitureList({
        {FurnitureType::CANIF_TREE, 2},
        {FurnitureType::BUSH, 1 }
    });
  if (id == "vegetationHigh")
    return FurnitureList({
        {FurnitureType::DECID_TREE, 2},
        {FurnitureType::BUSH, 1 }
    });
  if (id == "cryptCoffins")
    return FurnitureList({
        {FurnitureType::LOOT_COFFIN, 1},
    }, {
        FurnitureType::VAMPIRE_COFFIN
    });  
  if (id == "towerInside")
    return FurnitureList({
        {FurnitureType::GROUND_TORCH, 1},
    });
  if (id == "graves")
    return FurnitureList({
        {FurnitureType::GRAVE, 1},
    });
  if (id == "templeInside")
    return FurnitureList({
        {FurnitureType::CHEST, 1}
    }, {FurnitureType::ALTAR});
  USER_FATAL << "FurnitueListId not found: " << id;
  fail();
}

int FurnitureFactory::getPopulationIncrease(FurnitureType type, int numBuilt) const {
  auto info = getPopulationIncreaseInfo(type);
  return min(int(numBuilt * info.first), info.second.value_or(1000000));
}

PFurniture FurnitureFactory::getFurniture(FurnitureType type, TribeId tribe) const {
  USER_CHECK(furniture.count(type)) << "Furniture type not found " << int(type);
  auto ret = makeOwner<Furniture>(furniture.at(type));
  ret->setTribe(tribe);
  return ret;
}

const ViewObject& FurnitureFactory::getConstructionObject(FurnitureType type) const {
  static EnumMap<FurnitureType, optional<ViewObject>> objects;
  if (!objects[type]) {
    if (auto obj = getFurniture(type, TribeId::getMonster())->getViewObject())
      objects[type] = *obj;
    objects[type]->setModifier(ViewObject::Modifier::PLANNED);
  }
  return *objects[type];
}

ViewId FurnitureFactory::getViewId(FurnitureType type) const {
  return getFurniture(type, TribeId::getMonster())->getViewObject()->id();
}

bool FurnitureFactory::hasSupport(FurnitureType type, Position pos) const {
  switch (type) {
    case FurnitureType::CANDELABRUM_N:
    case FurnitureType::TORCH_N:
      return pos.minus(Vec2(0, 1)).isWall();
    case FurnitureType::CANDELABRUM_E:
    case FurnitureType::TORCH_E:
      return pos.plus(Vec2(1, 0)).isWall();
    case FurnitureType::CANDELABRUM_S:
    case FurnitureType::TORCH_S:
      return pos.plus(Vec2(0, 1)).isWall();
    case FurnitureType::CANDELABRUM_W:
    case FurnitureType::TORCH_W:
      return pos.minus(Vec2(1, 0)).isWall();
    case FurnitureType::IRON_DOOR:
    case FurnitureType::ADA_DOOR:
    case FurnitureType::WOOD_DOOR:
      return (pos.minus(Vec2(0, 1)).isWall() && pos.minus(Vec2(0, -1)).isWall()) ||
             (pos.minus(Vec2(1, 0)).isWall() && pos.minus(Vec2(-1, 0)).isWall());
    default:
      return true;
  }
}

static bool canSilentlyReplace(FurnitureType type) {
  switch (type) {
    case FurnitureType::TREE_TRUNK:
      return true;
    default:
      return false;
  }
}

FurnitureFactory::FurnitureFactory(const GameConfig* config) {
  while (1) {
    if (auto res = config->readObject(furniture, GameConfigId::FURNITURE)) {
      USER_INFO << *res;
      continue;
    }
    break;
  }
  for (auto& elem : furniture)
    elem.second.setType(elem.first);
}

bool FurnitureFactory::canBuild(FurnitureType type, Position pos) const {
  switch (type) {
    case FurnitureType::BRIDGE:
      return pos.getFurniture(FurnitureLayer::GROUND)->canBuildBridgeOver();
    case FurnitureType::DUNGEON_WALL:
      if (auto furniture = pos.getFurniture(FurnitureLayer::MIDDLE))
        return furniture->getType() == FurnitureType::MOUNTAIN;
      else
        return false;
    case FurnitureType::DUNGEON_WALL2:
      if (auto furniture = pos.getFurniture(FurnitureLayer::MIDDLE))
        return furniture->getType() == FurnitureType::MOUNTAIN2;
      else
        return false;
    default: {
      auto original = pos.getFurniture(getLayer(type));
      return pos.getFurniture(FurnitureLayer::GROUND)->getMovementSet().canEnter({MovementTrait::WALK}) &&
          (!original || canSilentlyReplace(original->getType())) && !pos.isWall();
    }
  }
}

bool FurnitureFactory::isUpgrade(FurnitureType base, FurnitureType upgraded) const {
  switch (base) {
    case FurnitureType::TRAINING_WOOD:
      return upgraded == FurnitureType::TRAINING_IRON || upgraded == FurnitureType::TRAINING_ADA;
    case FurnitureType::TRAINING_IRON:
      return upgraded == FurnitureType::TRAINING_ADA;
    case FurnitureType::BOOKCASE_WOOD:
      return upgraded == FurnitureType::BOOKCASE_IRON || upgraded == FurnitureType::BOOKCASE_GOLD;
    case FurnitureType::BOOKCASE_IRON:
      return upgraded == FurnitureType::BOOKCASE_GOLD;
    case FurnitureType::BED1:
      return upgraded == FurnitureType::BED2 || upgraded == FurnitureType::BED3;
    case FurnitureType::BED2:
      return upgraded == FurnitureType::BED3;
    case FurnitureType::COFFIN1:
      return upgraded == FurnitureType::COFFIN2 || upgraded == FurnitureType::COFFIN3;
    case FurnitureType::COFFIN2:
      return upgraded == FurnitureType::COFFIN3;
    default:
      return false;
  }
}

const vector<FurnitureType>& FurnitureFactory::getUpgrades(FurnitureType base) const {
  static EnumMap<FurnitureType, vector<FurnitureType>> upgradeMap(
      [this](const FurnitureType& base) {
    vector<FurnitureType> ret;
    for (auto type2 : ENUM_ALL(FurnitureType))
      if (isUpgrade(base, type2))
        ret.push_back(type2);
    return ret;
  });
  return upgradeMap[base];
}

FurnitureType FurnitureFactory::getWaterType(double depth) const {
  if (depth >= 2.0)
    return FurnitureType::WATER;
  else if (depth >= 1.0)
    return FurnitureType::SHALLOW_WATER1;
  else
    return FurnitureType::SHALLOW_WATER2;
}

FurnitureFactory::~FurnitureFactory() {
}

FurnitureFactory::FurnitureFactory(FurnitureFactory&&) = default;
