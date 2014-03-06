#include "stdafx.h"

#include "model.h"
#include "collective.h"
#include "quest.h"
#include "player.h"
#include "village_control.h"
#include "message_buffer.h"
#include "statistics.h"
#include "options.h"
#include "task.h"

template <class Archive> 
void Model::serialize(Archive& ar, const unsigned int version) { 
  ar& BOOST_SERIALIZATION_NVP(levels)
    & BOOST_SERIALIZATION_NVP(villageControls)
    & BOOST_SERIALIZATION_NVP(timeQueue)
    & BOOST_SERIALIZATION_NVP(deadCreatures)
    & BOOST_SERIALIZATION_NVP(lastTick)
    & BOOST_SERIALIZATION_NVP(levelLinks)
    & BOOST_SERIALIZATION_NVP(collective)
    & BOOST_SERIALIZATION_NVP(elvesDead)
    & BOOST_SERIALIZATION_NVP(humansDead)
    & BOOST_SERIALIZATION_NVP(dwarvesDead)
    & BOOST_SERIALIZATION_NVP(won)
    & BOOST_SERIALIZATION_NVP(addHero);
  Skill::serializeAll(ar);
  Quest::serializeAll(ar);
  Tribe::serializeAll(ar);
  Statistics::serialize(ar, version);
}

SERIALIZABLE(Model);

bool Model::isTurnBased() {
  return !collective || collective->isTurnBased();
}

const Creature* Model::getPlayer() const {
  for (const PLevel& l : levels)
    if (l->getPlayer())
      return l->getPlayer();
  return nullptr;
}

