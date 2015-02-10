#include "stdafx.h"
#include "model_builder.h"
#include "level.h"
#include "tribe.h"
#include "technology.h"
#include "item_type.h"
#include "inventory.h"
#include "collective_builder.h"
#include "options.h"
#include "player_control.h"
#include "spectator.h"
#include "creature.h"
#include "square.h"

static Location* getVillageLocation(bool markSurprise = false) {
  return new Location(NameGenerator::get(NameGeneratorId::TOWN)->getNext(), "", markSurprise);
}

typedef VillageControl::Villain VillainInfo;

struct EnemyInfo {
  SettlementInfo settlement;
  CollectiveConfig config;
  vector<VillainInfo> villains;
};


static EnemyInfo getVault(SettlementType type, CreatureFactory factory, Tribe* tribe, int num,
    optional<ItemFactory> itemFactory = none, vector<VillainInfo> villains = {}) {
  return {CONSTRUCT(SettlementInfo,
      c.type = type;
      c.creatures = factory;
      c.numCreatures = num;
      c.location = new Location(true);
      c.tribe = tribe;
      c.buildingId = BuildingId::DUNGEON;
      c.shopFactory = itemFactory;), CollectiveConfig::noImmigrants(),
    villains};
}

static EnemyInfo getVault(SettlementType type, CreatureId id, Tribe* tribe, int num,
    optional<ItemFactory> itemFactory = none, vector<VillainInfo> villains = {}) {
  return getVault(type, CreatureFactory::singleType(tribe, id), tribe, num, itemFactory, villains);
}

struct FriendlyVault {
  CreatureId id;
  int min;
  int max;
};

static vector<FriendlyVault> friendlyVaults {
  {CreatureId::SPECIAL_HUMANOID, 1, 2},
  {CreatureId::ORC, 3, 8},
  {CreatureId::OGRE, 2, 5},
  {CreatureId::VAMPIRE, 2, 5},
};

static vector<EnemyInfo> getVaults(Tribe::Set& tribeSet) {
  vector<EnemyInfo> ret {
    getVault(SettlementType::CAVE, CreatureId::GREEN_DRAGON,
        tribeSet.killEveryone.get(), 1, ItemFactory::dragonCave(),
        { CONSTRUCT(VillainInfo,
            c.minPopulation = 0;
            c.minTeamSize = 1;
            c.leaderAttacks = true;
            c.triggers = LIST({AttackTriggerId::ENEMY_POPULATION, 22});
            c.behaviour = VillageBehaviour(VillageBehaviourId::KILL_MEMBERS, 7);
            c.attackMessage = VillageControl::CREATURE_TITLE;)}),
    getVault(SettlementType::CAVE, CreatureId::RED_DRAGON,
        tribeSet.killEveryone.get(), 1, ItemFactory::dragonCave(),
        { CONSTRUCT(VillainInfo,
            c.minPopulation = 0;
            c.minTeamSize = 1;
            c.leaderAttacks = true;
            c.triggers = LIST({AttackTriggerId::ENEMY_POPULATION, 30});
            c.behaviour = VillageBehaviour(VillageBehaviourId::KILL_MEMBERS, 12);
            c.attackMessage = VillageControl::CREATURE_TITLE;)}),
    getVault(SettlementType::CAVE, CreatureId::CYCLOPS,
        tribeSet.killEveryone.get(), 1, ItemFactory::mushrooms(true),
        { CONSTRUCT(VillainInfo,
            c.minPopulation = 0;
            c.minTeamSize = 1;
            c.leaderAttacks = true;
            c.triggers = LIST({AttackTriggerId::ENEMY_POPULATION, 15});
            c.behaviour = VillageBehaviour(VillageBehaviourId::KILL_MEMBERS, 4);
            c.attackMessage = VillageControl::CREATURE_TITLE;)}),
    getVault(SettlementType::VAULT, CreatureFactory::insects(tribeSet.monster.get()),
        tribeSet.monster.get(), Random.get(6, 12)),
    getVault(SettlementType::VAULT, CreatureId::ORC, tribeSet.keeper.get(), Random.get(3, 8)),
    getVault(SettlementType::VAULT, CreatureId::CYCLOPS, tribeSet.monster.get(), 1,
        ItemFactory::mushrooms(true)),
    getVault(SettlementType::VAULT, CreatureId::RAT, tribeSet.pest.get(), Random.get(3, 8),
        ItemFactory::armory()),
  };
  for (int i : Range(Random.get(1, 3))) {
    FriendlyVault v = chooseRandom(friendlyVaults);
    ret.push_back(getVault(SettlementType::VAULT, v.id, tribeSet.keeper.get(), Random.get(v.min, v.max)));
  }
  return ret;
}

