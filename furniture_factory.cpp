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
#include "collective.h"
#include "territory.h"
#include "level.h"
#include "square_type.h"
#include "construction_map.h"

Furniture FurnitureFactory::get(FurnitureType type, TribeId tribe) {
  switch (type) {
    case FurnitureType::TRAINING_DUMMY:
      return Furniture("training dummy", ViewObject(ViewId::TRAINING_ROOM, ViewLayer::FLOOR), type,
          Furniture::BLOCKING, tribe);
    case FurnitureType::WORKSHOP:
      return Furniture("workshop", ViewObject(ViewId::WORKSHOP, ViewLayer::FLOOR), type,
          Furniture::BLOCKING, tribe);
    case FurnitureType::FORGE:
      return Furniture("forge", ViewObject(ViewId::FORGE, ViewLayer::FLOOR), type,
          Furniture::BLOCKING, tribe);
    case FurnitureType::LABORATORY:
      return Furniture("laboratory", ViewObject(ViewId::LABORATORY, ViewLayer::FLOOR), type,
          Furniture::BLOCKING, tribe);
    case FurnitureType::JEWELER:
      return Furniture("jeweler", ViewObject(ViewId::JEWELER, ViewLayer::FLOOR), type,
          Furniture::BLOCKING, tribe);
    case FurnitureType::BOOK_SHELF:
      return Furniture("book shelf", ViewObject(ViewId::LIBRARY, ViewLayer::FLOOR), type,
          Furniture::BLOCKING, tribe);
    case FurnitureType::THRONE:
      return Furniture("throne", ViewObject(ViewId::THRONE, ViewLayer::FLOOR), type,
          Furniture::NON_BLOCKING, tribe);
    case FurnitureType::IMPALED_HEAD:
      return Furniture("impaled head", ViewObject(ViewId::IMPALED_HEAD, ViewLayer::FLOOR), type,
          Furniture::NON_BLOCKING, tribe);
    case FurnitureType::BEAST_CAGE:
      return Furniture("beast cage", ViewObject(ViewId::BEAST_CAGE, ViewLayer::FLOOR), type,
            Furniture::NON_BLOCKING, tribe)
          .setUsageType(FurnitureUsageType::SLEEP)
          .setTickType(FurnitureTickType::BED);
    case FurnitureType::BED:
      return Furniture("bed", ViewObject(ViewId::BED, ViewLayer::FLOOR), type, Furniture::NON_BLOCKING, tribe)
          .setUsageType(FurnitureUsageType::SLEEP)
          .setTickType(FurnitureTickType::BED);
    case FurnitureType::GRAVE:
      return Furniture("grave", ViewObject(ViewId::GRAVE, ViewLayer::FLOOR), type, Furniture::NON_BLOCKING, tribe)
          .setUsageType(FurnitureUsageType::SLEEP)
          .setTickType(FurnitureTickType::BED);
    case FurnitureType::DEMON_SHRINE:
      return Furniture("demon shrine", ViewObject(ViewId::RITUAL_ROOM, ViewLayer::FLOOR), type,
          Furniture::BLOCKING, tribe);
    case FurnitureType::STOCKPILE_RES:
      return Furniture("resource stockpile", ViewObject(ViewId::STOCKPILE1, ViewLayer::FLOOR), type,
          Furniture::NON_BLOCKING, tribe);
    case FurnitureType::STOCKPILE_EQUIP:
      return Furniture("equipment stockpile", ViewObject(ViewId::STOCKPILE2, ViewLayer::FLOOR), type,
          Furniture::NON_BLOCKING, tribe);
    case FurnitureType::PRISON:
        return Furniture("prison", ViewObject(ViewId::PRISON, ViewLayer::FLOOR), type, Furniture::NON_BLOCKING, tribe);
    case FurnitureType::TREASURE_CHEST:
      return Furniture("treasure chest", ViewObject(ViewId::TREASURE_CHEST, ViewLayer::FLOOR), type,
          Furniture::NON_BLOCKING, tribe);
    case FurnitureType::EYEBALL:
      return Furniture("eyeball", ViewObject(ViewId::EYEBALL, ViewLayer::FLOOR), type,
          Furniture::BLOCKING, tribe);
    case FurnitureType::WHIPPING_POST:
      return Furniture("whipping post", ViewObject(ViewId::WHIPPING_POST, ViewLayer::FLOOR), type,
          Furniture::NON_BLOCKING, tribe);
    case FurnitureType::MINION_STATUE:
      return Furniture("statue", ViewObject(ViewId::MINION_STATUE, ViewLayer::FLOOR), type,
          Furniture::BLOCKING, tribe);
    case FurnitureType::BARRICADE:
      return Furniture("barricade", ViewObject(ViewId::BARRICADE, ViewLayer::FLOOR), type,
          Furniture::BLOCKING, tribe);
    case FurnitureType::CANIF_TREE:
      return Furniture("tree", ViewObject(ViewId::CANIF_TREE, ViewLayer::FLOOR), type, Furniture::NON_BLOCKING, tribe)
          .setCanCut()
          .setBlockVision()
          .setBlockVision(VisionId::ELF, false)
          .setDestroyedRemains(FurnitureType::TREE_TRUNK)
          .setBurntRemains(FurnitureType::BURNT_TREE)
          .setStrength(100)
          .setFireInfo(Fire(1000, 0.7))
          .setItemDrop(ItemFactory::singleType(ItemId::WOOD_PLANK, Range(25, 40)));
    case FurnitureType::DECID_TREE:
      return Furniture("tree", ViewObject(ViewId::DECID_TREE, ViewLayer::FLOOR), type, Furniture::NON_BLOCKING, tribe)
          .setCanCut()
          .setBlockVision()
          .setBlockVision(VisionId::ELF, false)
          .setDestroyedRemains(FurnitureType::TREE_TRUNK)
          .setBurntRemains(FurnitureType::BURNT_TREE)
          .setFireInfo(Fire(1000, 0.7))
          .setStrength(100)
          .setItemDrop(ItemFactory::singleType(ItemId::WOOD_PLANK, Range(25, 40)));
    case FurnitureType::TREE_TRUNK:
      return Furniture("tree trunk", ViewObject(ViewId::TREE_TRUNK, ViewLayer::FLOOR), type,
                       Furniture::NON_BLOCKING, tribe);
    case FurnitureType::BURNT_TREE:
      return Furniture("burnt tree", ViewObject(ViewId::BURNT_TREE, ViewLayer::FLOOR), type,
                       Furniture::NON_BLOCKING, tribe);
    case FurnitureType::BUSH:
      return Furniture("bush", ViewObject(ViewId::BUSH, ViewLayer::FLOOR), type, Furniture::NON_BLOCKING, tribe)
          .setCanCut()
          .setStrength(100)
          .setFireInfo(Fire(100, 0.8))
          .setItemDrop(ItemFactory::singleType(ItemId::WOOD_PLANK, Range(5, 10)));
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
      return Furniture("torch", ViewObject(ViewId::TORCH, ViewLayer::FLOOR), type,
          Furniture::NON_BLOCKING, tribe);
    case FurnitureType::TORTURE_TABLE:
      return Furniture("torture table", ViewObject(ViewId::TORTURE_TABLE, ViewLayer::FLOOR), type,
          Furniture::NON_BLOCKING, tribe)
          .setUsageType(FurnitureUsageType::TORTURE);
    case FurnitureType::FOUNTAIN:
      return Furniture("fountain", ViewObject(ViewId::FOUNTAIN, ViewLayer::FLOOR), type, Furniture::NON_BLOCKING, tribe)
          .setUsageType(FurnitureUsageType::FOUNTAIN);
    case FurnitureType::CHEST:
      return Furniture("chest", ViewObject(ViewId::CHEST, ViewLayer::FLOOR), type, Furniture::NON_BLOCKING, tribe)
          .setUsageType(FurnitureUsageType::CHEST);
    case FurnitureType::OPENED_CHEST:
      return Furniture("opened chest", ViewObject(ViewId::OPENED_CHEST, ViewLayer::FLOOR), type,
          Furniture::NON_BLOCKING, tribe);
    case FurnitureType::COFFIN:
      return Furniture("coffin", ViewObject(ViewId::COFFIN, ViewLayer::FLOOR), type, Furniture::NON_BLOCKING, tribe)
          .setUsageType(FurnitureUsageType::COFFIN);
    case FurnitureType::VAMPIRE_COFFIN:
      return Furniture("coffin", ViewObject(ViewId::COFFIN, ViewLayer::FLOOR), type, Furniture::NON_BLOCKING, tribe)
          .setUsageType(FurnitureUsageType::VAMPIRE_COFFIN);
    case FurnitureType::OPENED_COFFIN:
      return Furniture("opened coffin", ViewObject(ViewId::OPENED_COFFIN, ViewLayer::FLOOR), type,
          Furniture::NON_BLOCKING, tribe);
    case FurnitureType::DOOR:
      return Furniture("door", ViewObject(ViewId::DOOR, ViewLayer::FLOOR), type, Furniture::BLOCKING_ENEMIES, tribe)
          .setBlockVision()
          .setStrength(100)
          .setClickType(FurnitureClickType::LOCK);
    case FurnitureType::LOCKED_DOOR:
      return Furniture("locked door", ViewObject(ViewId::LOCKED_DOOR, ViewLayer::FLOOR), type,
            Furniture::BLOCKING, tribe)
          .setBlockVision()
          .setStrength(100)
          .setClickType(FurnitureClickType::UNLOCK);
    case FurnitureType::WELL:
      return Furniture("well", ViewObject(ViewId::WELL, ViewLayer::FLOOR), type,
          Furniture::NON_BLOCKING, tribe);
    case FurnitureType::KEEPER_BOARD:
      return Furniture("message board", ViewObject(ViewId::NOTICE_BOARD, ViewLayer::FLOOR), type,
            Furniture::NON_BLOCKING, tribe)
          .setUsageType(FurnitureUsageType::KEEPER_BOARD);
    case FurnitureType::UP_STAIRS:
      return Furniture("stairs", ViewObject(ViewId::UP_STAIRCASE, ViewLayer::FLOOR), type,
            Furniture::NON_BLOCKING, tribe)
          .setUsageType(FurnitureUsageType::STAIRS);
    case FurnitureType::DOWN_STAIRS:
      return Furniture("stairs", ViewObject(ViewId::DOWN_STAIRCASE, ViewLayer::FLOOR), type,
            Furniture::NON_BLOCKING, tribe)
          .setUsageType(FurnitureUsageType::STAIRS);
  }
}

