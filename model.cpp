#include "stdafx.h"

using namespace std;

bool Model::isTurnBased() {
  return !collective || collective->isTurnBased();
}

void Model::update(double totalTime) {
  if (collective)
    collective->render(view);
  do {
    if (collective && !collective->isTurnBased()) {
      // process a few times so events don't stack up when game is paused
      for (int i : Range(5))
        collective->processInput(view);
    }
    Creature* creature = timeQueue.getNextCreature();
    CHECK(creature) << "No more creatures";
    Debug() << creature->getTheName() << " moving now";
    double time = creature->getTime();
    if (time > totalTime)
      return;
    if (time >= lastTick + 1) {
      MEASURE({
          Debug() << "Turn " << time;
          for (Creature* c : timeQueue.getAllCreatures()) {
            c->tick(time);
          }
          for (Level* l : levels)
            for (Square* square : l->getTickingSquares())
              square->tick(time);
          lastTick = time;
          if (collective)
            collective->tick();
          }, "ticking time");
    }
    bool unpossessed = false;
    if (!creature->isDead()) {
      bool wasPlayer = creature->isPlayer();
      creature->makeMove();
      if (wasPlayer && !creature->isPlayer())
        unpossessed = true;
    }
    if (collective)
      collective->update(creature);
    if (!creature->isDead()) {
      Level* level = creature->getLevel();
      CHECK(level->getSquare(creature->getPosition())->getCreature() == creature);
    }
    if (unpossessed)
      break;
  } while (1);
}

void Model::addCreature(PCreature c) {
  c->setTime(timeQueue.getCurrentTime() + 1);
  CHECK(c->getLevel() != nullptr) << "Creature must already be located on a level.";
  timeQueue.addCreature(std::move(c));
}

void Model::removeCreature(Creature* c) {
  deadCreatures.push_back(timeQueue.removeCreature(c));
}

Level* Model::buildLevel(Level::Builder&& b, LevelMaker* maker, bool surface) {
  Level::Builder builder(std::move(b));
  maker->make(&builder, Rectangle(builder.getWidth(), builder.getHeight()));
  levels.push_back(builder.build(this, surface));
  return levels.back();
}

Model::Model(View* v) : view(v) {
}

Level* Model::prepareTopLevel(vector<SettlementInfo> settlements) {
  pair<CreatureId, string> castleNem1 = chooseRandom<pair<CreatureId, string>>(
      {{CreatureId::GHOST, "The castle cellar is haunted. Go and kill the evil that is lurking there."},
      {CreatureId::SPIDER, "The castle cellar is infested by vermin. Go and clean it up."}});
  pair<CreatureId, string> castleNem2 = chooseRandom<pair<CreatureId, string>>(
      {{CreatureId::DRAGON, "dragon"}, {CreatureId::CYCLOPS, "cyclops"}});
  Quest::dragon = Quest::killTribeQuest(Tribe::dragon, "There is a " + castleNem2.second + 
      " living in a cave. Kill it.");
  Quest::castleCellar = Quest::killTribeQuest(Tribe::castleCellar, castleNem1.second);
  Quest::bandits = Quest::killTribeQuest(Tribe::bandit, "There is a bandit camp nearby. Kill them all.");
  Level* top = buildLevel(
      Level::Builder(600, 600, "Wilderness"),
      LevelMaker::topLevel(CreatureFactory::forrest(), settlements),
      true);
  Level* c1 = buildLevel(
      Level::Builder(30, 20, "Crypt"),
      LevelMaker::cryptLevel(CreatureFactory::crypt(),{StairKey::CRYPT}, {}));
  Level* p1 = buildLevel(
      Level::Builder(13, 13, "Pyramid Level 2"),
      LevelMaker::pyramidLevel(CreatureFactory::pyramid(1), {StairKey::PYRAMID},  {StairKey::PYRAMID}));
  Level* p2 = buildLevel(
      Level::Builder(11, 11, "Pyramid Level 3"),
      LevelMaker::pyramidLevel(CreatureFactory::pyramid(2), {}, {StairKey::PYRAMID}));
  Level* cellar = buildLevel(
      Level::Builder(30, 20, "Cellar"),
      LevelMaker::cellarLevel(CreatureFactory::singleType(Tribe::castleCellar, castleNem1.first),
          SquareType::LOW_ROCK_WALL, StairLook::CELLAR, {StairKey::CASTLE_CELLAR}, {}));
  Level* dragon = buildLevel(
      Level::Builder(40, 30, capitalFirst(castleNem2.second) + "'s Cave"),
      LevelMaker::cavernLevel(CreatureFactory::singleType(Tribe::dragon, castleNem2.first),
          SquareType::MUD_WALL, SquareType::MUD, StairLook::NORMAL, {StairKey::DRAGON}, {}));
  addLink(StairDirection::DOWN, StairKey::CRYPT, top, c1);
  addLink(StairDirection::UP, StairKey::PYRAMID, top, p1);
  addLink(StairDirection::UP, StairKey::PYRAMID, p1, p2);
  addLink(StairDirection::DOWN, StairKey::CASTLE_CELLAR, top, cellar);
  addLink(StairDirection::DOWN, StairKey::DRAGON, top, dragon); 

  return top;
}

