#include "stdafx.h"
#include "furniture_factory.h"
#include "furniture.h"
#include "furniture_type.h"
#include "view_id.h"
#include "view_layer.h"
#include "view_object.h"
#include "tribe.h"
#include "item_type.h"
#include "furniture_usage.h"
#include "furniture_click.h"
#include "furniture_tick.h"
#include "furniture_entry.h"
#include "level.h"
#include "square_type.h"

static Furniture get(FurnitureType type, TribeId tribe) {
  switch (type) {
    case FurnitureType::TRAINING_WOOD:
      return Furniture("wooden training dummy", ViewObject(ViewId::TRAINING_WOOD, ViewLayer::FLOOR), type,
          Furniture::BLOCKING, tribe)
          .setUsageType(FurnitureUsageType::TRAIN)
          .setFireInfo(Fire(500, 0.5))
          .setCanHide()
          .setDestroyable(80);
    case FurnitureType::TRAINING_IRON:
      return Furniture("iron training dummy", ViewObject(ViewId::TRAINING_IRON, ViewLayer::FLOOR), type,
          Furniture::BLOCKING, tribe)
          .setUsageType(FurnitureUsageType::TRAIN)
          .setCanHide()
          .setDestroyable(80);
    case FurnitureType::TRAINING_STEEL:
      return Furniture("steel training dummy", ViewObject(ViewId::TRAINING_STEEL, ViewLayer::FLOOR), type,
          Furniture::BLOCKING, tribe)
          .setUsageType(FurnitureUsageType::TRAIN)
          .setCanHide()
          .setDestroyable(80);
    case FurnitureType::WORKSHOP:
      return Furniture("workshop", ViewObject(ViewId::WORKSHOP, ViewLayer::FLOOR), type, Furniture::BLOCKING, tribe)
          .setUsageTime(5)
          .setCanHide()
          .setFireInfo(Fire(500, 0.5))
          .setDestroyable(80);
    case FurnitureType::FORGE:
      return Furniture("forge", ViewObject(ViewId::FORGE, ViewLayer::FLOOR), type, Furniture::BLOCKING, tribe)
          .setUsageTime(5)
          .setCanHide()
          .setDestroyable(80);
    case FurnitureType::LABORATORY:
      return Furniture("laboratory", ViewObject(ViewId::LABORATORY, ViewLayer::FLOOR), type, Furniture::BLOCKING, tribe)
          .setUsageTime(5)
          .setCanHide()
          .setDestroyable(80);
    case FurnitureType::JEWELER:
      return Furniture("jeweler", ViewObject(ViewId::JEWELER, ViewLayer::FLOOR), type, Furniture::BLOCKING, tribe)
          .setUsageTime(5)
          .setCanHide()
          .setFireInfo(Fire(500, 0.5))
          .setDestroyable(40);
    case FurnitureType::STEEL_FURNACE:
      return Furniture("steel furnace", ViewObject(ViewId::STEEL_FURNACE, ViewLayer::FLOOR), type, Furniture::BLOCKING, tribe)
          .setUsageTime(5)
          .setCanHide()
          .setDestroyable(100);
    case FurnitureType::BOOK_SHELF:
      return Furniture("book shelf", ViewObject(ViewId::LIBRARY, ViewLayer::FLOOR), type, Furniture::BLOCKING, tribe)
          .setUsageTime(5)
          .setCanHide()
          .setFireInfo(Fire(700, 1.0))
          .setDestroyable(50);
    case FurnitureType::THRONE:
      return Furniture("throne", ViewObject(ViewId::THRONE, ViewLayer::FLOOR), type, Furniture::NON_BLOCKING, tribe)
          .setCanHide()
          .setDestroyable(80);
    case FurnitureType::IMPALED_HEAD:
      return Furniture("impaled head", ViewObject(ViewId::IMPALED_HEAD, ViewLayer::FLOOR), type,
          Furniture::NON_BLOCKING, tribe)
          .setDestroyable(10);
    case FurnitureType::BEAST_CAGE:
      return Furniture("beast cage", ViewObject(ViewId::BEAST_CAGE, ViewLayer::FLOOR), type,
            Furniture::NON_BLOCKING, tribe)
          .setUsageType(FurnitureUsageType::SLEEP)
          .setTickType(FurnitureTickType::BED)
          .setCanHide()
          .setFireInfo(Fire(500, 0.5))
          .setDestroyable(40);
    case FurnitureType::BED:
      return Furniture("bed", ViewObject(ViewId::BED, ViewLayer::FLOOR), type, Furniture::NON_BLOCKING, tribe)
          .setUsageType(FurnitureUsageType::SLEEP)
          .setTickType(FurnitureTickType::BED)
          .setCanHide()
          .setFireInfo(Fire(500, 0.7))
          .setDestroyable(40);
    case FurnitureType::GRAVE:
      return Furniture("grave", ViewObject(ViewId::GRAVE, ViewLayer::FLOOR), type, Furniture::NON_BLOCKING, tribe)
          .setUsageType(FurnitureUsageType::SLEEP)
          .setCanHide()
          .setTickType(FurnitureTickType::BED)
          .setDestroyable(40);
    case FurnitureType::DEMON_SHRINE:
      return Furniture("demon shrine", ViewObject(ViewId::RITUAL_ROOM, ViewLayer::FLOOR), type,
          Furniture::BLOCKING, tribe)
          .setCanHide()
          .setUsageTime(5)
          .setDestroyable(80);
    case FurnitureType::PRISON:
        return Furniture("prison", ViewObject(ViewId::PRISON, ViewLayer::FLOOR_BACKGROUND), type,
          Furniture::NON_BLOCKING, tribe);
    case FurnitureType::TREASURE_CHEST:
      return Furniture("treasure chest", ViewObject(ViewId::TREASURE_CHEST, ViewLayer::FLOOR), type,
          Furniture::NON_BLOCKING, tribe)
          .setCanHide()
          .setFireInfo(Fire(500, 0.5))
          .setDestroyable(40);
    case FurnitureType::EYEBALL:
      return Furniture("eyeball", ViewObject(ViewId::EYEBALL, ViewLayer::FLOOR), type, Furniture::BLOCKING, tribe)
          .setCanHide()
          .setDestroyable(30);
    case FurnitureType::WHIPPING_POST:
      return Furniture("whipping post", ViewObject(ViewId::WHIPPING_POST, ViewLayer::FLOOR), type,
          Furniture::NON_BLOCKING, tribe)
          .setUsageType(FurnitureUsageType::TIE_UP)
          .setFireInfo(Fire(300, 0.5))
          .setDestroyable(30);
    case FurnitureType::MINION_STATUE:
      return Furniture("statue", ViewObject(ViewId::MINION_STATUE, ViewLayer::FLOOR), type,
          Furniture::BLOCKING, tribe)
          .setCanHide()
          .setDestroyable(50);
    case FurnitureType::BARRICADE:
      return Furniture("barricade", ViewObject(ViewId::BARRICADE, ViewLayer::FLOOR), type,
          Furniture::BLOCKING, tribe)
          .setFireInfo(Fire(500, 0.3))
          .setDestroyable(80);
    case FurnitureType::CANIF_TREE:
      return Furniture("tree", ViewObject(ViewId::CANIF_TREE, ViewLayer::FLOOR), type, Furniture::NON_BLOCKING, tribe)
          .setCanHide()
          .setBlockVision()
          .setBlockVision(VisionId::ELF, false)
          .setDestroyedRemains(FurnitureType::TREE_TRUNK)
          .setBurntRemains(FurnitureType::BURNT_TREE)
          .setDestroyable(100, DestroyAction::Type::BOULDER)
          .setDestroyable(70, DestroyAction::Type::CUT)
          .setFireInfo(Fire(1000, 0.7))
          .setItemDrop(ItemFactory::singleType(ItemId::WOOD_PLANK, Range(40, 70)));
    case FurnitureType::DECID_TREE:
      return Furniture("tree", ViewObject(ViewId::DECID_TREE, ViewLayer::FLOOR), type, Furniture::NON_BLOCKING, tribe)
          .setCanHide()
          .setBlockVision()
          .setBlockVision(VisionId::ELF, false)
          .setDestroyedRemains(FurnitureType::TREE_TRUNK)
          .setBurntRemains(FurnitureType::BURNT_TREE)
          .setFireInfo(Fire(1000, 0.7))
          .setDestroyable(100, DestroyAction::Type::BOULDER)
          .setDestroyable(70, DestroyAction::Type::CUT)
          .setItemDrop(ItemFactory::singleType(ItemId::WOOD_PLANK, Range(40, 70)));
    case FurnitureType::TREE_TRUNK:
      return Furniture("tree trunk", ViewObject(ViewId::TREE_TRUNK, ViewLayer::FLOOR), type,
                       Furniture::NON_BLOCKING, tribe);
    case FurnitureType::BURNT_TREE:
      return Furniture("burnt tree", ViewObject(ViewId::BURNT_TREE, ViewLayer::FLOOR), type,
                       Furniture::NON_BLOCKING, tribe)
          .setCanHide()
          .setDestroyable(30);
    case FurnitureType::BUSH:
      return Furniture("bush", ViewObject(ViewId::BUSH, ViewLayer::FLOOR), type, Furniture::NON_BLOCKING, tribe)
          .setDestroyable(20, DestroyAction::Type::BOULDER)
          .setDestroyable(10, DestroyAction::Type::CUT)
          .setCanHide()
          .setFireInfo(Fire(100, 0.8))
          .setItemDrop(ItemFactory::singleType(ItemId::WOOD_PLANK, Range(10, 20)));
    case FurnitureType::CROPS:
      return Furniture("wheat", ViewObject(Random.choose(ViewId::CROPS, ViewId::CROPS2), ViewLayer::FLOOR),
                       type, Furniture::NON_BLOCKING, tribe)
          .setUsageType(FurnitureUsageType::CROPS)
          .setUsageTime(3);
    case FurnitureType::PIGSTY:
      return Furniture("pigsty", ViewObject(ViewId::MUD, ViewLayer::FLOOR),
                       type, Furniture::NON_BLOCKING, tribe)
          .setTickType(FurnitureTickType::PIGSTY);
    case FurnitureType::TORCH:
      return Furniture("torch", ViewObject(ViewId::TORCH, ViewLayer::FLOOR), type, Furniture::NON_BLOCKING, tribe)
          .setLightEmission(8.2);
    case FurnitureType::TORTURE_TABLE:
      return Furniture("torture table", ViewObject(ViewId::TORTURE_TABLE, ViewLayer::FLOOR), type,
          Furniture::NON_BLOCKING, tribe)
          .setCanHide()
          .setUsageType(FurnitureUsageType::TIE_UP)
          .setDestroyable(80);
    case FurnitureType::FOUNTAIN:
      return Furniture("fountain", ViewObject(ViewId::FOUNTAIN, ViewLayer::FLOOR), type, Furniture::NON_BLOCKING, tribe)
          .setCanHide()
          .setUsageType(FurnitureUsageType::FOUNTAIN)
          .setDestroyable(80);
    case FurnitureType::CHEST:
      return Furniture("chest", ViewObject(ViewId::CHEST, ViewLayer::FLOOR), type, Furniture::NON_BLOCKING, tribe)
          .setCanHide()
          .setUsageType(FurnitureUsageType::CHEST)
          .setFireInfo(Fire(500, 0.5))
          .setDestroyable(30);
    case FurnitureType::OPENED_CHEST:
      return Furniture("opened chest", ViewObject(ViewId::OPENED_CHEST, ViewLayer::FLOOR), type,
          Furniture::NON_BLOCKING, tribe)
          .setCanHide()
          .setFireInfo(Fire(500, 0.5))
          .setDestroyable(30);
    case FurnitureType::COFFIN:
      return Furniture("coffin", ViewObject(ViewId::COFFIN, ViewLayer::FLOOR), type, Furniture::NON_BLOCKING, tribe)
          .setCanHide()
          .setUsageType(FurnitureUsageType::COFFIN)
          .setFireInfo(Fire(500, 0.5))
          .setDestroyable(40);
    case FurnitureType::VAMPIRE_COFFIN:
      return Furniture("coffin", ViewObject(ViewId::COFFIN, ViewLayer::FLOOR), type, Furniture::NON_BLOCKING, tribe)
          .setCanHide()
          .setUsageType(FurnitureUsageType::VAMPIRE_COFFIN)
          .setFireInfo(Fire(500, 0.5))
          .setDestroyable(40);
    case FurnitureType::OPENED_COFFIN:
      return Furniture("opened coffin", ViewObject(ViewId::OPENED_COFFIN, ViewLayer::FLOOR), type,
          Furniture::NON_BLOCKING, tribe)
          .setCanHide()
          .setFireInfo(Fire(500, 0.5))
          .setDestroyable(40);
    case FurnitureType::DOOR:
      return Furniture("door", ViewObject(ViewId::DOOR, ViewLayer::FLOOR), type, Furniture::BLOCKING_ENEMIES, tribe)
          .setCanHide()
          .setBlockVision()
          .setFireInfo(Fire(500, 0.5))
          .setDestroyable(100)
          .setClickType(FurnitureClickType::LOCK);
    case FurnitureType::LOCKED_DOOR:
      return Furniture("locked door", ViewObject(ViewId::LOCKED_DOOR, ViewLayer::FLOOR), type,
          Furniture::BLOCKING, tribe)
          .setBlockVision()
          .setFireInfo(Fire(500, 0.5))
          .setDestroyable(100)
          .setClickType(FurnitureClickType::UNLOCK);
    case FurnitureType::WELL:
      return Furniture("well", ViewObject(ViewId::WELL, ViewLayer::FLOOR), type,
          Furniture::NON_BLOCKING, tribe)
          .setCanHide()
          .setFireInfo(Fire(500, 0.5))
          .setDestroyable(80);
    case FurnitureType::KEEPER_BOARD:
      return Furniture("message board", ViewObject(ViewId::NOTICE_BOARD, ViewLayer::FLOOR), type,
            Furniture::NON_BLOCKING, tribe)
          .setCanHide()
          .setUsageType(FurnitureUsageType::KEEPER_BOARD)
          .setFireInfo(Fire(500, 0.5))
          .setDestroyable(50);
    case FurnitureType::UP_STAIRS:
      return Furniture("stairs", ViewObject(ViewId::UP_STAIRCASE, ViewLayer::FLOOR), type,
            Furniture::NON_BLOCKING, tribe)
          .setCanHide()
          .setUsageType(FurnitureUsageType::STAIRS);
    case FurnitureType::DOWN_STAIRS:
      return Furniture("stairs", ViewObject(ViewId::DOWN_STAIRCASE, ViewLayer::FLOOR), type,
            Furniture::NON_BLOCKING, tribe)
          .setCanHide()
          .setUsageType(FurnitureUsageType::STAIRS);
    case FurnitureType::SOKOBAN_HOLE:
      return Furniture("hole", ViewObject(ViewId::SOKOBAN_HOLE, ViewLayer::FLOOR), type,
            Furniture::NON_BLOCKING, tribe)
          .setEntryType(FurnitureEntryType::SOKOBAN);
    case FurnitureType::BOULDER_TRAP:
      return Furniture("boulder", ViewObject(ViewId::BOULDER, ViewLayer::CREATURE), type, Furniture::BLOCKING, tribe)
          .setCanHide()
          .setTickType(FurnitureTickType::BOULDER_TRAP)
          .setDestroyable(40);
    case FurnitureType::BRIDGE:
      return Furniture("bridge", ViewObject(ViewId::BRIDGE, ViewLayer::FLOOR), type, Furniture::NON_BLOCKING, TribeId::getHostile())
          .setOverrideMovement();
    case FurnitureType::ROAD:
      return Furniture("road", ViewObject(ViewId::ROAD, ViewLayer::FLOOR), type, Furniture::NON_BLOCKING, tribe);
    case FurnitureType::MOUNTAIN:
      return Furniture("mountain", ViewObject(ViewId::MOUNTAIN, ViewLayer::FLOOR), type, Furniture::BLOCKING, TribeId::getHostile())
          .setBlockVision()
          .setConstructMessage(Furniture::FILL_UP)
          .setIsWall()
          .setDestroyable(200, DestroyAction::Type::BOULDER)
          .setDestroyable(50, DestroyAction::Type::DIG);
    case FurnitureType::IRON_ORE:
      return Furniture("iron ore", ViewObject(ViewId::IRON_ORE, ViewLayer::FLOOR), type, Furniture::BLOCKING, tribe)
          .setBlockVision()
          .setIsWall()
          .setDestroyable(200, DestroyAction::Type::BOULDER)
          .setItemDrop(ItemFactory::singleType(ItemId::IRON_ORE, Range(40, 70)))
          .setDestroyable(220, DestroyAction::Type::DIG);
    case FurnitureType::STONE:
      return Furniture("granite", ViewObject(ViewId::STONE, ViewLayer::FLOOR), type, Furniture::BLOCKING, tribe)
          .setBlockVision()
          .setIsWall()
          .setDestroyable(200, DestroyAction::Type::BOULDER)
          .setItemDrop(ItemFactory::singleType(ItemId::ROCK, Range(40, 70)))
          .setDestroyable(250, DestroyAction::Type::DIG);
    case FurnitureType::GOLD_ORE:
      return Furniture("gold ore", ViewObject(ViewId::GOLD_ORE, ViewLayer::FLOOR), type, Furniture::BLOCKING, tribe)
          .setBlockVision()
          .setIsWall()
          .setDestroyable(200, DestroyAction::Type::BOULDER)
          .setItemDrop(ItemFactory::singleType(ItemId::GOLD_PIECE, Range(40, 70)))
          .setDestroyable(220, DestroyAction::Type::DIG);
    case FurnitureType::DUNGEON_WALL:
      return Furniture("wall", ViewObject(ViewId::DUNGEON_WALL, ViewLayer::FLOOR), type, Furniture::BLOCKING, TribeId::getHostile())
          .setBlockVision()
          .setIsWall()
          .setConstructMessage(Furniture::REINFORCE)
          .setDestroyable(300, DestroyAction::Type::BOULDER)
          .setDestroyable(100, DestroyAction::Type::DIG);
    case FurnitureType::CASTLE_WALL:
      return Furniture("wall", ViewObject(ViewId::CASTLE_WALL, ViewLayer::FLOOR), type, Furniture::BLOCKING, tribe)
          .setBlockVision()
          .setIsWall()
          .setDestroyable(300, DestroyAction::Type::BOULDER);
    case FurnitureType::WOOD_WALL:
      return Furniture("wall", ViewObject(ViewId::WOOD_WALL, ViewLayer::FLOOR), type, Furniture::BLOCKING, tribe)
          .setBlockVision()
          .setIsWall()
          .setDestroyable(100, DestroyAction::Type::BOULDER)
          .setFireInfo(Fire(1000, 0.7));
    case FurnitureType::MUD_WALL:
      return Furniture("wall", ViewObject(ViewId::MUD_WALL, ViewLayer::FLOOR), type, Furniture::BLOCKING, tribe)
          .setBlockVision()
          .setIsWall()
          .setDestroyable(100, DestroyAction::Type::BOULDER);
    case FurnitureType::FLOOR_WOOD1:
      return Furniture("floor", ViewObject(ViewId::WOOD_FLOOR2, ViewLayer::FLOOR_BACKGROUND), type,
          Furniture::NON_BLOCKING, tribe)
          .setFireInfo(Fire(500, 0.5))
          .setLayer(FurnitureLayer::FLOOR);
    case FurnitureType::FLOOR_WOOD2:
      return Furniture("floor", ViewObject(ViewId::WOOD_FLOOR4, ViewLayer::FLOOR_BACKGROUND), type,
          Furniture::NON_BLOCKING, tribe)
          .setFireInfo(Fire(500, 0.5))
          .setLayer(FurnitureLayer::FLOOR);
    case FurnitureType::FLOOR_STONE1:
      return Furniture("floor", ViewObject(ViewId::STONE_FLOOR1, ViewLayer::FLOOR_BACKGROUND), type,
          Furniture::NON_BLOCKING, tribe)
          .setLayer(FurnitureLayer::FLOOR);
    case FurnitureType::FLOOR_STONE2:
      return Furniture("floor", ViewObject(ViewId::STONE_FLOOR5, ViewLayer::FLOOR_BACKGROUND), type,
          Furniture::NON_BLOCKING, tribe)
          .setLayer(FurnitureLayer::FLOOR);
    case FurnitureType::FLOOR_CARPET1:
      return Furniture("floor", ViewObject(ViewId::CARPET_FLOOR1, ViewLayer::FLOOR_BACKGROUND), type,
          Furniture::NON_BLOCKING, tribe)
          .setLayer(FurnitureLayer::FLOOR);
    case FurnitureType::FLOOR_CARPET2:
      return Furniture("floor", ViewObject(ViewId::CARPET_FLOOR4, ViewLayer::FLOOR_BACKGROUND), type,
          Furniture::NON_BLOCKING, tribe)
          .setLayer(FurnitureLayer::FLOOR);
  }
}