vector<EnemyInfo> getEnemyInfo(Tribe::Set& tribeSet) {
  vector<EnemyInfo> ret;
  for (int i : Range(6, 12)) {
    ret.push_back({CONSTRUCT(SettlementInfo,
        c.type = SettlementType::COTTAGE;
        c.creatures = CreatureFactory::humanVillage(tribeSet.human.get());
        c.numCreatures = Random.get(3, 7);
        c.location = new Location();
        c.tribe = tribeSet.human.get();
        c.buildingId = BuildingId::WOOD;), CollectiveConfig::noImmigrants(), {}});
  }
  for (int i : Range(2, 5)) {
    ret.push_back({CONSTRUCT(SettlementInfo,
        c.type = SettlementType::SMALL_MINETOWN;
        c.creatures = CreatureFactory::gnomeVillage(tribeSet.dwarven.get());
        c.numCreatures = Random.get(3, 7);
        c.location = new Location(true);
        c.tribe = tribeSet.dwarven.get();
        c.buildingId = BuildingId::DUNGEON;
        c.stockpiles = LIST({StockpileInfo::MINERALS, 300});), CollectiveConfig::noImmigrants(), {}});
  }
  ret.push_back({CONSTRUCT(SettlementInfo,
        c.type = SettlementType::ISLAND_VAULT;
        c.location = new Location(true);
        c.buildingId = BuildingId::DUNGEON;
        c.stockpiles = LIST({StockpileInfo::GOLD, 800});), CollectiveConfig::noImmigrants(), {}});
  append(ret, getVaults(tribeSet));
  append(ret, {
      {CONSTRUCT(SettlementInfo,
          c.type = SettlementType::CASTLE2;
          c.creatures = CreatureFactory::vikingTown(tribeSet.human.get());
          c.numCreatures = Random.get(12, 16);
          c.location = getVillageLocation();
          c.tribe = tribeSet.human.get();
          c.buildingId = BuildingId::WOOD_CASTLE;
          c.stockpiles = LIST({StockpileInfo::GOLD, 800});
          c.guardId = CreatureId::WARRIOR;
          c.elderLoot = ItemType(ItemId::TECH_BOOK, TechId::BEAST_MUT);),
       CollectiveConfig::withImmigrants(0.003, 16, {
           CONSTRUCT(ImmigrantInfo,
               c.id = CreatureId::WARRIOR;
               c.frequency = 1;
               c.traits = LIST(MinionTrait::FIGHTER);),          
           }),
       {CONSTRUCT(VillainInfo,
          c.minPopulation = 6;
          c.minTeamSize = 5;
          c.triggers = LIST({AttackTriggerId::POWER}, {AttackTriggerId::SELF_VICTIMS});
          c.behaviour = VillageBehaviour(VillageBehaviourId::KILL_LEADER);
          c.attackMessage = VillageControl::TRIBE_AND_NAME;)}},
      {CONSTRUCT(SettlementInfo,
          c.type = SettlementType::VILLAGE;
          c.creatures = CreatureFactory::lizardTown(tribeSet.lizard.get());
          c.numCreatures = Random.get(8, 14);
          c.location = getVillageLocation();
          c.tribe = tribeSet.lizard.get();
          c.buildingId = BuildingId::MUD;
          c.elderLoot = ItemType(ItemId::TECH_BOOK, TechId::HUMANOID_MUT);
          c.shopFactory = ItemFactory::mushrooms();),
       CollectiveConfig::withImmigrants(0.007, 15, {
           CONSTRUCT(ImmigrantInfo,
               c.id = CreatureId::LIZARDMAN;
               c.frequency = 1;
               c.traits = LIST(MinionTrait::FIGHTER);),          
           }),
       {CONSTRUCT(VillainInfo,
          c.minPopulation = 4;
          c.minTeamSize = 4;
          c.triggers = LIST({AttackTriggerId::POWER}, {AttackTriggerId::SELF_VICTIMS});
          c.behaviour = VillageBehaviour(VillageBehaviourId::KILL_LEADER);
          c.attackMessage = VillageControl::TRIBE_AND_NAME;)}},
      {CONSTRUCT(SettlementInfo,
          c.type = SettlementType::VILLAGE2;
          c.creatures = CreatureFactory::elvenVillage(tribeSet.elven.get());
          c.numCreatures = Random.get(11, 18);
          c.location = getVillageLocation();
          c.tribe = tribeSet.elven.get();
          c.stockpiles = LIST({StockpileInfo::GOLD, 800});
          c.buildingId = BuildingId::WOOD;
          c.elderLoot = ItemType(ItemId::TECH_BOOK, TechId::SPELLS_MAS);),
       CollectiveConfig::withImmigrants(0.002, 18, {
           CONSTRUCT(ImmigrantInfo,
               c.id = CreatureId::ELF_ARCHER;
               c.frequency = 1;
               c.traits = LIST(MinionTrait::FIGHTER);),          
           }), {}},
      {CONSTRUCT(SettlementInfo,
          c.type = SettlementType::MINETOWN;
          c.creatures = CreatureFactory::dwarfTown(tribeSet.dwarven.get());
          c.numCreatures = Random.get(9, 14);
          c.location = getVillageLocation(true);
          c.tribe = tribeSet.dwarven.get();
          c.buildingId = BuildingId::DUNGEON;
          c.stockpiles = LIST({StockpileInfo::GOLD, 1000}, {StockpileInfo::MINERALS, 600});
          c.shopFactory = ItemFactory::dwarfShop();),
       CollectiveConfig::withImmigrants(0.002, 15, {
           CONSTRUCT(ImmigrantInfo,
               c.id = CreatureId::DWARF;
               c.frequency = 1;
               c.traits = LIST(MinionTrait::FIGHTER);),          
           }),
       {CONSTRUCT(VillainInfo,
          c.minPopulation = 3;
          c.minTeamSize = 4;
          c.triggers = LIST({AttackTriggerId::POWER}, {AttackTriggerId::SELF_VICTIMS});
          c.behaviour = VillageBehaviour(VillageBehaviourId::KILL_MEMBERS, 3);
          c.attackMessage = VillageControl::TRIBE_AND_NAME;)}},
      {CONSTRUCT(SettlementInfo,
          c.type = SettlementType::CASTLE;
          c.creatures = CreatureFactory::humanCastle(tribeSet.human.get());
          c.numCreatures = Random.get(20, 26);
          c.location = getVillageLocation();
          c.tribe = tribeSet.human.get();
          c.stockpiles = LIST({StockpileInfo::GOLD, 700});
          c.buildingId = BuildingId::BRICK;
          c.guardId = CreatureId::CASTLE_GUARD;
          c.shopFactory = ItemFactory::villageShop();),
       CollectiveConfig::withImmigrants(0.003, 26, {
           CONSTRUCT(ImmigrantInfo,
               c.id = CreatureId::KNIGHT;
               c.frequency = 1;
               c.traits = LIST(MinionTrait::FIGHTER);),
           CONSTRUCT(ImmigrantInfo,
               c.id = CreatureId::ARCHER;
               c.frequency = 1;
               c.traits = LIST(MinionTrait::FIGHTER);),          
           }),
       {CONSTRUCT(VillainInfo,
          c.minPopulation = 12;
          c.minTeamSize = 10;
          c.triggers = LIST({AttackTriggerId::POWER}, {AttackTriggerId::SELF_VICTIMS});
          c.behaviour = VillageBehaviour(VillageBehaviourId::KILL_LEADER);
          c.attackMessage = VillageControl::TRIBE_AND_NAME;)}},
      {CONSTRUCT(SettlementInfo,
          c.type = SettlementType::WITCH_HOUSE;
          c.creatures = CreatureFactory::singleType(tribeSet.monster.get(), CreatureId::WITCH);
          c.numCreatures = 1;
          c.location = new Location();
          c.tribe = tribeSet.monster.get();
          c.buildingId = BuildingId::WOOD;
          c.elderLoot = ItemType(ItemId::TECH_BOOK, TechId::ALCHEMY_ADV);), CollectiveConfig::noImmigrants(), {}},
      {CONSTRUCT(SettlementInfo,
          c.type = SettlementType::CAVE;
          c.creatures = CreatureFactory::singleType(tribeSet.bandit.get(), CreatureId::BANDIT);
          c.numCreatures = Random.get(4, 9);
          c.location = new Location();
          c.tribe = tribeSet.bandit.get();
          c.buildingId = BuildingId::DUNGEON;),
       CollectiveConfig::withImmigrants(0.001, 10, {
           CONSTRUCT(ImmigrantInfo,
               c.id = CreatureId::BANDIT;
               c.frequency = 1;
               c.traits = LIST(MinionTrait::FIGHTER);),
           }),
       {CONSTRUCT(VillainInfo,
          c.minPopulation = 0;
          c.minTeamSize = 3;
          c.triggers = LIST({AttackTriggerId::GOLD, 500});
          c.behaviour = VillageBehaviour(VillageBehaviourId::STEAL_GOLD);
          c.attackMessage = VillageControl::TRIBE_AND_NAME;)}},
  });
  return ret;
}