static const int numVillages = 3;

vector<Location*> getVillageLocations() {
  vector<Location*> ret;
  for (int i : Range(numVillages))
    ret.push_back(new Location(NameGenerator::townNames.getNext(), ""));
  return ret;
}

Model* Model::heroModel(View* view, const string& heroName) {
  Model* m = new Model(view);
  vector<Location*> locations = getVillageLocations();
  Level* top = m->prepareTopLevel({
      {SettlementType::CASTLE, CreatureFactory::humanVillage(), locations[0], Tribe::human},
      {SettlementType::VILLAGE, CreatureFactory::humanVillagePeaceful(), locations[1], Tribe::human},
      {SettlementType::VILLAGE, CreatureFactory::elvenVillage(), locations[2], Tribe::elven}});
  Level* d1 = m->buildLevel(
      Level::Builder(60, 35, "Dwarven Halls"),
      LevelMaker::mineTownLevel(CreatureFactory::dwarfTown(1), {StairKey::DWARF}, {StairKey::DWARF}));
  Level* g1 = m->buildLevel(
      Level::Builder(60, 35, "Goblin Den"),
      LevelMaker::goblinTownLevel(CreatureFactory::goblinTown(1), {StairKey::DWARF}, {}));
  vector<Level*> gnomish;
  int numGnomLevels = 8;
 // int towerLinkIndex = Random.getRandom(1, numGnomLevels - 1);
  for (int i = 0; i < numGnomLevels; ++i) {
    vector<StairKey> upKeys {StairKey::DWARF};
 /*   if (i == towerLinkIndex)
      upKeys.push_back(StairKey::TOWER);*/
    gnomish.push_back(m->buildLevel(
          Level::Builder(60, 35, "Gnomish Mines Level " + convertToString(i + 1)),
          LevelMaker::roomLevel(CreatureFactory::level(i + 1), upKeys, {StairKey::DWARF})));
  }
 /* vector<Level*> tower;
  int numTowerLevels = 5;
  for (int i = 0; i < numTowerLevels; ++i)
    tower.push_back(m->buildLevel(
          Level::Builder(4, 4, "Stone Tower " + convertToString(i + 2)),
          LevelMaker::towerLevel(StairKey::TOWER, StairKey::TOWER)));

  for (int i = 0; i < numTowerLevels - 1; ++i)
    m->addLink(StairDirection::DOWN, StairKey::TOWER, tower[i + 1], tower[i]);*/

 // m->addLink(StairDirection::DOWN, StairKey::TOWER, tower[0], gnomish[towerLinkIndex]);
 // m->addLink(StairDirection::DOWN, StairKey::TOWER, top, tower.back());

  for (int i = 0; i < numGnomLevels - 1; ++i)
    m->addLink(StairDirection::DOWN, StairKey::DWARF, gnomish[i], gnomish[i + 1]);

  m->addLink(StairDirection::DOWN, StairKey::DWARF, top, d1);
  m->addLink(StairDirection::DOWN, StairKey::DWARF, d1, gnomish[0]);
  m->addLink(StairDirection::UP, StairKey::DWARF, g1, gnomish.back());
  map<const Level*, MapMemory>* levelMemory = new map<const Level*, MapMemory>();
  PCreature player = CreatureFactory::addInventory(
      PCreature(new Creature(ViewObject(ViewId::PLAYER, ViewLayer::CREATURE, "Player"), Tribe::player,
      CATTR(
          c.speed = 100;
          c.weight = 90;
          c.size = CreatureSize::LARGE;
          c.strength = 15;
          c.dexterity = 15;
          c.barehandedDamage = 5;
          c.humanoid = true;
          c.name = "adventurer";
          c.firstName = heroName;
          c.skills.insert(Skill::archery);
          c.skills.insert(Skill::twoHandedWeapon);), Player::getFactory(view, levelMemory))), {
      ItemId::FIRST_AID_KIT,
      ItemId::SWORD,
      ItemId::KNIFE,
      ItemId::DECEPTION_SCROLL,
      ItemId::DECEPTION_SCROLL,
      ItemId::DECEPTION_SCROLL,
      ItemId::DECEPTION_SCROLL,
      ItemId::LEATHER_ARMOR, ItemId::LEATHER_HELM});
  for (int i : Range(Random.getRandom(70, 131)))
    player->take(ItemFactory::fromId(ItemId::GOLD_PIECE));
  Tribe::goblin->makeSlightEnemy(player.get());
  Level* start = top;
  start->setPlayer(player.get());
  start->landCreature(StairDirection::UP, StairKey::PLAYER_SPAWN, std::move(player));
  return m;
}

