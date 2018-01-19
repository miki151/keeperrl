#include "stdafx.h"
#include "build_info.h"
#include "furniture_type.h"
#include "tutorial_highlight.h"
#include "collective.h"
#include "collective_config.h"
#include "game.h"
#include "villain_type.h"
#include "furniture_layer.h"
#include "view_id.h"
#include "technology.h"
#include "experience_type.h"
#include "model_builder.h"
#include "trap_type.h"
#include "quarters.h"

using ResourceId = Collective::ResourceId;

const vector<BuildInfo>& BuildInfo::get() {
  const string workshop = "Manufactories";
  static optional<vector<BuildInfo>> buildInfo;
  if (!buildInfo) {
    buildInfo = vector<BuildInfo>({
      BuildInfo(BuildInfo::DIG, "Dig or cut tree", "", 'd').setTutorialHighlight(TutorialHighlight::DIG_OR_CUT_TREES),
      BuildInfo({FurnitureType::MOUNTAIN, {ResourceId::STONE, 10}}, "Fill up tunnel", {},
          "Fill up one tile at a time. Cutting off an area is not allowed.", 0, "Structure"),
      BuildInfo({{FurnitureType::DUNGEON_WALL, FurnitureType::DUNGEON_WALL2}, {ResourceId::STONE, 2}}, "Reinforce wall", {},
          "Reinforce wall. +" + toString<int>(100 * CollectiveConfig::getEfficiencyBonus(FurnitureType::DUNGEON_WALL)) +
          " efficiency to surrounding tiles.", 0, "Structure"),
      BuildInfo({FurnitureType::WOOD_DOOR, {ResourceId::WOOD, 5}}, "Wooden door", {},
          "Click on a built door to lock it.", 'o', "Doors", true)
             .setTutorialHighlight(TutorialHighlight::BUILD_DOOR),
      BuildInfo({FurnitureType::IRON_DOOR, {ResourceId::IRON, 5}}, "Iron door", {},
          "Click on a built door to lock it.", 0, "Doors"),
      BuildInfo({FurnitureType::STEEL_DOOR, {ResourceId::STEEL, 5}}, "Steel door", {},
          "Click on a built door to lock it.", 0, "Doors"),
    });
    for (int i : All(CollectiveConfig::getFloors())) {
      auto& floor = CollectiveConfig::getFloors()[i];
      string efficiency = toString<int>(floor.efficiencyBonus * 100);
      buildInfo->push_back(
            BuildInfo({floor.type, floor.cost},
                floor.name + "  (+" + efficiency + ")",
                {}, floor.name + " floor. +" + efficiency + " efficiency to surrounding tiles.", i == 0 ? 'f' : 0,
                      "Floors", i == 0));
      if (floor.type == FurnitureType::FLOOR_WOOD1 || floor.type == FurnitureType::FLOOR_WOOD2)
        buildInfo->back().setTutorialHighlight(TutorialHighlight::BUILD_FLOOR);
    }
    append(*buildInfo, {
             BuildInfo({FurnitureLayer::FLOOR}, "Remove floor", "", 0, "Floors"),
      BuildInfo(ZoneId::STORAGE_RESOURCES, ViewId::STORAGE_RESOURCES, "Resources",
          "Only wood, iron and granite can be stored here.", 's', "Storage", true)
             .setTutorialHighlight(TutorialHighlight::RESOURCE_STORAGE),
      BuildInfo(ZoneId::STORAGE_EQUIPMENT, ViewId::STORAGE_EQUIPMENT, "Equipment",
          "All equipment for your minions can be stored here.", 0, "Storage")
             .setTutorialHighlight(TutorialHighlight::EQUIPMENT_STORAGE),
      BuildInfo({FurnitureType::TREASURE_CHEST, {ResourceId::WOOD, 5}}, "Treasure chest", {},
          "Stores gold.", 0, "Storage"),
    });
    auto& quarters = Quarters::getAllQuarters();
    for (int i : All(quarters))
      buildInfo->emplace_back(
        quarters[i].zone, quarters[i].viewId, "Quarters "_s + toString(i),
            "Designate separate quarters for chosen minions.", i == 0 ? 'q' : '\0', "Quarters", true);
    append(*buildInfo, {
      BuildInfo({FurnitureType::BOOKCASE_WOOD, {ResourceId::WOOD, 15}}, "Wooden bookcase",
          {{RequirementId::TECHNOLOGY, TechId::SPELLS}}, "Train your minions here. Adds up to " +
          toString(*CollectiveConfig::getTrainingMaxLevel(ExperienceType::SPELL, FurnitureType::BOOKCASE_WOOD)) + " spell levels.",
          'y', "Library", true).setTutorialHighlight(TutorialHighlight::BUILD_LIBRARY),
      BuildInfo({FurnitureType::BOOKCASE_IRON, {ResourceId::IRON, 15}}, "Iron bookcase",
          {{RequirementId::TECHNOLOGY, TechId::SPELLS_ADV}}, "Train your minions here. Adds up to " +
          toString(*CollectiveConfig::getTrainingMaxLevel(ExperienceType::SPELL, FurnitureType::BOOKCASE_IRON)) + " spell levels.",
          0, "Library"),
      BuildInfo({FurnitureType::BOOKCASE_GOLD, {ResourceId::GOLD, 15}}, "Golden bookcase",
          {{RequirementId::TECHNOLOGY, TechId::SPELLS_MAS}}, "Train your minions here. Adds up to " +
          toString(*CollectiveConfig::getTrainingMaxLevel(ExperienceType::SPELL, FurnitureType::BOOKCASE_GOLD)) + " spell levels.",
          0, "Library"),
      BuildInfo({FurnitureType::THRONE, {ResourceId::MANA, 300}, false, 1}, "Throne",
          {{RequirementId::VILLAGE_CONQUERED}},
          "Increases population limit by " + toString(ModelBuilder::getThronePopulationIncrease())),
      BuildInfo({FurnitureType::BED, {ResourceId::WOOD, 12}}, "Bed", {},
          "Humanoid minions sleep here.", 'b', "Living", true)
             .setTutorialHighlight(TutorialHighlight::BUILD_BED),
      BuildInfo({FurnitureType::GRAVE, {ResourceId::STONE, 15}}, "Graveyard", {},
          "Spot for hauling dead bodies and for undead creatures to sleep in.", 0, "Living"),
      BuildInfo({FurnitureType::BEAST_CAGE, {ResourceId::WOOD, 8}}, "Beast cage", {}, "Beasts sleep here.", 0, "Living"),
      BuildInfo({FurnitureType::PIGSTY, {ResourceId::WOOD, 5}}, "Pigsty",
          {{RequirementId::TECHNOLOGY, TechId::PIGSTY}},
          "Increases minion population limit by up to " +
          toString(ModelBuilder::getPigstyPopulationIncrease()) + ".", 0, "Living"),
      BuildInfo({FurnitureType::TRAINING_WOOD, {ResourceId::WOOD, 12}}, "Wooden dummy", {},
          "Train your minions here. Adds up to " +
          toString(*CollectiveConfig::getTrainingMaxLevel(ExperienceType::MELEE, FurnitureType::TRAINING_WOOD)) + " melee levels.",
          't', "Training room", true)
             .setTutorialHighlight(TutorialHighlight::TRAINING_ROOM),
      BuildInfo({FurnitureType::TRAINING_IRON, {ResourceId::IRON, 12}}, "Iron dummy",
          {{RequirementId::TECHNOLOGY, TechId::IRON_WORKING}},
          "Train your minions here. Adds up to " +
          toString(*CollectiveConfig::getTrainingMaxLevel(ExperienceType::MELEE, FurnitureType::TRAINING_IRON)) + " melee levels.",
          0, "Training room"),
      BuildInfo({FurnitureType::TRAINING_STEEL, {ResourceId::STEEL, 12}}, "Steel dummy",
          {{RequirementId::TECHNOLOGY, TechId::STEEL_MAKING}},
          "Train your minions here. Adds up to " +
          toString(*CollectiveConfig::getTrainingMaxLevel(ExperienceType::MELEE, FurnitureType::TRAINING_STEEL)) + " melee levels.",
          0, "Training room"),
      BuildInfo({FurnitureType::ARCHERY_RANGE, {ResourceId::WOOD, 12}}, "Archery target",
          {{RequirementId::TECHNOLOGY, TechId::ARCHERY}},
          "Train your minions here.", 0, "Training room"),
      BuildInfo({FurnitureType::WORKSHOP, {ResourceId::WOOD, 15}}, "Workshop", {},
          "Produces leather equipment, traps, first-aid kits and other.", 'm', workshop, true)
             .setTutorialHighlight(TutorialHighlight::BUILD_WORKSHOP),
      BuildInfo({FurnitureType::FORGE, {ResourceId::IRON, 20}}, "Forge",
          {{RequirementId::TECHNOLOGY, TechId::IRON_WORKING}}, "Produces iron weapons and armor.", 0, workshop),
      BuildInfo({FurnitureType::LABORATORY, {ResourceId::STONE, 10}}, "Laboratory",
          {{RequirementId::TECHNOLOGY, TechId::ALCHEMY}}, "Produces magical potions.", 0, workshop),
      BuildInfo({FurnitureType::JEWELER, {ResourceId::WOOD, 12}}, "Jeweler",
          {{RequirementId::TECHNOLOGY, TechId::JEWELLERY}}, "Produces magical rings and amulets.", 0, workshop),
      BuildInfo({FurnitureType::STEEL_FURNACE, {ResourceId::STONE, 100}}, "Steel furnace",
          {{RequirementId::TECHNOLOGY, TechId::STEEL_MAKING}}, "Turns iron ore into steel.", 0, workshop),
      BuildInfo({FurnitureType::DEMON_SHRINE, {ResourceId::GOLD, 30}}, "Demon shrine", {{RequirementId::TECHNOLOGY, TechId::DEMONOLOGY}},
          "Summons various demons to your dungeon."),
      BuildInfo({FurnitureType::PRISON, {ResourceId::IRON, 5}}, "Prison", {}, "Captured enemies are kept here.",
          'p', "Prison", true),
      BuildInfo({FurnitureType::TORTURE_TABLE, {ResourceId::IRON, 20}}, "Torture table", {},
          "Can be used to torture prisoners.", 0, "Prison"),
      BuildInfo(BuildInfo::CLAIM_TILE, "Claim tile", "Claim a tile. Building anything has the same effect.", 0, "Orders"),
      BuildInfo(ZoneId::FETCH_ITEMS, ViewId::FETCH_ICON, "Fetch items",
          "Order imps to fetch items from locations outside the dungeon. This is a one-time order.", 0, "Orders"),
      BuildInfo(ZoneId::PERMANENT_FETCH_ITEMS, ViewId::FETCH_ICON, "Fetch items persistently",
          "Order imps to fetch items from locations outside the dungeon. This is a persistent order.", 0,
          "Orders"),
      BuildInfo(BuildInfo::DISPATCH, "Prioritize task", "Click on an existing task to give it a high priority.", 'a',
          "Orders"),
      BuildInfo({FurnitureLayer::CEILING, FurnitureLayer::MIDDLE}, "Remove construction", "", 'e', "Orders")
          .setTutorialHighlight(TutorialHighlight::REMOVE_CONSTRUCTION),
      BuildInfo(BuildInfo::FORBID_ZONE, "Forbid zone", "Mark tiles to keep minions from entering.", 0, "Orders"),
      BuildInfo({FurnitureType::BRIDGE, {ResourceId::WOOD, 5}}, "Bridge", {},
        "Build it to pass over water or lava.", 0, "Installations"),
      BuildInfo({FurnitureType::BARRICADE, {ResourceId::WOOD, 5}}, "Barricade", {}, "", 0, "Installations"),
      BuildInfo(BuildInfo::FurnitureInfo(
             {FurnitureType::TORCH_N, FurnitureType::TORCH_E, FurnitureType::TORCH_S, FurnitureType::TORCH_W}),
             "Torch", {}, "Place it on tiles next to a wall.", 'c', "Installations")
          .setTutorialHighlight(TutorialHighlight::BUILD_TORCH),
      BuildInfo(BuildInfo::FurnitureInfo(FurnitureType::GROUND_TORCH),
             "Standing torch", {}, "", 0, "Installations")
          .setTutorialHighlight(TutorialHighlight::BUILD_TORCH),
      BuildInfo({FurnitureType::KEEPER_BOARD, {ResourceId::WOOD, 15}}, "Message board", {},
          "A board where you can leave a message for other players.", 0, "Installations"),
      BuildInfo({FurnitureType::EYEBALL, {ResourceId::WOOD, 30}}, "Eyeball", {},
        "Makes the area around it visible.", 0, "Installations"),
      BuildInfo({FurnitureType::PORTAL, {ResourceId::STONE, 60}}, "Portal", {},
        "Opens a connection if another portal is present.", 0, "Installations"),
      BuildInfo({FurnitureType::MINION_STATUE, {ResourceId::MANA, 50}}, "Statue", {},
        "Increases minion population limit by " +
              toString(ModelBuilder::getStatuePopulationIncrease()) + ".", 0, "Installations"),
      BuildInfo({FurnitureType::WHIPPING_POST, {ResourceId::WOOD, 20}}, "Whipping post", {},
          "A place to whip your minions if they need a morale boost.", 0, "Installations"),
      BuildInfo({FurnitureType::GALLOWS, {ResourceId::WOOD, 20}}, "Gallows", {},
          "For hanging prisoners.", 0, "Installations"),
      BuildInfo({FurnitureType::IMPALED_HEAD, {ResourceId::PRISONER_HEAD, 1}, true}, "Prisoner head", {},
          "Impaled head of an executed prisoner. Aggravates enemies.", 0, "Installations"),
#ifndef RELEASE
      BuildInfo({FurnitureType::TUTORIAL_ENTRANCE, {}}, "Tutorial entrance", {}, "", 0, "Installations"),
#endif
      BuildInfo({TrapType::TERROR, ViewId::TERROR_TRAP}, "Panic trap", {{RequirementId::TECHNOLOGY, TechId::TRAPS}},
          "Causes the trespasser to panic.", 0, "Traps"),
      BuildInfo({TrapType::POISON_GAS, ViewId::GAS_TRAP}, "Gas trap", {{RequirementId::TECHNOLOGY, TechId::TRAPS}},
          "Releases a cloud of poisonous gas.", 0, "Traps"),
      BuildInfo({TrapType::ALARM, ViewId::ALARM_TRAP}, "Alarm trap", {{RequirementId::TECHNOLOGY, TechId::TRAPS}},
          "Summons all minions", 0, "Traps"),
      BuildInfo({TrapType::WEB, ViewId::WEB_TRAP}, "Web trap", {{RequirementId::TECHNOLOGY, TechId::TRAPS}},
          "Immobilises the trespasser for some time.", 0, "Traps"),
      BuildInfo({TrapType::BOULDER, ViewId::BOULDER}, "Boulder trap", {{RequirementId::TECHNOLOGY, TechId::TRAPS}},
          "Causes a huge boulder to roll towards the enemy.", 0, "Traps"),
      BuildInfo({TrapType::SURPRISE, ViewId::SURPRISE_TRAP}, "Surprise trap",
          {{RequirementId::TECHNOLOGY, TechId::TRAPS}},
          "Teleports nearby minions to deal with the trespasser.", 0, "Traps"),
    });
  }
  return *buildInfo;
}

