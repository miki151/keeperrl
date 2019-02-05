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
#include "furniture_dropped_items.h"
#include "level.h"
#include "creature.h"
#include "creature_factory.h"
#include "movement_set.h"
#include "lasting_effect.h"
#include "furniture_on_built.h"

static Furniture get(FurnitureType type, TribeId tribe) {
  switch (type) {
    case FurnitureType::TRAINING_WOOD:
      return Furniture("wooden training dummy", ViewObject(ViewId::TRAINING_WOOD, ViewLayer::FLOOR), type, tribe)
          .setBlocking()
          .setUsageType(FurnitureUsageType::TRAIN)
          .setFireInfo(Fire(500, 0.5))
          .setCanHide()
          .setDestroyable(80);
    case FurnitureType::TRAINING_IRON:
      return Furniture("iron training dummy", ViewObject(ViewId::TRAINING_IRON, ViewLayer::FLOOR), type, tribe)
          .setBlocking()
          .setUsageType(FurnitureUsageType::TRAIN)
          .setCanHide()
          .setDestroyable(80);
    case FurnitureType::TRAINING_ADA:
      return Furniture("adamantine training dummy", ViewObject(ViewId::TRAINING_ADA, ViewLayer::FLOOR), type, tribe)
          .setBlocking()
          .setUsageType(FurnitureUsageType::TRAIN)
          .setCanHide()
          .setDestroyable(80);
    case FurnitureType::ARCHERY_RANGE:
      return Furniture("archery target", ViewObject(ViewId::ARCHERY_RANGE, ViewLayer::FLOOR), type, tribe)
          .setBlocking()
          .setStopProjectiles()
          .setUsageType(FurnitureUsageType::ARCHERY_RANGE)
          .setCanHide()
          .setDestroyable(80);
    case FurnitureType::WORKSHOP:
      return Furniture("workshop", ViewObject(ViewId::WORKSHOP, ViewLayer::FLOOR), type, tribe)
          .setBlocking()
          .setUsageTime(5_visible)
          .setCanHide()
          .setFireInfo(Fire(500, 0.5))
          .setDestroyable(80);
    case FurnitureType::FORGE:
      return Furniture("forge", ViewObject(ViewId::FORGE, ViewLayer::FLOOR), type, tribe)
          .setBlocking()
          .setUsageTime(5_visible)
          .setCanHide()
          .setDestroyable(80);
    case FurnitureType::LABORATORY:
      return Furniture("laboratory", ViewObject(ViewId::LABORATORY, ViewLayer::FLOOR), type, tribe)
          .setBlocking()
          .setUsageTime(5_visible)
          .setCanHide()
          .setDestroyable(80);
    case FurnitureType::JEWELLER:
      return Furniture("jeweller", ViewObject(ViewId::JEWELLER, ViewLayer::FLOOR), type, tribe)
          .setBlocking()
          .setUsageTime(5_visible)
          .setCanHide()
          .setFireInfo(Fire(500, 0.5))
          .setDestroyable(40);
    case FurnitureType::FURNACE:
      return Furniture("furnace", ViewObject(ViewId::FURNACE, ViewLayer::FLOOR), type, tribe)
          .setBlocking()
          .setUsageTime(5_visible)
          .setCanHide()
          .setDestroyable(100);
    case FurnitureType::BOOKCASE_WOOD:
      return Furniture("wooden bookcase", ViewObject(ViewId::BOOKCASE_WOOD, ViewLayer::FLOOR), type, tribe)
          .setBlocking()
          .setUsageType(FurnitureUsageType::STUDY)
          .setUsageTime(5_visible)
          .setCanHide()
          .setFireInfo(Fire(700, 1.0))
          .setDestroyable(50);
    case FurnitureType::BOOKCASE_IRON:
      return Furniture("iron bookcase", ViewObject(ViewId::BOOKCASE_IRON, ViewLayer::FLOOR), type, tribe)
          .setBlocking()
          .setUsageType(FurnitureUsageType::STUDY)
          .setUsageTime(5_visible)
          .setCanHide()
          .setFireInfo(Fire(700, 0.5))
          .setDestroyable(50);
    case FurnitureType::BOOKCASE_GOLD:
      return Furniture("golden bookcase", ViewObject(ViewId::BOOKCASE_GOLD, ViewLayer::FLOOR), type, tribe)
          .setBlocking()
          .setUsageType(FurnitureUsageType::STUDY)
          .setUsageTime(5_visible)
          .setCanHide()
          .setFireInfo(Fire(700, 0.5))
          .setDestroyable(50);
    case FurnitureType::THRONE:
      return Furniture("throne", ViewObject(ViewId::THRONE, ViewLayer::FLOOR), type, tribe)
          .setCanHide()
          .setUsageType(FurnitureUsageType::SIT_ON_THRONE)
          .setLuxury(1.0)
          .setDestroyable(80);
    case FurnitureType::IMPALED_HEAD:
      return Furniture("impaled head", ViewObject(ViewId::IMPALED_HEAD, ViewLayer::FLOOR), type, tribe)
          .setDestroyable(10);
    case FurnitureType::BEAST_CAGE:
      return Furniture("beast cage", ViewObject(ViewId::BEAST_CAGE, ViewLayer::FLOOR), type, tribe)
          .setUsageType(FurnitureUsageType::SLEEP)
          .setTickType(FurnitureTickType::BED)
          .setCanHide()
          .setFireInfo(Fire(500, 0.5))
          .setDestroyable(40);
    case FurnitureType::BED1:
      return Furniture("basic bed", ViewObject(ViewId::BED1, ViewLayer::FLOOR), type, tribe)
          .setUsageType(FurnitureUsageType::SLEEP)
          .setTickType(FurnitureTickType::BED)
          .setCanHide()
          .setFireInfo(Fire(500, 0.7))
          .setDestroyable(40);
    case FurnitureType::BED2:
      return Furniture("fine bed", ViewObject(ViewId::BED2, ViewLayer::FLOOR), type, tribe)
          .setUsageType(FurnitureUsageType::SLEEP)
          .setTickType(FurnitureTickType::BED)
          .setCanHide()
          .setFireInfo(Fire(500, 0.7))
          .setLuxury(0.3)
          .setDestroyable(40);
    case FurnitureType::BED3:
      return Furniture("luxurious bed", ViewObject(ViewId::BED3, ViewLayer::FLOOR), type, tribe)
          .setUsageType(FurnitureUsageType::SLEEP)
          .setTickType(FurnitureTickType::BED)
          .setCanHide()
          .setLuxury(0.7)
          .setFireInfo(Fire(500, 0.7))
          .setDestroyable(40);
    case FurnitureType::GRAVE:
      return Furniture("grave", ViewObject(ViewId::GRAVE, ViewLayer::FLOOR), type, tribe)
          .setCanHide()
          .setTickType(FurnitureTickType::BED)
          .setDestroyable(40);
    case FurnitureType::DEMON_SHRINE:
      return Furniture("demon shrine", ViewObject(ViewId::DEMON_SHRINE, ViewLayer::FLOOR), type, tribe)
          .setBlocking()
          .setCanHide()
          .setUsageTime(5_visible)
          .setLuxury(0.4)
          .setDestroyable(80);
    case FurnitureType::PRISON:
      return Furniture("prison", ViewObject(ViewId::PRISON, ViewLayer::FLOOR_BACKGROUND), type, tribe)
          .setTickType(FurnitureTickType::BED)
          .setUsageType(FurnitureUsageType::SLEEP);
    case FurnitureType::TREASURE_CHEST:
      return Furniture("treasure chest", ViewObject(ViewId::TREASURE_CHEST, ViewLayer::FLOOR), type, tribe)
          .setCanHide()
          .setFireInfo(Fire(500, 0.5))
          .setLuxury(0.4)
          .setDestroyable(40);
    case FurnitureType::EYEBALL:
      return Furniture("eyeball", ViewObject(ViewId::EYEBALL, ViewLayer::FLOOR), type, tribe)
          .setBlocking()
          .setCanHide()
          .setLightEmission(8.2)
          .setDestroyable(30);
    case FurnitureType::WHIPPING_POST:
      return Furniture("whipping post", ViewObject(ViewId::WHIPPING_POST, ViewLayer::FLOOR), type, tribe)
          .setUsageType(FurnitureUsageType::TIE_UP)
          .setFireInfo(Fire(300, 0.5))
          .setDestroyable(30);
    case FurnitureType::GALLOWS:
      return Furniture("gallows", ViewObject(ViewId::GALLOWS, ViewLayer::FLOOR), type, tribe)
          .setUsageType(FurnitureUsageType::TIE_UP)
          .setFireInfo(Fire(300, 0.5))
          .setDestroyable(30);
    case FurnitureType::MINION_STATUE:
      return Furniture("gold statue", ViewObject(ViewId::MINION_STATUE, ViewLayer::FLOOR), type, tribe)
          .setBlocking()
          .setCanHide()
          .setLuxury(0.7)
          .setDestroyable(50);
    case FurnitureType::STONE_MINION_STATUE:
      return Furniture("stone statue", ViewObject(ViewId::STONE_MINION_STATUE, ViewLayer::FLOOR), type, tribe)
          .setBlocking()
          .setCanHide()
          .setLuxury(0.3)
          .setDestroyable(50);
    case FurnitureType::BARRICADE:
      return Furniture("barricade", ViewObject(ViewId::BARRICADE, ViewLayer::FLOOR), type, tribe)
          .setBlocking()
          .setFireInfo(Fire(500, 0.3))
          .setDestroyable(80);
    case FurnitureType::CANIF_TREE:
      return Furniture("tree", ViewObject(ViewId::CANIF_TREE, ViewLayer::FLOOR), type, tribe)
          .setCanHide()
          .setBlockVision()
          .setBlockVision(VisionId::ELF, false)
          .setDestroyedRemains(FurnitureType::TREE_TRUNK)
          .setBurntRemains(FurnitureType::BURNT_TREE)
          .setDestroyable(100, DestroyAction::Type::BOULDER)
          .setDestroyable(70, DestroyAction::Type::CUT)
          .setFireInfo(Fire(1000, 0.7))
          .setItemDrop(ItemFactory::singleType(ItemType::WoodPlank{}, Range(8, 14)))
          .setSummonedElement("ENT");
    case FurnitureType::DECID_TREE:
      return Furniture("tree", ViewObject(ViewId::DECID_TREE, ViewLayer::FLOOR), type, tribe)
          .setCanHide()
          .setBlockVision()
          .setBlockVision(VisionId::ELF, false)
          .setDestroyedRemains(FurnitureType::TREE_TRUNK)
          .setBurntRemains(FurnitureType::BURNT_TREE)
          .setFireInfo(Fire(1000, 0.7))
          .setDestroyable(100, DestroyAction::Type::BOULDER)
          .setDestroyable(70, DestroyAction::Type::CUT)
          .setItemDrop(ItemFactory::singleType(ItemType::WoodPlank{}, Range(8, 14)))
          .setSummonedElement("ENT");
    case FurnitureType::TREE_TRUNK:
      return Furniture("tree trunk", ViewObject(ViewId::TREE_TRUNK, ViewLayer::FLOOR), type, tribe);
    case FurnitureType::BURNT_TREE:
      return Furniture("burnt tree", ViewObject(ViewId::BURNT_TREE, ViewLayer::FLOOR), type, tribe)
          .setCanHide()
          .setDestroyable(30);
    case FurnitureType::BUSH:
      return Furniture("bush", ViewObject(ViewId::BUSH, ViewLayer::FLOOR), type, tribe)
          .setDestroyable(20, DestroyAction::Type::BOULDER)
          .setDestroyable(10, DestroyAction::Type::CUT)
          .setCanHide()
          .setFireInfo(Fire(100, 0.8))
          .setItemDrop(ItemFactory::singleType(ItemType::WoodPlank{}, Range(2, 4)));
    case FurnitureType::CROPS:
      return Furniture("wheat", ViewObject(Random.choose(ViewId::CROPS, ViewId::CROPS2), ViewLayer::FLOOR), type, tribe)
          .setUsageType(FurnitureUsageType::CROPS)
          .setUsageTime(3_visible);
    case FurnitureType::PIGSTY:
      return Furniture("pigsty", ViewObject(ViewId::MUD, ViewLayer::FLOOR),
                       type, tribe)
          .setTickType(FurnitureTickType::PIGSTY);
    case FurnitureType::GROUND_TORCH:
      return Furniture("standing torch", ViewObject(ViewId::STANDING_TORCH, ViewLayer::FLOOR), type, tribe)
          .setLuxury(0.1)
          .setLightEmission(8.2);
    case FurnitureType::TORCH_N:
      return Furniture("torch", ViewObject(ViewId::TORCH, ViewLayer::TORCH1).setAttachmentDir(Dir::N), type, tribe)
          .setLightEmission(8.2)
          .setLayer(FurnitureLayer::CEILING);
    case FurnitureType::TORCH_E:
      return Furniture("torch", ViewObject(ViewId::TORCH, ViewLayer::TORCH2).setAttachmentDir(Dir::E), type, tribe)
          .setLightEmission(8.2)
          .setLayer(FurnitureLayer::CEILING);
    case FurnitureType::TORCH_S:
      return Furniture("torch", ViewObject(ViewId::TORCH, ViewLayer::TORCH2).setAttachmentDir(Dir::S), type, tribe)
          .setLightEmission(8.2)
          .setLayer(FurnitureLayer::CEILING);
    case FurnitureType::TORCH_W:
      return Furniture("torch", ViewObject(ViewId::TORCH, ViewLayer::TORCH2).setAttachmentDir(Dir::W), type, tribe)
          .setLightEmission(8.2)
          .setLayer(FurnitureLayer::CEILING);
    case FurnitureType::CANDELABRUM_N:
      return Furniture("candelabrum", ViewObject(ViewId::CANDELABRUM_NS, ViewLayer::TORCH1).setAttachmentDir(Dir::N), type, tribe)
          .setLightEmission(8.2)
          .setLuxury(0.3)
          .setLayer(FurnitureLayer::CEILING);
    case FurnitureType::CANDELABRUM_S:
      return Furniture("candelabrum", ViewObject(ViewId::CANDELABRUM_NS, ViewLayer::TORCH2).setAttachmentDir(Dir::S), type, tribe)
          .setLightEmission(8.2)
          .setLuxury(0.3)
          .setLayer(FurnitureLayer::CEILING);
    case FurnitureType::CANDELABRUM_E:
      return Furniture("candelabrum", ViewObject(ViewId::CANDELABRUM_E, ViewLayer::TORCH2).setAttachmentDir(Dir::E), type, tribe)
          .setLightEmission(8.2)
          .setLuxury(0.3)
          .setLayer(FurnitureLayer::CEILING);
    case FurnitureType::CANDELABRUM_W:
      return Furniture("candelabrum", ViewObject(ViewId::CANDELABRUM_W, ViewLayer::TORCH2).setAttachmentDir(Dir::W), type, tribe)
          .setLightEmission(8.2)
          .setLuxury(0.3)
          .setLayer(FurnitureLayer::CEILING);
    case FurnitureType::TORTURE_TABLE:
      return Furniture("torture table", ViewObject(ViewId::TORTURE_TABLE, ViewLayer::FLOOR), type,
          tribe)
          .setCanHide()
          .setUsageType(FurnitureUsageType::TIE_UP)
          .setDestroyable(80);
    case FurnitureType::FOUNTAIN:
      return Furniture("fountain", ViewObject(ViewId::FOUNTAIN, ViewLayer::FLOOR), type, tribe)
          .setCanHide()
          .setLuxury(0.7)
          .setUsageType(FurnitureUsageType::FOUNTAIN)
          .setSummonedElement("WATER_ELEMENTAL")
          .setDestroyable(80);
    case FurnitureType::ALTAR:
      return Furniture("altar", ViewObject(ViewId::ALTAR, ViewLayer::FLOOR), type, tribe)
          .setCanHide()
          .setUsageType(FurnitureUsageType::DESECRATE);
    case FurnitureType::ALTAR_DES:
      return Furniture("desecrated altar", ViewObject(ViewId::ALTAR_DES, ViewLayer::FLOOR), type, tribe)
          .setCanHide();
    case FurnitureType::CHEST:
      return Furniture("chest", ViewObject(ViewId::CHEST, ViewLayer::FLOOR), type, tribe)
          .setCanHide()
          .setUsageType(FurnitureUsageType::CHEST)
          .setFireInfo(Fire(500, 0.5))
          .setDestroyable(30);
    case FurnitureType::OPENED_CHEST:
      return Furniture("opened chest", ViewObject(ViewId::OPENED_CHEST, ViewLayer::FLOOR), type, tribe)
          .setCanHide()
          .setFireInfo(Fire(500, 0.5))
          .setDestroyable(30);
    case FurnitureType::COFFIN1:
      return Furniture("basic coffin", ViewObject(ViewId::COFFIN1, ViewLayer::FLOOR), type, tribe)
          .setCanHide()
          .setUsageType(FurnitureUsageType::SLEEP)
          .setFireInfo(Fire(500, 0.5))
          .setDestroyable(40);
    case FurnitureType::COFFIN2:
      return Furniture("fine coffin", ViewObject(ViewId::COFFIN2, ViewLayer::FLOOR), type, tribe)
          .setCanHide()
          .setUsageType(FurnitureUsageType::SLEEP)
          .setLuxury(0.3)
          .setFireInfo(Fire(500, 0.5))
          .setDestroyable(40);
    case FurnitureType::COFFIN3:
      return Furniture("luxurious coffin", ViewObject(ViewId::COFFIN3, ViewLayer::FLOOR), type, tribe)
          .setCanHide()
          .setUsageType(FurnitureUsageType::SLEEP)
          .setLuxury(0.7)
          .setFireInfo(Fire(500, 0.5))
          .setDestroyable(40);
    case FurnitureType::LOOT_COFFIN:
      return Furniture("coffin", ViewObject(ViewId::COFFIN1, ViewLayer::FLOOR), type, tribe)
          .setCanHide()
          .setUsageType(FurnitureUsageType::COFFIN)
          .setFireInfo(Fire(500, 0.5))
          .setDestroyable(40);
    case FurnitureType::VAMPIRE_COFFIN:
      return Furniture("coffin", ViewObject(ViewId::COFFIN1, ViewLayer::FLOOR), type, tribe)
          .setCanHide()
          .setUsageType(FurnitureUsageType::VAMPIRE_COFFIN)
          .setFireInfo(Fire(500, 0.5))
          .setDestroyable(40);
    case FurnitureType::OPENED_COFFIN:
      return Furniture("opened coffin", ViewObject(ViewId::OPENED_COFFIN, ViewLayer::FLOOR), type, tribe)
          .setCanHide()
          .setUsageType(FurnitureUsageType::SLEEP)
          .setFireInfo(Fire(500, 0.5))
          .setDestroyable(40);
    case FurnitureType::WOOD_DOOR:
      return Furniture("wooden door", ViewObject(ViewId::WOOD_DOOR, ViewLayer::FLOOR), type, tribe)
          .setBlockingEnemies()
          .setCanHide()
          .setBlockVision()
          .setFireInfo(Fire(500, 0.5))
          .setDestroyable(80)
          .setClickType(FurnitureClickType::LOCK);
    case FurnitureType::IRON_DOOR:
      return Furniture("iron door", ViewObject(ViewId::IRON_DOOR, ViewLayer::FLOOR), type, tribe)
          .setBlockingEnemies()
          .setCanHide()
          .setBlockVision()
          .setDestroyable(160)
          .setClickType(FurnitureClickType::LOCK);
    case FurnitureType::ADA_DOOR:
      return Furniture("adamantine door", ViewObject(ViewId::ADA_DOOR, ViewLayer::FLOOR), type, tribe)
          .setBlockingEnemies()
          .setCanHide()
          .setBlockVision()
          .setDestroyable(320)
          .setClickType(FurnitureClickType::LOCK);
    case FurnitureType::WELL:
      return Furniture("well", ViewObject(ViewId::WELL, ViewLayer::FLOOR), type, tribe)
          .setCanHide()
          .setFireInfo(Fire(500, 0.5))
          .setSummonedElement("WATER_ELEMENTAL")
          .setDestroyable(80);
    case FurnitureType::KEEPER_BOARD:
      return Furniture("message board", ViewObject(ViewId::NOTICE_BOARD, ViewLayer::FLOOR), type, tribe)
          .setCanHide()
          .setUsageType(FurnitureUsageType::KEEPER_BOARD)
          .setClickType(FurnitureClickType::KEEPER_BOARD)
          .setFireInfo(Fire(500, 0.5))
          .setDestroyable(50);
    case FurnitureType::UP_STAIRS:
      return Furniture("stairs", ViewObject(ViewId::UP_STAIRCASE, ViewLayer::FLOOR), type, tribe)
          .setCanHide()
          .setUsageType(FurnitureUsageType::STAIRS);
    case FurnitureType::DOWN_STAIRS:
      return Furniture("stairs", ViewObject(ViewId::DOWN_STAIRCASE, ViewLayer::FLOOR), type, tribe)
          .setCanHide()
          .setOnBuilt(FurnitureOnBuilt::DOWN_STAIRS)
          .setUsageType(FurnitureUsageType::STAIRS);
    case FurnitureType::SOKOBAN_HOLE:
      return Furniture("hole", ViewObject(ViewId::SOKOBAN_HOLE, ViewLayer::FLOOR), type, tribe)
          .setEntryType(FurnitureEntry(FurnitureEntry::Sokoban{}));
    case FurnitureType::BRIDGE:
      return Furniture("bridge", ViewObject(ViewId::BRIDGE, ViewLayer::FLOOR), type, tribe)
          .setOverrideMovement()
          .setCanRemoveNonFriendly(true)
          .setCanRemoveWithCreaturePresent(false);
    case FurnitureType::ROAD:
      return Furniture("road", ViewObject(ViewId::ROAD, ViewLayer::FLOOR), type, tribe);
    case FurnitureType::MOUNTAIN:
      return Furniture("soft rock", ViewObject(ViewId::MOUNTAIN, ViewLayer::FLOOR), type, tribe)
          .setBlocking()
          .setBlockVision()
          .setConstructMessage(Furniture::FILL_UP)
          .setIsWall()
          .setForgetAfterBuilding()
          .setDestroyable(200, DestroyAction::Type::BOULDER)
          .setDestroyable(30, DestroyAction::Type::DIG)
          .setDestroyable(200, DestroyAction::Type::HOSTILE_DIG)
          .setDestroyable(2000, DestroyAction::Type::HOSTILE_DIG_NO_SKILL)
          .setSummonedElement("EARTH_ELEMENTAL");
    case FurnitureType::MOUNTAIN2:
      return Furniture("hard rock", ViewObject(ViewId::MOUNTAIN2, ViewLayer::FLOOR), type, tribe)
          .setBlocking()
          .setBlockVision()
          .setConstructMessage(Furniture::FILL_UP)
          .setIsWall()
          .setForgetAfterBuilding()
          .setDestroyable(500, DestroyAction::Type::BOULDER)
          .setDestroyable(70, DestroyAction::Type::DIG)
          .setDestroyable(500, DestroyAction::Type::HOSTILE_DIG)
          .setDestroyable(2000, DestroyAction::Type::HOSTILE_DIG_NO_SKILL)
          .setSummonedElement("EARTH_ELEMENTAL");
    case FurnitureType::ADAMANTIUM_ORE:
      return Furniture("adamantium ore", ViewObject(ViewId::ADAMANTIUM_ORE, ViewLayer::FLOOR), type, tribe)
          .setBlocking()
          .setBlockVision()
          .setIsWall()
          .setClearFogOfWar()
          .setItemDrop(ItemFactory::singleType(ItemType::AdaOre{}, Range(16, 28)))
          .setDestroyable(500, DestroyAction::Type::BOULDER)
          .setDestroyable(500, DestroyAction::Type::DIG)
          .setDestroyable(500, DestroyAction::Type::HOSTILE_DIG)
          .setDestroyable(2000, DestroyAction::Type::HOSTILE_DIG_NO_SKILL);
    case FurnitureType::IRON_ORE:
      return Furniture("iron ore", ViewObject(ViewId::IRON_ORE, ViewLayer::FLOOR), type, tribe)
          .setBlocking()
          .setBlockVision()
          .setIsWall()
          .setClearFogOfWar()
          .setItemDrop(ItemFactory::singleType(ItemType::IronOre{}, Range(16, 28)))
          .setDestroyable(200, DestroyAction::Type::BOULDER)
          .setDestroyable(220, DestroyAction::Type::DIG)
          .setDestroyable(200, DestroyAction::Type::HOSTILE_DIG)
          .setDestroyable(2000, DestroyAction::Type::HOSTILE_DIG_NO_SKILL);
    case FurnitureType::STONE:
      return Furniture("granite", ViewObject(ViewId::STONE, ViewLayer::FLOOR), type, tribe)
          .setBlocking()
          .setBlockVision()
          .setIsWall()
          .setClearFogOfWar()
          .setItemDrop(ItemFactory::singleType(ItemType::Rock{}, Range(16, 28)))
          .setDestroyable(200, DestroyAction::Type::BOULDER)
          .setDestroyable(250, DestroyAction::Type::DIG)
          .setDestroyable(200, DestroyAction::Type::HOSTILE_DIG)
          .setDestroyable(2000, DestroyAction::Type::HOSTILE_DIG_NO_SKILL);
    case FurnitureType::GOLD_ORE:
      return Furniture("gold ore", ViewObject(ViewId::GOLD_ORE, ViewLayer::FLOOR), type, tribe)
          .setBlocking()
          .setBlockVision()
          .setIsWall()
          .setClearFogOfWar()
          .setItemDrop(ItemFactory::singleType(ItemType::GoldPiece{}, Range(16, 28)))
          .setDestroyable(200, DestroyAction::Type::BOULDER)
          .setDestroyable(220, DestroyAction::Type::DIG)
          .setDestroyable(200, DestroyAction::Type::HOSTILE_DIG)
          .setDestroyable(2000, DestroyAction::Type::HOSTILE_DIG_NO_SKILL);
    case FurnitureType::DUNGEON_WALL:
      return Furniture("wall", ViewObject(ViewId::DUNGEON_WALL, ViewLayer::FLOOR), type, tribe)
          .setBlocking()
          .setBlockVision()
          .setIsWall()
          .setForgetAfterBuilding()
          .setLuxury(0.2)
          .setConstructMessage(Furniture::REINFORCE)
          .setDestroyable(300, DestroyAction::Type::BOULDER)
          .setDestroyable(100, DestroyAction::Type::DIG)
          .setDestroyable(1900, DestroyAction::Type::HOSTILE_DIG)
          .setDestroyable(2000, DestroyAction::Type::HOSTILE_DIG_NO_SKILL);
    case FurnitureType::DUNGEON_WALL2:
      return Furniture("wall", ViewObject(ViewId::DUNGEON_WALL2, ViewLayer::FLOOR), type, tribe)
          .setBlocking()
          .setBlockVision()
          .setIsWall()
          .setForgetAfterBuilding()
          .setLuxury(0.2)
          .setConstructMessage(Furniture::REINFORCE)
          .setDestroyable(300, DestroyAction::Type::BOULDER)
          .setDestroyable(100, DestroyAction::Type::DIG)
          .setDestroyable(1900, DestroyAction::Type::HOSTILE_DIG)
          .setDestroyable(2000, DestroyAction::Type::HOSTILE_DIG_NO_SKILL);
    case FurnitureType::PIT:
      return Furniture("pit", ViewObject(ViewId::PIT, ViewLayer::FLOOR), type, tribe)
          .setBlocking()
          .setForgetAfterBuilding()
          .setTickType(FurnitureTickType::PIT);
    case FurnitureType::CASTLE_WALL:
      return Furniture("wall", ViewObject(ViewId::CASTLE_WALL, ViewLayer::FLOOR), type, tribe)
          .setBlocking()
          .setBlockVision()
          .setIsBuildingSupport()
          .setIsWall()
          .setDestroyable(300, DestroyAction::Type::BOULDER)
          .setDestroyable(100, DestroyAction::Type::DIG)
          .setDestroyable(1900, DestroyAction::Type::HOSTILE_DIG)
          .setDestroyable(2000, DestroyAction::Type::HOSTILE_DIG_NO_SKILL);
    case FurnitureType::WOOD_WALL:
      return Furniture("wall", ViewObject(ViewId::WOOD_WALL, ViewLayer::FLOOR), type, tribe)
          .setBlocking()
          .setBlockVision()
          .setIsWall()
          .setIsBuildingSupport()
          .setDestroyable(100, DestroyAction::Type::BOULDER)
          .setDestroyable(100, DestroyAction::Type::DIG)
          .setDestroyable(300, DestroyAction::Type::HOSTILE_DIG)
          .setDestroyable(300, DestroyAction::Type::HOSTILE_DIG_NO_SKILL)
          .setSummonedElement("ENT")
          .setFireInfo(Fire(1000, 0.7));
    case FurnitureType::MUD_WALL:
      return Furniture("wall", ViewObject(ViewId::MUD_WALL, ViewLayer::FLOOR), type, tribe)
          .setBlocking()
          .setBlockVision()
          .setIsWall()
          .setIsBuildingSupport()
          .setDestroyable(100, DestroyAction::Type::BOULDER);
    case FurnitureType::RUIN_WALL:
      return Furniture("wall", ViewObject(ViewId::RUIN_WALL, ViewLayer::FLOOR), type, tribe)
          .setBlocking()
          .setBlockVision()
          .setIsWall()
          .setDestroyable(100, DestroyAction::Type::BOULDER);
    case FurnitureType::FLOOR_WOOD1:
      return Furniture("floor", ViewObject(ViewId::WOOD_FLOOR2, ViewLayer::FLOOR_BACKGROUND), type, tribe)
          .setFireInfo(Fire(500, 0.5))
          .setLuxury(0.1)
          .setLayer(FurnitureLayer::FLOOR);
    case FurnitureType::FLOOR_WOOD2:
      return Furniture("floor", ViewObject(ViewId::WOOD_FLOOR4, ViewLayer::FLOOR_BACKGROUND), type, tribe)
          .setFireInfo(Fire(500, 0.5))
          .setLuxury(0.1)
          .setLayer(FurnitureLayer::FLOOR);
    case FurnitureType::FLOOR_STONE1:
      return Furniture("floor", ViewObject(ViewId::STONE_FLOOR1, ViewLayer::FLOOR_BACKGROUND), type, tribe)
          .setLuxury(0.3)
          .setLayer(FurnitureLayer::FLOOR);
    case FurnitureType::FLOOR_STONE2:
      return Furniture("floor", ViewObject(ViewId::STONE_FLOOR5, ViewLayer::FLOOR_BACKGROUND), type, tribe)
          .setLuxury(0.3)
          .setLayer(FurnitureLayer::FLOOR);
    case FurnitureType::FLOOR_CARPET1:
      return Furniture("floor", ViewObject(ViewId::CARPET_FLOOR1, ViewLayer::FLOOR_BACKGROUND), type, tribe)
          .setLuxury(0.7)
          .setLayer(FurnitureLayer::FLOOR);
    case FurnitureType::FLOOR_CARPET2:
      return Furniture("floor", ViewObject(ViewId::CARPET_FLOOR4, ViewLayer::FLOOR_BACKGROUND), type, tribe)
          .setLuxury(0.7)
          .setLayer(FurnitureLayer::FLOOR);
    case FurnitureType::ALARM_TRAP:
      return Furniture("alarm trap", ViewObject(ViewId::ALARM_TRAP, ViewLayer::FLOOR), type, tribe)
          .setEntryType(FurnitureEntry(FurnitureEntry::Trap(Effect::Alarm{})))
          .setEmitsWarning()
          .setConstructMessage(Furniture::SET_UP);
    case FurnitureType::INVISIBLE_ALARM:
      return Furniture("", ViewObject(ViewId::EMPTY, ViewLayer::FLOOR), type, tribe)
          .setEntryType(FurnitureEntry(FurnitureEntry::Trap(Effect::Alarm{true}, true)));
    case FurnitureType::POISON_GAS_TRAP:
      return Furniture("poison gas trap", ViewObject(ViewId::GAS_TRAP, ViewLayer::FLOOR), type, tribe)
          .setEntryType(FurnitureEntry(FurnitureEntry::Trap(Effect::EmitPoisonGas{})))
          .setEmitsWarning()
          .setConstructMessage(Furniture::SET_UP);
    case FurnitureType::WEB_TRAP:
      return Furniture("web trap", ViewObject(ViewId::WEB_TRAP, ViewLayer::FLOOR), type, tribe)
          .setEntryType(FurnitureEntry(FurnitureEntry::Trap(Effect::Lasting{LastingEffect::ENTANGLED})))
          .setEmitsWarning()
          .setConstructMessage(Furniture::SET_UP);
    case FurnitureType::SPIDER_WEB:
      return Furniture("spider web", ViewObject(ViewId::WEB_TRAP, ViewLayer::FLOOR), type, tribe)
          .setEntryType(FurnitureEntry(FurnitureEntry::Trap(Effect::Lasting{LastingEffect::ENTANGLED},
              true)));
    case FurnitureType::SURPRISE_TRAP:
      return Furniture("surprise trap", ViewObject(ViewId::SURPRISE_TRAP, ViewLayer::FLOOR), type, tribe)
          .setEntryType(FurnitureEntry(FurnitureEntry::Trap(Effect::TeleEnemies{})))
          .setEmitsWarning()
          .setConstructMessage(Furniture::SET_UP);
    case FurnitureType::TERROR_TRAP:
      return Furniture("panic trap", ViewObject(ViewId::TERROR_TRAP, ViewLayer::FLOOR), type, tribe)
          .setEntryType(FurnitureEntry(FurnitureEntry::Trap(Effect::Lasting{LastingEffect::PANIC})))
          .setEmitsWarning()
          .setConstructMessage(Furniture::SET_UP);
    case FurnitureType::BOULDER_TRAP:
      return Furniture("boulder trap", ViewObject(ViewId::BOULDER, ViewLayer::CREATURE), type, tribe)
          .setBlocking()
          .setCanHide()
          .setTickType(FurnitureTickType::BOULDER_TRAP)
          .setDestroyable(40)
          .setEmitsWarning()
          .setConstructMessage(Furniture::SET_UP);
    case FurnitureType::PORTAL:
      return Furniture("portal", ViewObject(ViewId::PORTAL, ViewLayer::FLOOR), type, tribe)
          .setDestroyable(40)
          .setUsageType(FurnitureUsageType::PORTAL)
          .setTickType(FurnitureTickType::PORTAL);
    case FurnitureType::METEOR_SHOWER:
      return Furniture("meteor shower", none, type, tribe)
          .setLayer(FurnitureLayer::CEILING)
          .setConstructMessage(none)
          .setTickType(FurnitureTickType::METEOR_SHOWER);
    case FurnitureType::WATER: {
      auto ret = Furniture("water", ViewObject(ViewId::WATER, ViewLayer::FLOOR_BACKGROUND)
              .setAttribute(ViewObject::Attribute::WATER_DEPTH, 100.0), type, tribe)
          .setLayer(FurnitureLayer::GROUND)
          .setEntryType(FurnitureEntry::Water{})
          .setDroppedItems(FurnitureDroppedItems::Water{"sinks", "sink", "You hear a splash."_s})
          .setCanBuildBridgeOver()
          .setTickType(FurnitureTickType::EXTINGUISH_FIRE)
          .setSummonedElement("WATER_ELEMENTAL");
      ret.modMovementSet()
          .clearTraits()
          .addTrait(MovementTrait::FLY)
          .addTrait(MovementTrait::SWIM)
          .addForcibleTrait(MovementTrait::WALK);
      return ret;
    }
    case FurnitureType::SHALLOW_WATER1: {
      auto ret = Furniture("shallow water", ViewObject(ViewId::WATER, ViewLayer::FLOOR_BACKGROUND)
              .setAttribute(ViewObject::Attribute::WATER_DEPTH, 1.5), type, tribe)
          .setLayer(FurnitureLayer::GROUND)
          .setEntryType(FurnitureEntry::Water{})
          .setCanBuildBridgeOver()
          .setTickType(FurnitureTickType::EXTINGUISH_FIRE)
          .setSummonedElement("WATER_ELEMENTAL");
      ret.modMovementSet()
          .clearTraits()
          .addForcibleTrait(MovementTrait::WALK)
          .addTrait(MovementTrait::FLY)
          .addTrait(MovementTrait::SWIM)
          .addTrait(MovementTrait::WADE);
      return ret;
    }
    case FurnitureType::SHALLOW_WATER2: {
      auto ret = Furniture("shallow water", ViewObject(ViewId::WATER, ViewLayer::FLOOR_BACKGROUND)
              .setAttribute(ViewObject::Attribute::WATER_DEPTH, 0.5), type, tribe)
          .setLayer(FurnitureLayer::GROUND)
          .setEntryType(FurnitureEntry::Water{})
          .setCanBuildBridgeOver()
          .setTickType(FurnitureTickType::EXTINGUISH_FIRE)
          .setSummonedElement("WATER_ELEMENTAL");
      ret.modMovementSet()
          .clearTraits()
          .addForcibleTrait(MovementTrait::WALK)
          .addTrait(MovementTrait::FLY)
          .addTrait(MovementTrait::SWIM)
          .addTrait(MovementTrait::WADE);
      return ret;
    }
    case FurnitureType::MAGMA: {
      auto ret = Furniture("magma", ViewObject(ViewId::MAGMA, ViewLayer::FLOOR_BACKGROUND), type, tribe)
          .setLayer(FurnitureLayer::GROUND)
          .setEntryType(FurnitureEntry::Magma{})
          .setDroppedItems(FurnitureDroppedItems::Water{"burns", "burn", none})
          .setLightEmission(8.2)
          .setCanBuildBridgeOver()
          .setSummonedElement("FIRE_ELEMENTAL");
      ret.modMovementSet()
          .clearTraits()
          .addTrait(MovementTrait::FLY)
          .addForcibleTrait(MovementTrait::WALK);
      return ret;
    }
    case FurnitureType::SAND:
      return Furniture("sand", ViewObject(ViewId::SAND, ViewLayer::FLOOR_BACKGROUND), type, tribe)
          .setLayer(FurnitureLayer::GROUND);
    case FurnitureType::GRASS:
      return Furniture("grass", ViewObject(ViewId::GRASS, ViewLayer::FLOOR_BACKGROUND), type, tribe)
          .setLayer(FurnitureLayer::GROUND);
    case FurnitureType::MUD:
      return Furniture("mud", ViewObject(ViewId::MUD, ViewLayer::FLOOR_BACKGROUND), type, tribe)
          .setLayer(FurnitureLayer::GROUND);
    case FurnitureType::HILL:
      return Furniture("hill", ViewObject(ViewId::HILL, ViewLayer::FLOOR_BACKGROUND), type, tribe)
          .setLayer(FurnitureLayer::GROUND);
    case FurnitureType::FLOOR:
      return Furniture("floor", ViewObject(ViewId::FLOOR, ViewLayer::FLOOR_BACKGROUND), type, tribe)
          .setLayer(FurnitureLayer::GROUND);
    case FurnitureType::BLACK_FLOOR:
      return Furniture("floor", ViewObject(ViewId::EMPTY, ViewLayer::FLOOR_BACKGROUND), type, tribe)
          .setLayer(FurnitureLayer::GROUND);
    case FurnitureType::TUTORIAL_ENTRANCE:
      return Furniture("tutorial entrance", ViewObject(ViewId::TUTORIAL_ENTRANCE, ViewLayer::TORCH2), type, tribe)
          .setLayer(FurnitureLayer::CEILING);
  }
}