Model* Model::collectiveModel(View* view, int numCreatures) {
  Model* m = new Model(view);
  CreatureFactory factory = CreatureFactory::collectiveStart();
  vector<Location*> villageLocations = getVillageLocations();
  Level* top = m->prepareTopLevel({
      {SettlementType::CASTLE, CreatureFactory::humanVillagePeaceful(), villageLocations[0], Tribe::human},
      {SettlementType::VILLAGE, CreatureFactory::elvenVillagePeaceful(), villageLocations[1], Tribe::elven},
      {SettlementType::VILLAGE, CreatureFactory::humanVillagePeaceful(), villageLocations[2], Tribe::human}});
  Level* second = m->buildLevel(
          Level::Builder(60, 35, "Dungeons of doom "),
          LevelMaker::roomLevel(CreatureFactory::level(3), {StairKey::DWARF}, {StairKey::DWARF}));
  m->collective = new Collective(CreatureFactory::collectiveMinions(), CreatureFactory::collectiveUndead());
  Level* l = m->buildLevel(Level::Builder(60, 35, "A Cave"),
      LevelMaker::collectiveLevel({StairKey::DWARF}, {StairKey::DWARF}, StairKey::TOWER, m->collective));
  Level* dwarf = m->buildLevel(
          Level::Builder(60, 35, NameGenerator::townNames.getNext()),
          LevelMaker::mineTownLevel(CreatureFactory::singleType(Tribe::dwarven, CreatureId::DWARF),
              {StairKey::DWARF}, {}));
  Level* hell = m->buildLevel(
          Level::Builder(30, 20, "Hell"),
          LevelMaker::cellarLevel(CreatureFactory::hellLevel(), SquareType::HELL_WALL, StairLook::HELL,
              {StairKey::TOWER}, {}));
  m->addLink(StairDirection::DOWN, StairKey::DWARF, top, l);
  m->addLink(StairDirection::DOWN, StairKey::DWARF, l, second);
  m->addLink(StairDirection::DOWN, StairKey::DWARF, second, dwarf);
  m->addLink(StairDirection::DOWN, StairKey::TOWER, l, hell);
  m->collective->setLevel(l);
  Tribe::elven->addEnemy(Tribe::player);
  Tribe::dwarven->addEnemy(Tribe::player);
  Tribe::goblin->addEnemy(Tribe::player);
  Tribe::human->addEnemy(Tribe::player);
  for (int i : Range(numCreatures)) {
    PCreature c = factory.random(MonsterAIFactory::collective(m->collective));
    l->landCreature(StairDirection::UP, StairKey::PLAYER_SPAWN, c.get());
    m->collective->addCreature(c.get());
    m->addCreature(std::move(c));
  }
  vector<tuple<int, int, int>> heroAttackTime {
      { make_tuple(800, 2, 4) },
      { make_tuple(1400, 4, 7) },
      { make_tuple(2000, 12, 18) }};
  vector<pair<CreatureFactory, CreatureFactory>> villageFactories {
    { CreatureFactory::collectiveEnemies(), CreatureFactory::collectiveFinalAttack() },
    { CreatureFactory::collectiveElfEnemies(), CreatureFactory::collectiveElfFinalAttack() }
  };
  int cnt = 0;
  for (Location* loc : villageLocations) {
    VillageControl* control = VillageControl::humanVillage(m->collective, loc,
        StairDirection::DOWN, StairKey::DWARF);
    CreatureFactory firstAttack = villageFactories[cnt].first;
    CreatureFactory lastAttack = villageFactories[cnt].second;
    for (int i : All(heroAttackTime)) {
      int attackTime = get<0>(heroAttackTime[i]) + Random.getRandom(-200, 200);
      int heroCount = Random.getRandom(get<1>(heroAttackTime[i]), get<2>(heroAttackTime[i]));
      CreatureFactory& factory = (i == heroAttackTime.size() - 1 ? lastAttack : firstAttack);
      for (int k : Range(heroCount)) {
        PCreature c = factory.random(MonsterAIFactory::villageControl(control, loc));
        control->addCreature(c.get(), attackTime);
        top->landCreature(loc->getBounds().getAllSquares(), std::move(c));
      }
    }
    if (++cnt > 1)
      break;
  }
  VillageControl* dwarfControl = VillageControl::dwarfVillage(m->collective, l, StairDirection::UP, StairKey::DWARF);
  CreatureFactory firstAttack = CreatureFactory::singleType(Tribe::dwarven, CreatureId::DWARF);
  CreatureFactory lastAttack = CreatureFactory::dwarfTown(1);
  for (int i : All(heroAttackTime)) {
    CreatureFactory& factory = (i == heroAttackTime.size() - 1 ? lastAttack : firstAttack);
    int attackTime = get<0>(heroAttackTime[i]) + Random.getRandom(-200, 200);
    int heroCount = Random.getRandom(get<1>(heroAttackTime[i]), get<2>(heroAttackTime[i]));
    for (int k : Range(heroCount)) {
      PCreature c = factory.random(MonsterAIFactory::villageControl(dwarfControl, nullptr));
      dwarfControl->addCreature(c.get(), attackTime);
      dwarf->landCreature(StairDirection::UP, StairKey::DWARF, std::move(c));
    }
  }
  Tribe::player->addEnemy(Tribe::dwarven);
  return m;
}

void Model::addLink(StairDirection dir, StairKey key, Level* l1, Level* l2) {
  levelLinks[make_tuple(dir, key, l1)] = l2;
  levelLinks[make_tuple(opposite(dir), key, l2)] = l1;
}

Vec2 Model::changeLevel(StairDirection dir, StairKey key, Creature* c) {
  Level* current = c->getLevel();
  Level* target = levelLinks[make_tuple(dir, key, current)];
  Vec2 newPos = target->landCreature(opposite(dir), key, c);
  if (c->isPlayer()) {
    current->setPlayer(nullptr);
    target->setPlayer(c);
  }
  return newPos;
}

void Model::changeLevel(Level* target, Vec2 position, Creature* c) {
  Level* current = c->getLevel();
  target->landCreature({position}, c);
  if (c->isPlayer()) {
    current->setPlayer(nullptr);
    target->setPlayer(c);
  }
}