BuildInfo::FurnitureInfo::FurnitureInfo(FurnitureType type, CostInfo cost, bool noCredit, optional<int> maxNumber)
    : FurnitureInfo(vector<FurnitureType>(1, type), cost, noCredit, maxNumber) {}

BuildInfo::FurnitureInfo::FurnitureInfo(vector<FurnitureType> t, CostInfo c, bool n, optional<int> m)
    : types(t), cost(c), noCredit(n), maxNumber(m) {}

BuildInfo& BuildInfo::setTutorialHighlight(TutorialHighlight t) {
  tutorialHighlight = t;
  return *this;
}

BuildInfo::BuildInfo(FurnitureInfo info, const string& n, vector<Requirement> req, const string& h, char key,
            string group, bool hotkeyOpens) : furnitureInfo(info), buildType(FURNITURE), name(n),
    requirements(req), help(h), hotkey(key), groupName(group), hotkeyOpensGroup(hotkeyOpens) {}

BuildInfo::BuildInfo(TrapInfo info, const string& n, vector<Requirement> req, const string& h, char key,
            string group) : trapInfo(info), buildType(TRAP), name(n), requirements(req), help(h), hotkey(key), groupName(group) {}

BuildInfo::BuildInfo(BuildType type, const string& n, const string& h, char key, string group)
    : buildType(type), name(n), help(h), hotkey(key), groupName(group) {
    CHECK(contains({DIG, FORBID_ZONE, DISPATCH, CLAIM_TILE}, type));
  }