bool FurnitureParams::operator == (const FurnitureParams& p) const {

  return type == p.type && tribe == p.tribe;
}

bool FurnitureFactory::hasSupport(FurnitureType type, Position pos) {
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

bool FurnitureFactory::canBuild(FurnitureType type, Position pos) {
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
      auto original = pos.getFurniture(Furniture::getLayer(type));
      return pos.getFurniture(FurnitureLayer::GROUND)->getMovementSet().canEnter({MovementTrait::WALK}) &&
          (!original || canSilentlyReplace(original->getType())) && !pos.isWall();
    }
  }
}

bool FurnitureFactory::isUpgrade(FurnitureType base, FurnitureType upgraded) {
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
  return makeOwner<Furniture>(::get(type, tribe));
}

FurnitureFactory FurnitureFactory::roomFurniture(TribeId tribe) {
  return FurnitureFactory(tribe, {
      {FurnitureType::BED1, 2},
      {FurnitureType::GROUND_TORCH, 1},
      {FurnitureType::CHEST, 2}
  });
}

FurnitureFactory FurnitureFactory::castleFurniture(TribeId tribe) {
  return FurnitureFactory(tribe, {
      {FurnitureType::BED1, 2},
      {FurnitureType::GROUND_TORCH, 1},
      {FurnitureType::FOUNTAIN, 1},
      {FurnitureType::CHEST, 2}
  });
}