bool FurnitureParams::operator == (const FurnitureParams& p) const {

  return type == p.type && tribe == p.tribe;
}

static bool canBuildDoor(Position pos) {
  return pos.canEnterEmpty({MovementTrait::WALK}) && (
      (pos.minus(Vec2(0, 1)).isWall() && pos.minus(Vec2(0, -1)).isWall()) ||
      (pos.minus(Vec2(1, 0)).isWall() && pos.minus(Vec2(-1, 0)).isWall()));
}

bool FurnitureFactory::canBuild(FurnitureType type, Position pos) {
  switch (type) {
    case FurnitureType::BRIDGE:
      return !pos.canEnterEmpty({MovementTrait::WALK}) && pos.canEnterEmpty({MovementTrait::SWIM});
    case FurnitureType::DOOR:
      return canBuildDoor(pos);
    case FurnitureType::DUNGEON_WALL:
      if (auto furniture = pos.getFurniture(FurnitureLayer::MIDDLE))
        return furniture->getType() == FurnitureType::MOUNTAIN;
      else
        return false;
    default:
      return pos.canEnterSquare({MovementTrait::WALK}) && !pos.isWall();
  }
}

bool FurnitureFactory::isUpgrade(FurnitureType base, FurnitureType upgraded) {
  switch (base) {
    case FurnitureType::TRAINING_WOOD:
      return upgraded == FurnitureType::TRAINING_IRON || upgraded == FurnitureType::TRAINING_STEEL;
    case FurnitureType::TRAINING_IRON:
      return upgraded == FurnitureType::TRAINING_STEEL;
    default:
      return false;
  }
}