void Model::update(double totalTime) {
  if (addHero) {
    CHECK(collective && collective->isRetired());
    landHeroPlayer();
    addHero = false;
  }
  if (collective) {
    collective->render(view);
  }
  do {
    if (collective && !collective->isTurnBased()) {
      // process a few times so events don't stack up when game is paused
      for (int i : Range(10))
        collective->processInput(view);
    }
    Creature* creature = timeQueue.getNextCreature();
    CHECK(creature) << "No more creatures";
    Debug() << creature->getTheName() << " moving now";
    double time = creature->getTime();
    if (time > totalTime)
      return;
    if (time >= lastTick + 1) {
      MEASURE({ tick(time); }, "ticking time");
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

void Model::tick(double time) {
  Debug() << "Turn " << time;
  for (Creature* c : timeQueue.getAllCreatures()) {
    c->tick(time);
  }
  for (PLevel& l : levels)
    for (Square* square : l->getTickingSquares())
      square->tick(time);
  lastTick = time;
  if (collective) {
    collective->tick();
    bool conquered = true;
    for (PVillageControl& control : villageControls) {
      control->tick(time);
      conquered &= control->isConquered();
    }
    if (conquered && !won) {
      collective->onConqueredLand(NameGenerator::worldNames.getNext());
      won = true;
    }
  }
}

void Model::addCreature(PCreature c) {
  c->setTime(timeQueue.getCurrentTime() + 1);
  timeQueue.addCreature(std::move(c));
}

void Model::removeCreature(Creature* c) {
  deadCreatures.push_back(timeQueue.removeCreature(c));
}

Level* Model::buildLevel(Level::Builder&& b, LevelMaker* maker, bool surface) {
  Level::Builder builder(std::move(b));
  maker->make(&builder, builder.getBounds());
  levels.push_back(builder.build(this, surface));
  return levels.back().get();
}

Model::Model(View* v) : view(v) {
}

Model::~Model() {
}

Level* Model::prepareTopLevel2(vector<SettlementInfo> settlements) {
  Level* top = buildLevel(
      Level::Builder(180, 120, "Wilderness"),
      LevelMaker::topLevel2(CreatureFactory::forrest(), settlements),
      true);
  return top;
}

Level* Model::prepareTopLevel(vector<SettlementInfo> settlements) {
  pair<CreatureId, string> castleNem1 = chooseRandom<pair<CreatureId, string>>(
      {{CreatureId::GHOST, "The castle cellar is haunted. Go and kill the evil that is lurking there."},
      {CreatureId::SPIDER, "The castle cellar is infested by vermin. Go and clean it up."}}, {1, 1});
  pair<CreatureId, string> castleNem2 = chooseRandom<pair<CreatureId, string>>(
      {{CreatureId::DRAGON, "dragon"}, {CreatureId::CYCLOPS, "cyclops"}});
  Quest::dragon = Quest::killTribeQuest(Tribe::dragon, "A " + castleNem2.second + 
      " is harrasing our village. Kill it. It lives in a cave not far from here.");
  Quest::castleCellar = Quest::killTribeQuest(Tribe::castleCellar, castleNem1.second);
  Quest::bandits = Quest::killTribeQuest(Tribe::bandit, "There is a bandit camp nearby. Kill them all.");
  Quest::dwarves = Quest::killTribeQuest(Tribe::dwarven, "Slay our enemy, the dwarf baron. I will reward you.", true);
  Quest::goblins = Quest::killTribeQuest(Tribe::goblin, "The goblin den is located deep under the earth. "
      "Slay the great goblin. I will reward you.", true);
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

vector<Location*> getVillageLocations(int numVillages) {
  vector<Location*> ret;
  for (int i : Range(numVillages))
    ret.push_back(new Location(NameGenerator::townNames.getNext(), ""));
  return ret;
}

static void setHandicap(Tribe* tribe) {
  if (Options::getValue(OptionId::EASY_GAME))
    tribe->setHandicap(2);
  else
    tribe->setHandicap(-1);
}

Model* Model::heroModel(View* view) {
  Creature::noExperienceLevels();
  Model* m = new Model(view);
  vector<Location*> locations = getVillageLocations(3);
  Level* top = m->prepareTopLevel({
      {SettlementType::CASTLE, CreatureFactory::humanVillage(0.3), Random.getRandom(10, 20), CreatureId::AVATAR,
          locations[0], Tribe::human, {30, 20}, {StairKey::CASTLE_CELLAR}},
      {SettlementType::VILLAGE, CreatureFactory::elvenVillage(0.3), Random.getRandom(10, 20), CreatureId::ELF_LORD,
          locations[2], Tribe::elven, {30, 20}, {}}});
  Level* d1 = m->buildLevel(
      Level::Builder(60, 35, "Dwarven Halls"),
      LevelMaker::mineTownLevel(CreatureFactory::dwarfTown(), {StairKey::DWARF}, {StairKey::DWARF}));
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
  PCreature player = m->makePlayer();
  for (int i : Range(Random.getRandom(70, 131)))
    player->take(ItemFactory::fromId(ItemId::GOLD_PIECE));
  Tribe::goblin->makeSlightEnemy(player.get());
  Level* start = top;
  start->setPlayer(player.get());
  start->landCreature(StairDirection::UP, StairKey::PLAYER_SPAWN, std::move(player));
  setHandicap(Tribe::player);
  return m;
}

PCreature Model::makePlayer() {
  map<const Level*, MapMemory>* levelMemory = new map<const Level*, MapMemory>();
  PCreature player = CreatureFactory::addInventory(
      PCreature(new Creature(ViewObject(ViewId::PLAYER, ViewLayer::CREATURE, "Player"), Tribe::player,
      CATTR(
          c.speed = 100;
          c.weight = 90;
          c.size = CreatureSize::LARGE;
          c.strength = 13;
          c.dexterity = 15;
          c.barehandedDamage = 5;
          c.humanoid = true;
          c.name = "Adventurer";
          c.firstName = NameGenerator::firstNames.getNext();
          c.skills.insert(Skill::archery);
          c.skills.insert(Skill::twoHandedWeapon);), Player::getFactory(view, this, levelMemory))), {
      ItemId::FIRST_AID_KIT,
      ItemId::SWORD,
      ItemId::KNIFE,
      ItemId::LEATHER_ARMOR, ItemId::LEATHER_HELM});
  for (int i : Range(Random.getRandom(70, 131)))
    player->take(ItemFactory::fromId(ItemId::GOLD_PIECE));
  return player;
}

void Model::exitAction() {
  auto ind = view->chooseFromList("Would you like to:", {
      collective && won ? "Retire fortress" : "Save the game",
      "Abandon the game",
      "Cancel"});
  if (ind == 0) {
    if (!collective || collective->isRetired())
      throw SaveGameException(GameType::ADVENTURER);
    else if (!won)
      throw SaveGameException(GameType::KEEPER);
    else {
      retireCollective();
      throw SaveGameException(GameType::RETIRED_KEEPER);
    }
  }
  if (ind == 1)
    throw GameOverException();
  return;
}

void Model::retireCollective() {
  CHECK(collective);
  Statistics::init();
  Tribe::keeper->setHandicap(0);
  setHandicap(Tribe::player);
  collective->retire();
  won = false;
  addHero = true;
}

void Model::landHeroPlayer() {
  PCreature player = makePlayer();
  levels[0]->setPlayer(player.get());
  levels[0]->landCreature(StairDirection::UP, StairKey::HERO_SPAWN, std::move(player));
  auto handicap = view->getNumber("Choose handicap", 10);
  if (handicap)
    Tribe::player->setHandicap(*handicap);
}

string Model::getGameIdentifier() const {
  if (collective)
    return *collective->getKeeper()->getFirstName();
  else
    return *NOTNULL(getPlayer())->getFirstName();
}

void Model::onKillEvent(const Creature* victim, const Creature* killer) {
  if (collective && collective->isRetired() && victim == collective->getKeeper()) {
    const Creature* c = getPlayer();
    killedKeeper(c->getNameAndTitle(), collective->getKeeper()->getNameAndTitle(), NameGenerator::worldNames.getNext(), c->getKills(), c->getPoints());
  }
}

Model* Model::collectiveModel(View* view) {
  Model* m = new Model(view);
  vector<Location*> villageLocations = getVillageLocations(2);
  vector<SettlementInfo> settlements {
    {SettlementType::CASTLE, CreatureFactory::humanVillage(0.0), 12, Nothing(), villageLocations[0],
      Tribe::human, {30, 20}, {}},
    {SettlementType::VILLAGE, CreatureFactory::elvenVillage(0.0), 7, Nothing(), villageLocations[1],
      Tribe::elven, {30, 20}, {}}  };
  vector<CreatureFactory> cottageF {
    CreatureFactory::humanVillage(0),
    CreatureFactory::elvenVillage(0),
  };
  vector<Tribe*> cottageT { Tribe::human, Tribe::elven };
  for (int i : Range(4, 8))
    settlements.push_back(
       {SettlementType::COTTAGE, cottageF[i % 2], Random.getRandom(3, 7), Nothing(), new Location(), cottageT[i % 2],
       {10, 10}, {}});
  Level* top = m->prepareTopLevel2(settlements);
  Level* d1 = m->buildLevel(
      Level::Builder(60, 35, "Dwarven Halls"),
      LevelMaker::mineTownLevel(CreatureFactory::dwarfTownPeaceful(), {StairKey::DWARF}, {}));
  m->addLink(StairDirection::DOWN, StairKey::DWARF, top, d1);
  m->collective.reset(new Collective(m, Tribe::keeper));
  m->collective->setLevel(top);
  PCreature c = CreatureFactory::fromId(CreatureId::KEEPER, Tribe::keeper,
      MonsterAIFactory::collective(m->collective.get()));
  Creature* ref = c.get();
  top->landCreature(StairDirection::UP, StairKey::PLAYER_SPAWN, c.get());
  m->addCreature(std::move(c));
  m->collective->addCreature(ref);
 // m->collective->possess(ref, view);
  for (int i : Range(4)) {
    PCreature c = CreatureFactory::fromId(CreatureId::IMP, Tribe::keeper,
        MonsterAIFactory::collective(m->collective.get()));
    top->landCreature(StairDirection::UP, StairKey::PLAYER_SPAWN, c.get());
    m->collective->addCreature(c.get(), MinionType::IMP);
    m->addCreature(std::move(c));
  }
  vector<CreatureFactory> villageFactories {
    { CreatureFactory::collectiveFinalAttack() },
    { CreatureFactory::collectiveElfFinalAttack() }
  };
  vector<int> heroCounts {
    20, 10
  };
  int cnt = 0;
  auto killedCoeff = [] () { return Random.getDouble(0.1, 0.3); };
  auto powerCoeff = [] () { return Random.getDouble(0.2, 1); };
  for (int i : All(villageLocations)) {
    PVillageControl control = VillageControl::topLevelVillage(m->collective.get(), villageLocations[i],
        killedCoeff(), powerCoeff());
    for (int j : Range(heroCounts[i])) {
      PCreature c = villageFactories[i].random(MonsterAIFactory::villageControl(control.get(), villageLocations[i]));
      control->addCreature(c.get());
      top->landCreature(villageLocations[i]->getBounds().getAllSquares(), std::move(c));
    }
    m->villageControls.push_back(std::move(control));
  }
  PVillageControl dwarfControl = VillageControl::dwarfVillage(m->collective.get(), d1,
      StairDirection::UP, StairKey::DWARF, killedCoeff(), powerCoeff());
  int dwarfCount = 10;
  CreatureFactory factory = CreatureFactory::dwarfTown();
  for (int k : Range(dwarfCount)) {
    PCreature c = factory.random(MonsterAIFactory::villageControl(dwarfControl.get(), nullptr));
    dwarfControl->addCreature(c.get());
    d1->landCreature(StairDirection::UP, StairKey::DWARF, std::move(c));
  }
  m->villageControls.push_back(std::move(dwarfControl));
  setHandicap(Tribe::keeper);
  return m;
}

View* Model::getView() {
  return view;
}

void Model::setView(View* v) {
  view = v;
  if (collective)
    v->setTimeMilli(collective->getKeeper()->getTime() * 300);
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
  
void Model::conquered(const string& title, const string& land, vector<const Creature*> kills, int points) {
  string text= "You have conquered this land. You killed " + convertToString(kills.size()) +
      " innocent beings and scored " + convertToString(points) +
      " points. Thank you for playing KeeperRL alpha. If you like the game, consider donating to support its development.\n \n";
  for (string stat : Statistics::getText())
    text += stat + "\n";
  view->presentText("Victory", text);
  ofstream("highscore.txt", std::ofstream::out | std::ofstream::app)
    << title << "," << "conquered the land of " + land + "," << points << std::endl;
  showHighscore(true);
}

void Model::killedKeeper(const string& title, const string& keeper, const string& land,
    vector<const Creature*> kills, int points) {
  string text= "You have freed this land from the bloody reign of " + keeper + 
      ". You killed " + convertToString(kills.size()) +
      " enemies and scored " + convertToString(points) +
      " points. Thank you for playing KeeperRL alpha. If you like the game, consider donating to support its development.\n \n";
  for (string stat : Statistics::getText())
    text += stat + "\n";
  view->presentText("Victory", text);
  ofstream("highscore.txt", std::ofstream::out | std::ofstream::app)
    << title << "," << "freed the land of " + land + "," << points << std::endl;
  showHighscore(true);
  throw GameOverException();
}

void Model::gameOver(const Creature* creature, int numKills, const string& enemiesString, int points) {
  string text = "And so dies ";
  string title;
  if (auto firstName = creature->getFirstName())
    title = *firstName + " ";
  title += "the " + creature->getName();
  text += title;
  string killer;
  if (const Creature* c = creature->getLastAttacker()) {
    killer = c->getName();
    text += ", killed by a " + killer;
  }
  text += ". He killed " + convertToString(numKills) 
      + " " + enemiesString + " and scored " + convertToString(points) + " points.\n \n";
  for (string stat : Statistics::getText())
    text += stat + "\n";
  view->presentText("Game over", text);
  ofstream("highscore.txt", std::ofstream::out | std::ofstream::app)
    << title << "," << "killed by a " + killer << "," << points << std::endl;
  showHighscore(true);
  if (view->yesOrNoPrompt("Would you like to see the last messages?"))
    messageBuffer.showHistory();
  throw GameOverException();
}

void Model::showHighscore(bool highlightLast) {
  struct Elem {
    string name;
    string killer;
    int points;
    bool highlight = false;
  };
  vector<Elem> v;
  ifstream in("highscore.txt");
  while (1) {
    char buf[100];
    in.getline(buf, 100);
    if (!in)
      break;
    vector<string> p = split(string(buf), ',');
    CHECK(p.size() == 3) << "Input error " << p;
    Elem e;
    e.name = p[0];
    e.killer = p[1];
    e.points = convertFromString<int>(p[2]);
    v.push_back(e);
  }
  if (v.empty())
    return;
  if (highlightLast)
    v.back().highlight = true;
  sort(v.begin(), v.end(), [] (const Elem& a, const Elem& b) -> bool {
      return a.points > b.points;
    });
  vector<View::ListElem> scores;
  for (Elem& elem : v) {
    scores.push_back(View::ListElem(elem.name + ", " + elem.killer + "       " +
        convertToString(elem.points) + " points",
        highlightLast && !elem.highlight ? View::INACTIVE : View::NORMAL));
  }
  view->presentList("High scores", scores);
}