static CollectiveConfig getKeeperConfig(bool fastImmigration) {
  return CollectiveConfig::keeper(
      fastImmigration ? 0.1 : 0.011,
      500,
      3,
      {
      CONSTRUCT(ImmigrantInfo,
        c.id = CreatureId::GOBLIN;
        c.frequency = 1;
        c.attractions = LIST(
          {{AttractionId::SQUARE, SquareId::WORKSHOP}, 1.0, 12.0},
          {{AttractionId::SQUARE, SquareId::JEWELER}, 1.0, 9.0},
          {{AttractionId::SQUARE, SquareId::FORGE}, 1.0, 9.0},
          );
        c.traits = LIST(MinionTrait::FIGHTER, MinionTrait::NO_EQUIPMENT);
        c.salary = 10;),
      CONSTRUCT(ImmigrantInfo,
          c.id = CreatureId::ORC;
          c.frequency = 0.7;
          c.attractions = LIST(
            {{AttractionId::SQUARE, SquareId::TRAINING_ROOM}, 1.0, 12.0},
            );
          c.traits = {MinionTrait::FIGHTER};
          c.salary = 20;),
      CONSTRUCT(ImmigrantInfo,
          c.id = CreatureId::ORC_SHAMAN;
          c.frequency = 0.15;
          c.attractions = LIST(
            {{AttractionId::SQUARE, SquareId::LIBRARY}, 1.0, 16.0},
            {{AttractionId::SQUARE, SquareId::LABORATORY}, 1.0, 9.0},
            );
          c.traits = {MinionTrait::FIGHTER};
          c.salary = 20;),
      CONSTRUCT(ImmigrantInfo,
          c.id = CreatureId::OGRE;
          c.frequency = 0.3;
          c.attractions = LIST(
            {{AttractionId::SQUARE, SquareId::TRAINING_ROOM}, 3.0, 16.0}
            );
          c.traits = {MinionTrait::FIGHTER};
          c.salary = 40;),
      CONSTRUCT(ImmigrantInfo,
          c.id = CreatureId::HARPY;
          c.frequency = 0.3;
          c.attractions = LIST(
            {{AttractionId::SQUARE, SquareId::TRAINING_ROOM}, 3.0, 16.0},
            {{AttractionId::ITEM_INDEX, ItemIndex::RANGED_WEAPON}, 1.0, 3.0, true}
            );
          c.traits = {MinionTrait::FIGHTER};
          c.salary = 40;),
      CONSTRUCT(ImmigrantInfo,
          c.id = CreatureId::SPECIAL_HUMANOID;
          c.frequency = 0.2;
          c.attractions = LIST(
            {{AttractionId::SQUARE, SquareId::TRAINING_ROOM}, 3.0, 16.0},
            );
          c.traits = {MinionTrait::FIGHTER};
          c.spawnAtDorm = true;
          c.techId = TechId::HUMANOID_MUT;
          c.salary = 40;),
      CONSTRUCT(ImmigrantInfo,
          c.id = CreatureId::ZOMBIE;
          c.frequency = 0.5;
          c.traits = {MinionTrait::FIGHTER};
          c.salary = 10;
          c.spawnAtDorm = true;),
      CONSTRUCT(ImmigrantInfo,
          c.id = CreatureId::VAMPIRE;
          c.frequency = 0.3;
          c.traits = {MinionTrait::FIGHTER};
          c.salary = 40;
          c.spawnAtDorm = true;
          c.attractions = LIST(
            {{AttractionId::SQUARE, SquareId::TRAINING_ROOM}, 2.0, 12.0}
            );),
      CONSTRUCT(ImmigrantInfo,
          c.id = CreatureId::LOST_SOUL;
          c.frequency = 0.3;
          c.traits = {MinionTrait::FIGHTER};
          c.salary = 0;
          c.attractions = LIST(
            {{AttractionId::SQUARE, SquareId::RITUAL_ROOM}, 1.0, 9.0}
            );
          c.spawnAtDorm = true;),
      CONSTRUCT(ImmigrantInfo,
          c.id = CreatureId::SUCCUBUS;
          c.frequency = 0.3;
          c.traits = LIST(MinionTrait::FIGHTER, MinionTrait::NO_EQUIPMENT);
          c.salary = 0;
          c.attractions = LIST(
            {{AttractionId::SQUARE, SquareId::RITUAL_ROOM}, 2.0, 12.0}
            );
          c.spawnAtDorm = true;),
      CONSTRUCT(ImmigrantInfo,
          c.id = CreatureId::DOPPLEGANGER;
          c.frequency = 0.2;
          c.traits = {MinionTrait::FIGHTER};
          c.salary = 0;
          c.attractions = LIST(
            {{AttractionId::SQUARE, SquareId::RITUAL_ROOM}, 4.0, 12.0}
            );
          c.spawnAtDorm = true;),
      CONSTRUCT(ImmigrantInfo,
          c.id = CreatureId::RAVEN;
          c.frequency = 1.0;
          c.traits = LIST(MinionTrait::FIGHTER, MinionTrait::NO_RETURNING);
          c.limit = Model::SunlightInfo::DAY;
          c.salary = 0;),
      CONSTRUCT(ImmigrantInfo,
          c.id = CreatureId::BAT;
          c.frequency = 1.0;
          c.traits = LIST(MinionTrait::FIGHTER, MinionTrait::NO_RETURNING);
          c.limit = Model::SunlightInfo::NIGHT;
          c.salary = 0;),
      CONSTRUCT(ImmigrantInfo,
          c.id = CreatureId::WOLF;
          c.frequency = 0.15;
          c.traits = LIST(MinionTrait::FIGHTER, MinionTrait::NO_RETURNING);
          c.groupSize = Range(3, 9);
          c.autoTeam = true;
          c.salary = 0;),
      CONSTRUCT(ImmigrantInfo,
          c.id = CreatureId::CAVE_BEAR;
          c.frequency = 0.1;
          c.traits = LIST(MinionTrait::FIGHTER, MinionTrait::NO_RETURNING);
          c.salary = 0;),
      CONSTRUCT(ImmigrantInfo,
          c.id = CreatureId::WEREWOLF;
          c.frequency = 0.1;
          c.traits = LIST(MinionTrait::FIGHTER, MinionTrait::NO_RETURNING);
          c.attractions = LIST(
            {{AttractionId::SQUARE, SquareId::TRAINING_ROOM}, 4.0, 12.0}
            );
          c.salary = 0;),
      CONSTRUCT(ImmigrantInfo,
          c.id = CreatureId::SPECIAL_MONSTER_KEEPER;
          c.frequency = 0.2;
          c.traits = LIST(MinionTrait::FIGHTER, MinionTrait::NO_RETURNING);
          c.spawnAtDorm = true;
          c.techId = TechId::BEAST_MUT;
          c.salary = 0;)});
}