const vector<FurnitureType>& FurnitureFactory::getUpgrades(FurnitureType base) {
  static EnumMap<FurnitureType, vector<FurnitureType>> upgradeMap(
      [](const FurnitureType& base) {
    vector<FurnitureType> ret;
    for (auto type2 : ENUM_ALL(FurnitureType))
      if (FurnitureFactory::isUpgrade(base, type2))
        ret.push_back(type2);
    return ret;
  });
  return upgradeMap[base];
}

FurnitureFactory::FurnitureFactory(TribeId t, const EnumMap<FurnitureType, double>& d,
    const vector<FurnitureType>& u) : tribe(t), distribution(d), unique(u) {
}

FurnitureFactory::FurnitureFactory(TribeId t, FurnitureType f) : tribe(t), distribution({{f, 1}}) {
}

PFurniture FurnitureFactory::get(FurnitureType type, TribeId tribe) {
  return PFurniture(new Furniture(::get(type, tribe)));
}

FurnitureFactory FurnitureFactory::roomFurniture(TribeId tribe) {
  return FurnitureFactory(tribe, {
      {FurnitureType::BED, 2},
      {FurnitureType::TORCH, 1},
      {FurnitureType::CHEST, 2}
  });
}

FurnitureFactory FurnitureFactory::castleFurniture(TribeId tribe) {
  return FurnitureFactory(tribe, {
      {FurnitureType::BED, 2},
      {FurnitureType::TORCH, 1},
      {FurnitureType::FOUNTAIN, 1},
      {FurnitureType::CHEST, 2}
  });
}

FurnitureFactory FurnitureFactory::dungeonOutside(TribeId tribe) {
  return FurnitureFactory(tribe, {
      {FurnitureType::TORCH, 1},
  });
}

FurnitureFactory FurnitureFactory::castleOutside(TribeId tribe) {
  return FurnitureFactory(tribe, {
      {FurnitureType::TORCH, 1},
      {FurnitureType::WELL, 1},
  });
}

FurnitureFactory FurnitureFactory::villageOutside(TribeId tribe) {
  return FurnitureFactory(tribe, {
      {FurnitureType::TORCH, 1},
      {FurnitureType::WELL, 1},
  });
}

FurnitureFactory FurnitureFactory::cryptCoffins(TribeId tribe) {
  return FurnitureFactory(tribe, {
      {FurnitureType::COFFIN, 1},
  }, {
      FurnitureType::VAMPIRE_COFFIN
  });
}

FurnitureParams FurnitureFactory::getRandom(RandomGen& random) {
  if (!unique.empty()) {
    FurnitureType f = unique.back();
    unique.pop_back();
    return {f, *tribe};
  } else
    return {random.choose(distribution), *tribe};
}