static bool canBuildDoor(Position pos) {
  if (!pos.canConstruct(FurnitureType::DOOR))
    return false;
  return (pos.minus(Vec2(0, 1)).canSupportDoorOrTorch() && pos.minus(Vec2(0, -1)).canSupportDoorOrTorch()) ||
       (pos.minus(Vec2(1, 0)).canSupportDoorOrTorch() && pos.minus(Vec2(-1, 0)).canSupportDoorOrTorch());
}

bool FurnitureFactory::canBuild(FurnitureType type, Position pos, const Collective* col) {
  switch (type) {
    case FurnitureType::DOOR: return canBuildDoor(pos);
    case FurnitureType::KEEPER_BOARD:
    case FurnitureType::EYEBALL: return true;
    default: return col->getTerritory().contains(pos);
  }
}

FurnitureFactory::FurnitureFactory(TribeId t, const EnumMap<FurnitureType, double>& d,
    const vector<FurnitureType>& u) : tribe(t), distribution(d), unique(u) {
}

FurnitureFactory::FurnitureFactory(TribeId t, FurnitureType f) : tribe(t), distribution({{f, 1}}) {
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

Furniture FurnitureFactory::getRandom(RandomGen& random) {
  if (!unique.empty()) {
    FurnitureType f = unique.back();
    unique.pop_back();
    return get(f, *tribe);
  } else
    return get(random.choose(distribution), *tribe);
}