static EnumMap<CollectiveResourceId, int> getKeeperCredit(bool resourceBonus) {
  return {{CollectiveResourceId::MANA, 200}};
  if (resourceBonus) {
    EnumMap<CollectiveResourceId, int> credit;
    for (auto elem : ENUM_ALL(CollectiveResourceId))
      credit[elem] = 10000;
    return credit;
  }
 
}

PModel ModelBuilder::collectiveModel(ProgressMeter& meter, Options* options, View* view, const string& worldName) {
  for (int i : Range(5)) {
    try {
      return tryCollectiveModel(meter, options, view, worldName);
    } catch (LevelGenException ex) {
      Debug() << "Retrying level gen";
    }
  }
  FAIL << "Couldn't generate a level";
  return nullptr;
}

static string getNewIdSuffix() {
  vector<char> chars;
  for (char c : Range(128))
    if (isalnum(c))
      chars.push_back(c);
  string ret;
  for (int i : Range(4))
    ret += chooseRandom(chars);
  return ret;
}

PModel ModelBuilder::tryCollectiveModel(ProgressMeter& meter, Options* options, View* view, const string& worldName) {
  Model* m = new Model(view, worldName, Tribe::Set());
  m->setOptions(options);
  vector<EnemyInfo> enemyInfo = getEnemyInfo(m->tribeSet);
  vector<SettlementInfo> settlements;
  for (auto& elem : enemyInfo) {
    elem.settlement.collective =
      new CollectiveBuilder(elem.config, elem.settlement.tribe);
    settlements.push_back(elem.settlement);
  }
  Level* top = m->prepareTopLevel(meter, settlements);
  m->collectives.push_back(CollectiveBuilder(
        getKeeperConfig(options->getBoolValue(OptionId::FAST_IMMIGRATION)), m->tribeSet.keeper.get())
      .setLevel(top)
      .setCredit(getKeeperCredit(options->getBoolValue(OptionId::STARTING_RESOURCE)))
      .build("Keeper"));
 
  m->playerCollective = m->collectives.back().get();
  m->playerControl = new PlayerControl(m->playerCollective, m, top);
  m->playerCollective->setControl(PCollectiveControl(m->playerControl));
  PCreature c = CreatureFactory::fromId(CreatureId::KEEPER, m->tribeSet.keeper.get(),
      MonsterAIFactory::collective(m->playerCollective));
  string keeperName = options->getStringValue(OptionId::KEEPER_NAME);
  if (!keeperName.empty())
    c->setFirstName(keeperName);
  m->gameIdentifier = *c->getFirstName() + "_" + m->worldName + getNewIdSuffix();
  m->gameDisplayName = *c->getFirstName() + " of " + m->worldName;
  Creature* ref = c.get();
  top->landCreature(StairDirection::UP, StairKey::PLAYER_SPAWN, c.get());
  m->addCreature(std::move(c));
  m->playerControl->addKeeper(ref);
  for (int i : Range(4)) {
    PCreature c = CreatureFactory::fromId(CreatureId::IMP, m->tribeSet.keeper.get(),
        MonsterAIFactory::collective(m->playerCollective));
    top->landCreature(StairDirection::UP, StairKey::PLAYER_SPAWN, c.get());
    m->playerControl->addImp(c.get());
    m->addCreature(std::move(c));
  }
  for (int i : All(enemyInfo)) {
    if (!enemyInfo[i].settlement.collective->hasCreatures())
      continue;
    PVillageControl control;
    Location* location = enemyInfo[i].settlement.location;
    PCollective collective = enemyInfo[i].settlement.collective->build(
        location->hasName() ? location->getName() : "");
    if (!enemyInfo[i].villains.empty())
      getOnlyElement(enemyInfo[i].villains).collective = m->playerCollective;
    control.reset(new VillageControl(collective.get(), location, enemyInfo[i].villains));
    if (location->hasName())
      m->mainVillains.push_back(collective.get());
    collective->setControl(std::move(control));
    m->collectives.push_back(std::move(collective));
  }
  return PModel(m);
}

PModel ModelBuilder::splashModel(ProgressMeter& meter, View* view, const string& splashPath) {
  Model* m = new Model(view, "", Tribe::Set());
  Level* l = m->buildLevel(
      Level::Builder(meter, Level::getSplashBounds().getW(), Level::getSplashBounds().getH(), "Wilderness", false),
      LevelMaker::splashLevel(
          CreatureFactory::singleType(m->tribeSet.human.get(), CreatureId::AVATAR),
          CreatureFactory::splashHeroes(m->tribeSet.human.get()),
          CreatureFactory::splashMonsters(m->tribeSet.keeper.get()),
          CreatureFactory::singleType(m->tribeSet.keeper.get(), CreatureId::IMP), splashPath));
  m->spectator.reset(new Spectator(l));
  return PModel(m);
}