BuildInfo::BuildInfo(const vector<FurnitureLayer>& layers, const string& n, const string& h, char key, string group)
    : buildType(DESTROY), name(n), help(h), hotkey(key), groupName(group), destroyLayers(layers) {
  }

BuildInfo::BuildInfo(ZoneId zone, ViewId view, const string& n, const string& h, char key, string group,
            bool hotkeyOpens)
    : buildType(ZONE), name(n), help(h), hotkey(key), groupName(group), hotkeyOpensGroup(hotkeyOpens) , zone(zone),
        viewId(view) {}

bool BuildInfo::meetsRequirement(WConstCollective col, Requirement req) {
  switch (req.getId()) {
    case RequirementId::TECHNOLOGY:
      return col->hasTech(req.get<TechId>());
    case RequirementId::VILLAGE_CONQUERED: {
      auto& mainVillains = col->getGame()->getVillains(VillainType::MAIN);
      for (WConstCollective enemy : mainVillains)
        if (enemy->isConquered())
          return true;
      return mainVillains.empty();
    }
  }
}

string BuildInfo::getRequirementText(Requirement req) {
  switch (req.getId()) {
    case RequirementId::TECHNOLOGY:
      return Technology::get(req.get<TechId>())->getName();
    case RequirementId::VILLAGE_CONQUERED:
      return "that at least one main villain is conquered";
  }
}

vector<BuildInfo::RoomInfo> BuildInfo::getRoomInfo() {
  vector<RoomInfo> ret;
  for (auto& bInfo : get())
    if (bInfo.buildType == BuildInfo::FURNITURE)
      ret.push_back({bInfo.name, bInfo.help, bInfo.requirements});
  return ret;
}