FurnitureFactory FurnitureFactory::dungeonOutside(TribeId tribe) {
  return FurnitureFactory(tribe, {
      {FurnitureType::GROUND_TORCH, 1},
  });
}

FurnitureFactory FurnitureFactory::castleOutside(TribeId tribe) {
  return FurnitureFactory(tribe, {
      {FurnitureType::INVISIBLE_ALARM, 10},
      {FurnitureType::GROUND_TORCH, 1},
      {FurnitureType::WELL, 1},
  });
}

FurnitureFactory FurnitureFactory::villageOutside(TribeId tribe) {
  return FurnitureFactory(tribe, {
      {FurnitureType::GROUND_TORCH, 1},
      {FurnitureType::WELL, 1},
  });
}

FurnitureFactory FurnitureFactory::cryptCoffins(TribeId tribe) {
  return FurnitureFactory(tribe, {
      {FurnitureType::LOOT_COFFIN, 1},
  }, {
      FurnitureType::VAMPIRE_COFFIN
  });
}

FurnitureType FurnitureFactory::getWaterType(double depth) {
  if (depth >= 2.0)
    return FurnitureType::WATER;
  else if (depth >= 1.0)
    return FurnitureType::SHALLOW_WATER1;
  else
    return FurnitureType::SHALLOW_WATER2;
}

FurnitureParams FurnitureFactory::getRandom(RandomGen& random) {
  if (!unique.empty()) {
    FurnitureType f = unique.back();
    unique.pop_back();
    return {f, *tribe};
  } else
    return {random.choose(distribution), *tribe};
}

