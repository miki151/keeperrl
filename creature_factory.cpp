#include "stdafx.h"

using namespace std;

static map<string, function<PCreature ()> > creatureMap;
static map<string, vector<string> > inventoryMap;
static vector<string> keys;
static map<string, pair<int, int> > levelRange;

class BoulderController : public Monster {
  public:
  BoulderController(Creature* c, Vec2 dir) : Monster(c, MonsterAIFactory::idle()), direction(dir) {}

  BoulderController(Creature* c, Tribe* _myTribe) : Monster(c, MonsterAIFactory::idle()),
      stopped(true), myTribe(_myTribe) {}

  virtual void makeMove() override {
    if (myTribe != nullptr && stopped) {
      for (Vec2 v : Vec2::directions8(true)) {
        int radius = 6;
        bool found = false;
        for (int i = 1; i <= radius; ++i) {
          if (!(creature->getPosition() + (v * i)).inRectangle(creature->getLevel()->getBounds()))
            break;
          if (Creature* other = creature->getSquare(v * i)->getCreature())
            if (other->getTribe() != myTribe) {
              direction = v;
              stopped = false;
              found = true;
              EventListener::addTriggerEvent(creature->getLevel(), creature->getPosition());
              break;
            }
          if (!creature->getSquare(v * i)->canEnter(creature))
            break;
        }
        if (found)
          break;
      }
    }
    if (stopped) {
      creature->wait();
      return;
    }
    if (creature->getConstSquare(direction)->getStrength() < 300)
      creature->squash(direction);
    if (creature->canMove(direction))
      creature->move(direction);
    else {
      creature->getLevel()->globalMessage(creature->getPosition() + direction,
          creature->getTheName() + " crashes on the " + creature->getConstSquare(direction)->getName(),
              "You hear a crash");
      creature->die();
    }
    if (creature->isDead())
      return;
    double speed = creature->getSpeed();
    double deceleration = 0.1;
    speed -= deceleration * 100 * 100 / speed;
    if (speed < 30) {
      if (myTribe) {
        creature->die();
        return;
      }
      speed = 100;
      stopped = true;
      creature->setStationary();
      myTribe = nullptr;
    }
    creature->setSpeed(speed);
  }

  virtual void you(MsgType type, const string& param) const override {
    string msg, msgNoSee;
    switch (type) {
      case MsgType::BURN: msg = creature->getTheName() + " burns in the " + param; break;
      case MsgType::DROWN: msg = creature->getTheName() + " falls into the " + param;
                           msgNoSee = "You hear a loud splash"; break;
      case MsgType::KILLED_BY: msg = creature->getTheName() + " is destroyed by " + param; break;
      case MsgType::HIT_THROWN_ITEM: msg = param + " hits " + creature->getTheName(); break;
      case MsgType::CRASH_THROWN_ITEM: msg = param + " crashes on " + creature->getTheName(); break;
      case MsgType::ENTER_PORTAL: msg = creature->getTheName() + " disappears in the portal."; break;
      default: break;
    }
    if (!msg.empty())
      creature->globalMessage(msg, msgNoSee);
  }

  virtual void onBump(Creature* c) override {
    Vec2 dir = creature->getPosition() - c->getPosition();
    string it = c->canSee(creature) ? creature->getTheName() : "it";
    string something = c->canSee(creature) ? creature->getTheName() : "something";
    if (!creature->canMove(dir)) {
      c->privateMessage(it + " won't move in this direction");
      return;
    }
    c->privateMessage("You push " + something);
    if (stopped || dir == direction) {
      direction = dir;
      creature->setSpeed(100 * c->getAttr(AttrType::STRENGTH) / 30);
      stopped = false;
    }
  }

  private:
  Vec2 direction;
  bool stopped = false;
  Tribe* myTribe = nullptr;
};

class Boulder : public Creature {
  public:
  Boulder(const ViewObject& obj, const CreatureAttributes& attr, Vec2 dir) : 
    Creature(obj, Tribe::killEveryone, attr, ControllerFactory([dir](Creature* c) { 
            return new BoulderController(c, dir); })) {}

  Boulder(const ViewObject& obj, const CreatureAttributes& attr, Tribe* myTribe) : 
    Creature(obj, myTribe, attr, ControllerFactory([myTribe](Creature* c) { 
            return new BoulderController(c, myTribe); })) {}

  virtual void dropCorpse() override {
    drop(ItemFactory::fromId(ItemId::ROCK, Random.getRandom(100, 200)));
  }
};

PCreature CreatureFactory::getRollingBoulder(Vec2 direction) {
  return PCreature(new Boulder(
        ViewObject(ViewId::BOULDER, ViewLayer::CREATURE, "Boulder"), CATTR(
            c.dexterity = 1;
            c.strength = 1000;
            c.weight = 1000;
            c.humanoid = false;
            c.size = CreatureSize::LARGE;
            c.speed = 200;
            c.permanentlyBlind = true;
            c.stationary = true;
            c.noSleep = true;
            c.invincible = true;
            c.breathing = false;
            c.name = "boulder";), direction));
}

PCreature CreatureFactory::getGuardingBoulder(Tribe* tribe) {
  return PCreature(new Boulder(
        ViewObject(ViewId::BOULDER, ViewLayer::CREATURE, "Boulder"), CATTR(
            c.dexterity = 1;
            c.strength = 1000;
            c.weight = 1000;
            c.humanoid = false;
            c.size = CreatureSize::LARGE;
            c.speed = 200;
            c.permanentlyBlind = true;
            c.noSleep = true;
            c.stationary = true;
            c.invincible = true;
            c.breathing = false;
            c.name = "boulder";), tribe));
}

class KrakenController : public Monster {
  public:
  KrakenController(Creature* c) : Monster(c, MonsterAIFactory::monster()) {
    numSpawns = chooseRandom({1, 2}, {4, 1});
  }

  void makeReady() {
    ready = true;
  }
  
  void unReady() {
    ready = false;
  }
  
  virtual void onKilled(const Creature* attacker) override {
    if (attacker)
      attacker->privateMessage("You cut the kraken's tentacle");
    for (Creature* c : spawns)
      if (!c->isDead())
        c->die(nullptr);
  }

  virtual void you(MsgType type, const string& param) const override {
    string msg, msgNoSee;
    switch (type) {
      case MsgType::KILLED_BY: msg = "The kraken's tentacle is cut by " + param; break;
      case MsgType::DIE:
      case MsgType::DIE_OF: return;
      default: Monster::you(type, param); break;
    }
    if (!msg.empty())
      creature->globalMessage(msg, msgNoSee);
  }

  virtual void makeMove() override {
    int radius = 10;
    if (waitNow) {
      creature->wait();
      waitNow = false;
      return;
    }
    for (Creature* c : spawns)
      if (c->isDead()) {
        ++numSpawns;
        removeElement(spawns, c);
        break;
      }
    if (held && ((held->getPosition() - creature->getPosition()).length8() != 1 || held->isDead()))
      held = nullptr;
    if (held) {
      held->you(MsgType::HAPPENS_TO, creature->getTheName() + " pulls");
      if (father) {
        held->setHeld(father->creature);
        father->held = held;
        creature->die(nullptr, false);
        creature->getLevel()->moveCreature(held, creature->getPosition() - held->getPosition());
      } else {
        held->you(MsgType::ARE, "eaten by " + creature->getTheName());
        held->die();
      }
    }
    if (numSpawns > 0)
    for (Vec2 v: Rectangle(Vec2(-radius, -radius), Vec2(radius + 1, radius + 1)))
      if (creature->getLevel()->inBounds(creature->getPosition() + v))
        if (Creature * c = creature->getSquare(v)->getCreature())
        if (creature->canSee(c) && creature->isEnemy(c) && !creature->isStationary()) {
          if (v.length8() == 1) {
            if (ready) {
              c->you(MsgType::HAPPENS_TO, creature->getTheName() + " swings itself around");
              c->setHeld(creature);
              held = c;
              unReady();
            } else
              makeReady();
            break;
          }
          PCreature spawn = CreatureFactory::fromId(CreatureId::KRAKEN, creature->getTribe());
          pair<Vec2, Vec2> dirs = v.approxL1();
          vector<Vec2> moves;
          if (creature->getSquare(dirs.first)->canEnter(spawn.get()))
            moves.push_back(dirs.first);
          if (creature->getSquare(dirs.second)->canEnter(spawn.get()))
            moves.push_back(dirs.second);
          if (!moves.empty()) {
            if (!ready) {
              makeReady();
            } else {
              Vec2 move = chooseRandom(moves);
              spawns.push_back(spawn.get());
              dynamic_cast<KrakenController*>(spawn->getController())->father = this;
              creature->getLevel()->addCreature(creature->getPosition() + move, std::move(spawn));
              --numSpawns;
              unReady();
            }
          } else
            unReady();
          break;
        }
    creature->wait();
  }
  private:
  int numSpawns;
  bool waitNow = true;
  bool ready = false;
  Creature* held = nullptr;
  vector<Creature*> spawns;
  KrakenController* father = nullptr;
};

/*class Shapechanger : public Monster {
  public:
  Shapechanger(const ViewObject& obj, Tribe* tribe, const CreatureAttributes& attr, vector<CreatureId> _creatures)
      : Monster(obj, tribe, attr, MonsterAIFactory::monster()), creatures(_creatures) {}

  virtual void makeMoveSpecial() override {
    int radius = 3;
    for (Vec2 v: Rectangle(Vec2(-radius, -radius), Vec2(radius + 1, radius + 1)))
      if (getLevel()->inBounds(getPosition() + v))
        if (Creature* enemy = getSquare(v)->getCreature())
          if (canSee(enemy) && isEnemy(enemy) && enemy->isPlayer()) {
            PCreature c = CreatureFactory::fromId(chooseRandom(creatures), getTribe());
            enemy->you(MsgType::ARE, "frozen in place by " + getTheName() + "!");
            enemy->setHeld(c.get());
            globalMessage(getTheName() + " turns into " + c->getAName());
            Vec2 pos = getPosition();
            die(nullptr, false);
            getLevel()->addCreature(pos, std::move(c));
            return;
          }
    Monster::makeMoveSpecial();
  }

  private:
  vector<CreatureId> creatures;
};*/

class KamikazeController : public Monster {
  public:
  KamikazeController(Creature* c, MonsterAIFactory f) : Monster(c, f) {}

  virtual void makeMove() override {
    for (Vec2 v : Vec2::directions8())
      if (creature->getLevel()->inBounds(creature->getPosition() + v))
        if (Creature* c = creature->getSquare(v)->getCreature())
          if (creature->isEnemy(c) && creature->canSee(c)) {
            creature->globalMessage(creature->getTheName() + " explodes!");
            c->getSquare()->setOnFire(1);
            creature->die(nullptr, false);
            return;
          }
    Monster::makeMove();
  }
};

class ShopkeeperController : public Monster, public EventListener {
  public:

  ShopkeeperController(Creature* c, Location* area) : Monster(c, MonsterAIFactory::stayInLocation(area)), shopArea(area) {
    EventListener::addListener(this);
  }

  virtual void makeMove() override {
    if (creature->getLevel() != shopArea->getLevel()) {
      Monster::makeMove();
      return;
    }
    if (firstMove) {
      for (Vec2 v : shopArea->getBounds())
        for (Item* item : creature->getLevel()->getSquare(v)->getItems()) {
          myItems.push_back(item);
          item->setUnpaid(true);
        }
      firstMove = false;
    }
    vector<const Creature*> creatures;
    for (Vec2 v : shopArea->getBounds())
      if (const Creature* c = creature->getLevel()->getSquare(v)->getCreature()) {
        creatures.push_back(c);
        if (!prevCreatures.count(c) && !thieves.count(c) && !creature->isEnemy(c)) {
          if (!debt.count(c))
            c->privateMessage("\"Welcome to " + *creature->getFirstName() + "'s shop!\"");
          else {
            c->privateMessage("\"Pay your debt or... !\"");
            thiefCount.erase(c);
          }
        }
      }
    for (auto elem : debt) {
      const Creature* c = elem.first;
      if (!contains(creatures, c)) {
        c->privateMessage("\"Come back, you owe me " + convertToString(elem.second) + " zorkmids!\"");
        if (++thiefCount[c] == 4) {
          c->privateMessage("\"Thief! Thief!\"");
          creature->getTribe()->onItemsStolen(c);
          thiefCount.erase(c);
          debt.erase(c);
          thieves.insert(c);
          for (Item* item : c->getEquipment().getItems())
            if (contains(unpaidItems[c], item))
              item->setUnpaid(false);
          break;
        }
      }
    }
    prevCreatures.clear();
    prevCreatures.insert(creatures.begin(), creatures.end());
    Monster::makeMove();
  }

  virtual void onItemsAppeared(vector<Item*> items, const Creature* from) {
    for (Item* item : items) {
      CHECK(item->getType() == ItemType::GOLD);
      --debt[from];
    }
    creature->pickUp(items, false);
    CHECK(debt[from] == 0) << "Bad debt " << debt[from];
    debt.erase(from);
    for (Item* it : from->getEquipment().getItems())
      if (contains(unpaidItems[from], it)) {
        it->setUnpaid(false);
        removeElement(myItems, it);
      }
    unpaidItems.erase(from);
  }
  
  virtual void onItemsAppeared(Vec2 position, const vector<Item*>& items) override {
    if (position.inRectangle(shopArea->getBounds())) {
      myItems.insert(myItems.end(), items.begin(), items.end());
      for (Item* it : items)
        it->setUnpaid(true);
    }
  }

  virtual void onPickupEvent(const Creature* c, const vector<Item*>& items) override {
    if (c->getPosition().inRectangle(shopArea->getBounds())) {
      for (const Item* it1 : items)
        if (Optional<int> elem  = findElement(myItems, it1)) {
          Item* item = myItems[*elem];
          debt[c] += item->getPrice();
          unpaidItems[c].push_back(item);
        }
    }
  }

  virtual void onDropEvent(const Creature* c, const vector<Item*>& items) override {
    if (c->getPosition().inRectangle(shopArea->getBounds())) {
      for (const Item* it1 : items)
        if (Optional<int> elem  = findElement(myItems, it1)) {
          Item* item = myItems[*elem];
          if ((debt[c] -= item->getPrice()) == 0)
            debt.erase(c);
          removeElement(unpaidItems[c], item);
        }
    }
  }

  virtual const Level* getListenerLevel() const override {
    return creature->getLevel();
  }

  virtual void onKilled(const Creature* attacker) override {
    EventListener::removeListener(this);
    for (Item* item : myItems)
      item->setUnpaid(false);
  }

  virtual int getDebt(const Creature* debtor) const override {
    if (debt.count(debtor)) {
      return debt.at(debtor);
    }
    else {
      return 0;
    }
  }

  private:
  unordered_set<const Creature*> prevCreatures;
  unordered_map<const Creature*, int> debt;
  unordered_map<const Creature*, int> thiefCount;
  unordered_set<const Creature*> thieves;
  unordered_map<const Creature*, vector<Item*>> unpaidItems;
  Location* shopArea;
  vector<Item*> myItems;
  bool firstMove = true;
};

class VillageElder : public Creature {
  public:
  VillageElder(vector<pair<Skill*, double>> _teachSkills, vector<Quest*> _quests,
      const ViewObject& object, Tribe* t, const CreatureAttributes& attr, ControllerFactory factory) : 
      Creature(object, t, attr, factory), teachSkills(_teachSkills), quests(_quests) {
    getTribe()->addImportantMember(this);
  }

  bool teach(Creature* who) {
    for (auto elem : teachSkills)
      if (!who->hasSkill(elem.first) && getTribe()->getStanding(who) >= elem.second) {
        Skill* teachSkill = elem.first;
        who->addSkill(teachSkill);
        who->privateMessage("\"You are a friend of our tribe. I will share my knowledge with you.\"");
        who->privateMessage(getName() + " teaches you the " + teachSkill->getName());
        if (teachSkill == Skill::archery) {
          who->privateMessage(getName() + " hands you a bow and a quiver of arrows.");
          who->take(std::move(ItemFactory::fromId(ItemId::BOW)));
          who->take(ItemFactory::fromId(ItemId::ARROW, Random.getRandom(20, 36)));
        }
        return true;
      }
    return false;
  }

  bool tribeQuest(Creature* who) {
    for (Quest* q : quests) {
      if (q->isFinished()) {  
        who->privateMessage("\"" + (*who->getFirstName()) + ", you have fulfilled your quest.\"");
        removeElement(quests, q);
        return true;
      }
      string message = MessageBuffer::important(q->getMessage());
      if (const Location* loc = q->getLocation())
        message += " You need to head " + (loc->getBounds().middle() - getPosition()).getBearing() + " to find it.";
      who->privateMessage(message);
      q->addAdventurer(who);
      return true;
    }
    return false;
  }

  virtual void onChat(Creature* who) override {
    if (isEnemy(who)) {
      Creature::onChat(who);
      return;
    }
    if (tribeQuest(who))
      return;
    Creature::onChat(who);
    return;
 //   if (teach(who))
 //     return;
    const vector<Location*> locations = getLevel()->getAllLocations();
    if (locations.size() == 0)
      Creature::onChat(who);
    else
      while (1) {
        Location* l = locations[Random.getRandom(locations.size())];
        if (l->hasName() && l != getLevel()->getLocation(getPosition())) {
          Vec2 dir = l->getBounds().middle() - getPosition();
          string dist = dir.lengthD() > 150 ? "far" : "close";
          string bearing = dir.getBearing();
          who->privateMessage("\"There is a " + l->getName() + " " + dist + " in the " + bearing + "\"");
          who->privateMessage("\"" + l->getDescription() + "\"");
          return;
        }
      }
  }

  private:
  vector<pair<Skill*, double>> teachSkills;
  vector<Quest*> quests;
  const Item* bringItem = nullptr;
  const Creature* killCreature = nullptr;
};

class DragonController : public Monster {
  public:
  using Monster::Monster;

  virtual void makeMove() override {
    Effect::applyToCreature(creature, EffectType::EMIT_POISON_GAS, EffectStrength::WEAK);
    Monster::makeMove();
  }
};

PCreature CreatureFactory::addInventory(PCreature c, const vector<ItemId>& items) {
  for (ItemId item : items) {
    PItem it = ItemFactory::fromId(item);
    Item* ref = it.get();
    c->take(std::move(it));
  }
  return c;
}

PCreature CreatureFactory::getShopkeeper(Location* shopArea, Tribe* tribe) {
  PCreature ret(new Creature(
      ViewObject(tribe == Tribe::dwarven ? ViewId::DWARVEN_SHOPKEEPER : ViewId::ELVEN_SHOPKEEPER,
          ViewLayer::CREATURE, "Shopkeeper"), tribe,
      CATTR(
        c.speed = 100;
        c.size = CreatureSize::LARGE;
        c.strength = 17;
        c.dexterity = 13;
        c.barehandedDamage = 13;
        c.humanoid = true;
        c.weight = 100;
        c.chatReactionFriendly = "complains about high import tax";
        c.chatReactionHostile = "\"Die!\"";
        c.name = "shopkeeper";
        c.firstName = NameGenerator::firstNames.getNext();),
      ControllerFactory([shopArea](Creature* c) { 
          return new ShopkeeperController(c, shopArea); })));
  vector<ItemId> inventory(Random.getRandom(100, 300), ItemId::GOLD_PIECE);
  inventory.push_back(ItemId::SWORD);
  inventory.push_back(ItemId::LEATHER_ARMOR);
  inventory.push_back(ItemId::LEATHER_BOOTS);
  inventory.push_back(ItemId::HEALING_POTION);
  inventory.push_back(ItemId::HEALING_POTION);
  return addInventory(std::move(ret), inventory);
}


PCreature CreatureFactory::random(MonsterAIFactory actorFactory) {
  if (unique.size() > 0) {
    CreatureId id = unique.back();
    unique.pop_back();
    return fromId(id, tribe, actorFactory);
  }
  return fromId(chooseRandom(creatures, weights), tribe, actorFactory);
}

PCreature get(ViewId viewId,
    CreatureAttributes attr, 
    Tribe* tribe,
    ControllerFactory factory) {
  return PCreature(new Creature(ViewObject(viewId, ViewLayer::CREATURE, *attr.name), tribe, attr, factory));
}

CreatureFactory::CreatureFactory(Tribe* t, const vector<CreatureId>& c, const vector<double>& w,
    const vector<CreatureId>& u)
    : tribe(t), creatures(c), weights(w), unique(u) {
}

CreatureFactory CreatureFactory::humanVillagePeaceful() {
  return CreatureFactory(Tribe::human, { CreatureId::PESEANT,
      CreatureId::CHILD, CreatureId::HORSE, CreatureId::COW, CreatureId::PIG },
      { 2, 1, 1, 1, 1}, {});
}

CreatureFactory CreatureFactory::humanVillage() {
  return CreatureFactory(Tribe::human, { CreatureId::PESEANT, CreatureId::KNIGHT,
      CreatureId::CHILD, CreatureId::HORSE, CreatureId::COW, CreatureId::PIG },
      { 2, 1, 1, 1, 1, 1}, { CreatureId::AVATAR});
}

CreatureFactory CreatureFactory::elvenVillage() {
  return CreatureFactory(Tribe::elven, { CreatureId::ELF, CreatureId::ELF_CHILD, CreatureId::HORSE, CreatureId::COW },
      { 2, 2, 1, 1}, { CreatureId::ELF_LORD});
}

CreatureFactory CreatureFactory::elvenVillagePeaceful() {
  return CreatureFactory(Tribe::elven, { CreatureId::ELF_CHILD, CreatureId::HORSE, CreatureId::COW },
      { 2, 1, 1}, {});
}

CreatureFactory CreatureFactory::forrest() {
  return CreatureFactory(Tribe::wildlife,
      { CreatureId::DEER, CreatureId::FOX, CreatureId::BOAR, CreatureId::LEPRECHAUN },
      { 4, 2, 2, 1}, {});
}

CreatureFactory CreatureFactory::crypt() {
  return CreatureFactory(Tribe::monster, { CreatureId::ZOMBIE}, { 1}, {});
}

CreatureFactory CreatureFactory::hellLevel() {
  return CreatureFactory(Tribe::monster, { CreatureId::DRAGON}, { 1}, {CreatureId::DARK_KNIGHT});
}

CreatureFactory CreatureFactory::collectiveUndead() {
  return CreatureFactory(Tribe::player, { CreatureId::ZOMBIE, CreatureId::VAMPIRE, CreatureId::MUMMY},
      {1, 1, 1}, {});
}

CreatureFactory CreatureFactory::collectiveStart() {
  return CreatureFactory(Tribe::player, { CreatureId::IMP}, { 1}, {CreatureId::DUNGEON_HEART} );
}

CreatureFactory CreatureFactory::collectiveMinions() {
  return CreatureFactory(Tribe::player, { CreatureId::GNOME, CreatureId::BILE_DEMON, CreatureId::HELL_HOUND,
      CreatureId::SPECIAL_MONSTER}, { 3, 4, 5, 1}, {CreatureId::SPECIAL_MONSTER_HUMANOID});
}

CreatureFactory CreatureFactory::collectiveEnemies() {
  return CreatureFactory(Tribe::human, { CreatureId::KNIGHT, CreatureId::ARCHER}, { 1, 1}, {});
}

CreatureFactory CreatureFactory::collectiveFinalAttack() {
  return CreatureFactory(Tribe::human, { CreatureId::KNIGHT, CreatureId::ARCHER}, { 1, 1}, {CreatureId::AVATAR});
}

CreatureFactory CreatureFactory::collectiveElfEnemies() {
  return CreatureFactory(Tribe::elven, { CreatureId::ELF}, { 1}, {});
}

CreatureFactory CreatureFactory::collectiveElfFinalAttack() {
  return CreatureFactory(Tribe::elven, { CreatureId::ELF, }, { 1}, {CreatureId::ELF_LORD});
}

CreatureFactory CreatureFactory::collectiveSurpriseEnemies() {
  return CreatureFactory(Tribe::monster, { CreatureId::SPECIAL_MONSTER}, { 1}, {});
}

CreatureFactory CreatureFactory::dwarfTown(int num) {
  int maxLevel = 3;
  CHECK(num <= maxLevel && num > 0);
  map<CreatureId, vector<int>> frequencies {
      { CreatureId::GNOME, { 0, 1, 1}},
      { CreatureId::JACKAL, { 0, 1, 1 }},
      { CreatureId::RAT, { 2, 1, 1}},
      { CreatureId::BAT, { 0, 1, 1 }},
      { CreatureId::DWARF, { 6, 1, 1 }},
      { CreatureId::GOBLIN, { 1, 1, 1 }}};
  vector<vector<CreatureId>> uniqueMonsters(maxLevel);
  uniqueMonsters[0].push_back(CreatureId::DWARF_BARON);
  vector<CreatureId> ids;
  vector<double> freq;
  for (auto elem : frequencies) {
    ids.push_back(elem.first);
    freq.push_back(elem.second[num - 1]);
  }
  return CreatureFactory(Tribe::monster, ids, freq, uniqueMonsters[num - 1]);
}

CreatureFactory CreatureFactory::goblinTown(int num) {
  int maxLevel = 3;
  CHECK(num <= maxLevel && num > 0);
  map<CreatureId, vector<int>> frequencies {
      { CreatureId::GNOME, { 0, 1, 1}},
      { CreatureId::JACKAL, { 0, 1, 1 }},
      { CreatureId::RAT, { 1, 1, 1}},
      { CreatureId::BAT, { 0, 1, 1 }},
      { CreatureId::GOBLIN, { 2, 1, 1 }}};
  vector<vector<CreatureId>> uniqueMonsters(maxLevel);
  uniqueMonsters[0].push_back(CreatureId::GREAT_GOBLIN);
  vector<CreatureId> ids;
  vector<double> freq;
  for (auto elem : frequencies) {
    ids.push_back(elem.first);
    freq.push_back(elem.second[num - 1]);
  }
  return CreatureFactory(Tribe::monster, ids, freq, uniqueMonsters[num - 1]);
}

CreatureFactory CreatureFactory::pyramid(int level) {
  if (level == 2)
    return CreatureFactory(Tribe::monster, { CreatureId::MUMMY }, {1}, { CreatureId::MUMMY_LORD });
  else
    return CreatureFactory(Tribe::monster, { CreatureId::MUMMY }, {1}, { });
}

CreatureFactory CreatureFactory::level(int num) {
  int maxLevel = 8;
  CHECK(num <= maxLevel && num > 0);
  map<CreatureId, vector<int>> frequencies {
      { CreatureId::SPECIAL_MONSTER, { 5, 8, 10, 12, 15, 18, 20, 22}},
      { CreatureId::GNOME, { 400, 200, 100, 100, 100, 100, 100, 100, 100, 100}},
      { CreatureId::LEPRECHAUN, { 20, 20, 20, 20, 20, 20, 20, 20}},
      { CreatureId::GOBLIN, { 20, 20, 30, 50, 50, 100, 200, 400 }},
      { CreatureId::BILE_DEMON, { 0, 0, 100, 100, 200, 200, 200, 200, 200, 200 }},
      { CreatureId::JACKAL, { 400, 100, 100, 100, 100, 100, 100, 100, 100, 100 }},
      { CreatureId::SPIDER, { 400, 100, 100, 100, 100, 100, 100, 100, 100, 100 }},
      { CreatureId::RAT, { 200, 200, 200, 200, 200, 200, 200, 200, 200, 200 }},
      { CreatureId::BAT, { 100, 100, 200, 200, 200, 200, 200, 200, 200, 200 }},
      { CreatureId::ZOMBIE, { 0, 0, 0, 30, 50, 100, 100, 100, 100, 100 }},
      { CreatureId::VAMPIRE_BAT, { 0, 0, 0, 10, 30, 50, 100, 100, 100, 100 }},
      { CreatureId::NIGHTMARE, { 5, 5, 10, 10, 20, 30, 30, 40, 40, 40 }},
      { CreatureId::DWARF, { 400, 200, 100, 50, 50, 30, 20, 20 }}};
  vector<vector<CreatureId>> uniqueMonsters(maxLevel);
  uniqueMonsters[Random.getRandom(5, maxLevel)].push_back(CreatureId::SPECIAL_MONSTER);
  vector<CreatureId> ids;
  vector<double> freq;
  for (auto elem : frequencies) {
    ids.push_back(elem.first);
    freq.push_back(elem.second[num - 1]);
  }

  return CreatureFactory(Tribe::monster, ids, freq, uniqueMonsters[num - 1]);
}

CreatureFactory CreatureFactory::singleType(Tribe* tribe, CreatureId id) {
  return CreatureFactory(tribe, { id}, {1}, {});
}

vector<PCreature> CreatureFactory::getFlock(int size, CreatureId id, Creature* leader) {
  vector<PCreature> ret;
  for (int i : Range(size)) {
    PCreature c = fromId(id, leader->getTribe(), MonsterAIFactory::follower(leader, 5));
    ret.push_back(std::move(c));
  }
  return ret;
}

PCreature getSpecial(const string& name, Tribe* tribe, bool humanoid, ControllerFactory factory) {
  RandomGen r;
  r.init(hash<string>()(name));
  PCreature c = get(humanoid ? ViewId::SPECIAL_HUMANOID : ViewId::SPECIAL_BEAST, CATTR(
        c.speed = r.getRandom(70, 150);
        c.size = chooseRandom({CreatureSize::SMALL, CreatureSize::MEDIUM, CreatureSize::LARGE}, {1, 1, 1});
        c.strength = r.getRandom(14, 21);
        c.dexterity = r.getRandom(14, 21);
        c.barehandedDamage = r.getRandom(10);
        c.humanoid = humanoid;
        c.weight = c.size == CreatureSize::LARGE ? r.getRandom(80,120) : 
                   c.size == CreatureSize::MEDIUM ? r.getRandom(40, 60) :
                   r.getRandom(5, 20);
        if (*c.humanoid) {
          c.chatReactionFriendly = "\"I am the mighty " + name + "\"";
          c.chatReactionHostile = "\"I am the mighty " + name + ". Die!\"";
        } else {
          c.chatReactionFriendly = c.chatReactionHostile = "The " + name + " snarls.";
        }
        c.name = name;
        if (Random.roll(10)) {
          c.noBody = true;
          c.wings = c.arms = c.legs = 0;
          *c.strength -= 5;
          *c.dexterity += 10;
          c.barehandedDamage += 10;
        } else {
          c.wings = r.roll(4) ? 2 : 0;
          if (c.wings)
            c.flyer = true;
          if (*c.humanoid == false) {
            c.arms = r.roll(2) ? 2 : 0;
            c.legs = r.getRandom(3) * 2;
            *c.strength += 5;
            *c.dexterity += 5;
            c.barehandedDamage += 5;
            switch (Random.getRandom(8)) {
              case 0: c.attackEffect = EffectType::POISON; break;
              case 1: c.attackEffect = EffectType::FIRE; c.barehandedAttack = AttackType::HIT; break;
              default: break;
            }
          }
          if (Random.roll(10))
            c.undead = true;
        }
        if (r.roll(3))
          c.skills.insert(Skill::swimming);
        c.specialMonster = true;
        ), tribe, factory);
  if (c->isHumanoid()) {
    if (Random.roll(400)) {
      c->take(ItemFactory::fromId(ItemId::BOW));
      c->take(ItemFactory::fromId(ItemId::ARROW, Random.getRandom(20, 36)));
    } else
      c->take(ItemFactory::fromId(chooseRandom(
            {ItemId::SWORD, ItemId::BATTLE_AXE, ItemId::WAR_HAMMER})));
  } else {
    switch (Random.getRandom(3)) {
      case 0:
        c->take(ItemFactory::fromId(
              chooseRandom({ItemId::WARNING_AMULET, ItemId::HEALING_AMULET, ItemId::DEFENSE_AMULET})));
        break;
      case 1:
        c->take(ItemFactory::fromId(ItemId::INVISIBLE_POTION, Random.getRandom(3, 6)));
        break;
      case 2:
        c->take(ItemFactory::fromId(
              chooseRandom({ItemId::STRENGTH_MUSHROOM, ItemId::DEXTERITY_MUSHROOM}), Random.getRandom(3, 6)));
        break;
      default:
        Debug(FATAL) << "Unhandled case value";
    }

  }
  Debug() << c->getDescription();
  return c;
}

PCreature get(CreatureId id, Tribe* tribe, MonsterAIFactory actorFactory) {
  ControllerFactory factory = Monster::getFactory(actorFactory);
  switch (id) {
    case CreatureId::GOBLIN: return get(ViewId::GOBLIN, CATTR(
                                c.speed = 100;
                                c.size = CreatureSize::LARGE;
                                c.strength = 16;
                                c.dexterity = 14;
                                c.barehandedDamage = 3;
                                c.humanoid = true;
                                c.weight = 100;
                                c.skills.insert(Skill::twoHandedWeapon);
                                c.chatReactionFriendly = "curses all elves";
                                c.chatReactionHostile = "\"Die!\"";
                                c.name = "goblin";), Tribe::goblin, factory);
    case CreatureId::BANDIT: return get(ViewId::BANDIT, CATTR(
                                c.speed = 80;
                                c.size = CreatureSize::LARGE;
                                c.strength = 15;
                                c.dexterity = 13;
                                c.barehandedDamage = 3;
                                c.humanoid = true;
                                c.weight = 100;
                                c.chatReactionFriendly = "curses all law enforcement";
                                c.chatReactionHostile = "\"Die!\"";
                                c.name = "bandit";), tribe, factory);
    case CreatureId::GHOST: return get(ViewId::GHOST, CATTR(
                                c.speed = 80;
                                c.size = CreatureSize::LARGE;
                                c.strength = 14;
                                c.dexterity = 19;
                                c.barehandedDamage = 3;
                                c.barehandedAttack = AttackType::HIT;
                                c.humanoid = false;
                                c.noBody = true;
                                c.weight = 10;
                                c.chatReactionFriendly = "\"Wouuuouuu!!!\"";
                                c.chatReactionHostile = "\"Wouuuouuu!!!\"";
                                c.name = "ghost";), tribe, factory);
    case CreatureId::DEVIL: return get(ViewId::DEVIL, CATTR(
                                c.speed = 100;
                                c.size = CreatureSize::LARGE;
                                c.strength = 19;
                                c.dexterity = 16;
                                c.barehandedDamage = 10;
                                c.humanoid = true;
                                c.weight = 80;
                                c.chatReactionFriendly = "curses all dungeons";
                                c.chatReactionHostile = "\"Die!\"";
                                c.name = "devil";), tribe, factory);
    case CreatureId::DARK_KNIGHT: return get(ViewId::DARK_KNIGHT, CATTR(
                                c.speed = 100;
                                c.size = CreatureSize::LARGE;
                                c.strength = 22;
                                c.dexterity = 19;
                                c.barehandedDamage = 3;
                                c.humanoid = true;
                                c.weight = 100;
                                c.chatReactionFriendly = "curses all dungeons";
                                c.chatReactionHostile = "\"Die!\"";
                                c.name = "dark knight";), tribe, factory);
    case CreatureId::CYCLOPS: return get(ViewId::CYCLOPS, CATTR(
                                c.speed = 100;
                                c.size = CreatureSize::LARGE;
                                c.strength = 25;
                                c.dexterity = 18;
                                c.barehandedDamage = 10;
                                c.humanoid = true;
                                c.weight = 150;
                                c.firstName = NameGenerator::demonNames.getNext();
                                c.name = "cyclops";), tribe, factory);
    case CreatureId::DRAGON: return get(ViewId::DRAGON, CATTR(
                                c.speed = 100;
                                c.size = CreatureSize::LARGE;
                                c.strength = 27;
                                c.dexterity = 15;
                                c.barehandedDamage = 10;
                                c.humanoid = false;
                                c.weight = 1000;
                                c.wings = 2;
                                c.breathing = false;
                                c.firstName = NameGenerator::demonNames.getNext();
                                c.name = "dragon";), tribe, ControllerFactory([=](Creature* c) {
                                      return new DragonController(c, actorFactory);
                                    }));
    case CreatureId::KNIGHT: return get(ViewId::KNIGHT, CATTR(
                                c.speed = 100;
                                c.size = CreatureSize::LARGE;
                                c.strength = 19;
                                c.dexterity = 16;
                                c.barehandedDamage = 3;
                                c.humanoid = true;
                                c.weight = 100;
                                c.chatReactionFriendly = "curses all dungeons";
                                c.chatReactionHostile = "\"Die!\"";
                                c.name = "knight";), tribe, factory);
    case CreatureId::CASTLE_GUARD: return get(ViewId::CASTLE_GUARD, CATTR(
                                c.speed = 100;
                                c.size = CreatureSize::LARGE;
                                c.strength = 19;
                                c.dexterity = 16;
                                c.barehandedDamage = 3;
                                c.humanoid = true;
                                c.weight = 100;
                                c.chatReactionFriendly = "curses all dungeons";
                                c.chatReactionHostile = "\"Die!\"";
                                c.name = "guard";), tribe, factory);
    case CreatureId::AVATAR: return PCreature(new VillageElder({}, {Quest::castleCellar, Quest::dragon},
                                ViewObject(ViewId::AVATAR, ViewLayer::CREATURE, "Duke"), tribe, CATTR(
                                c.speed = 100;
                                c.size = CreatureSize::LARGE;
                                c.strength = 23;
                                c.dexterity = 18;
                                c.barehandedDamage = 8;
                                c.humanoid = true;
                                c.weight = 120;
                                c.chatReactionFriendly = "curses all dungeons";
                                c.chatReactionHostile = "\"Die!\"";
                                c.name = "Duke of " + NameGenerator::worldNames.getNext();), factory));
    case CreatureId::ARCHER: return get(ViewId::ARCHER, CATTR(
                                c.speed = 100;
                                c.size = CreatureSize::LARGE;
                                c.strength = 16;
                                c.dexterity = 20;
                                c.barehandedDamage = 3;
                                c.humanoid = true;
                                c.weight = 100;
                                c.chatReactionFriendly = "curses all dungeons";
                                c.chatReactionHostile = "\"Die!\"";
                                c.name = "archer";), tribe, factory);
    case CreatureId::PESEANT: return get(ViewId::PESEANT, CATTR(
                                c.speed = 80;
                                c.size = CreatureSize::LARGE;
                                c.strength = 14;
                                c.dexterity = 12;
                                c.barehandedDamage = 3;
                                c.humanoid = true;
                                c.weight = 100;
                                c.chatReactionFriendly = "curses all dungeons";
                                c.chatReactionHostile = "\"Heeelp!\"";
                                c.name = "peseant";), tribe, factory);
    case CreatureId::CHILD: return get(ViewId::CHILD, CATTR(
                                c.speed = 140;
                                c.size = CreatureSize::LARGE;
                                c.strength = 8;
                                c.dexterity = 16;
                                c.barehandedDamage = 3;
                                c.humanoid = true;
                                c.weight = 40;
                                c.chatReactionFriendly = "plaaaaay!";
                                c.chatReactionHostile = "\"Heeelp!\"";
                                c.name = "child";), tribe, factory);
    case CreatureId::ZOMBIE: return get(ViewId::ZOMBIE, CATTR(
                                c.speed = 60;
                                c.size = CreatureSize::LARGE;
                                c.strength = 12;
                                c.dexterity = 13;
                                c.barehandedDamage = 13;
                                c.humanoid = true;
                                c.weight = 100;
                                c.undead = true;
                                c.name = "zombie";), tribe, factory);
    case CreatureId::VAMPIRE_BAT:
    case CreatureId::VAMPIRE: return get(ViewId::VAMPIRE, CATTR(
                                c.speed = 120;
                                c.size = CreatureSize::LARGE;
                                c.strength = 15;
                                c.dexterity = 15;
                                c.barehandedDamage = 2;
                                c.humanoid = true;
                                c.weight = 100;
                                c.undead = true;
                                c.flyer = true;
                                c.chatReactionFriendly = "curses all humans";
                                c.chatReactionHostile = "\"Die!\"";
                                c.name = "vampire";), tribe, factory);
 /*   case CreatureId::VAMPIRE_BAT: return PCreature(new Shapechanger(
                                   ViewObject(ViewId::BAT, ViewLayer::CREATURE, "Bat"),
                                   tribe,
                                CATTR(
                                c.speed = 150;
                                c.size = CreatureSize::SMALL;
                                c.strength = 3;
                                c.dexterity = 16;
                                c.barehandedDamage = 12;
                                c.humanoid = false;
                                c.legs = 0;
                                c.arms = 0;
                                c.wings = 2;
                                c.weight = 1;
                                c.flyer = true;
                                c.name = "bat";), {
                                    CreatureId::VAMPIRE}
                                    ));*/
    case CreatureId::MUMMY: return get(ViewId::MUMMY, CATTR(
                                c.speed = 60;
                                c.size = CreatureSize::LARGE;
                                c.strength = 12;
                                c.dexterity = 13;
                                c.barehandedDamage = 13;
                                c.humanoid = true;
                                c.weight = 100;
                                c.undead = true;
                                c.skills.insert(Skill::twoHandedWeapon);
                                c.name = "mummy";), tribe, factory);
    case CreatureId::MUMMY_LORD: return get(ViewId::MUMMY_LORD, CATTR(
                                c.speed = 60;
                                c.size = CreatureSize::LARGE;
                                c.strength = 15;
                                c.dexterity = 13;
                                c.barehandedDamage = 13;
                                c.humanoid = true;
                                c.weight = 120;
                                c.undead = true;
                                c.skills.insert(Skill::twoHandedWeapon);
                                c.chatReactionFriendly = "curses all gravediggers";
                                c.chatReactionHostile = "\"Die!\"";
                                c.name = NameGenerator::aztecNames.getNext();), tribe, factory);
    case CreatureId::GREAT_GOBLIN: return PCreature(new VillageElder(
                                {},
                                {},
                                ViewObject(ViewId::GREAT_GOBLIN, ViewLayer::CREATURE, "Great goblin"), Tribe::goblin,
                                CATTR(
                                c.speed = 100;
                                c.size = CreatureSize::LARGE;
                                c.strength = 20;
                                c.dexterity = 13;
                                c.barehandedDamage = 5;
                                c.humanoid = true;
                                c.weight = 180;
                                c.skills.insert(Skill::twoHandedWeapon);
                                c.chatReactionFriendly = "curses all elves";
                                c.chatReactionHostile = "\"Die!\"";
                                c.name = "great goblin";), factory));
    case CreatureId::GNOME: return get(ViewId::GNOME, CATTR(
                                c.speed = 80;
                                c.size = CreatureSize::MEDIUM;
                                c.strength = 12;
                                c.dexterity = 13;
                                c.barehandedDamage = 3;
                                c.humanoid = true;
                                c.weight = 45;
                                c.chatReactionFriendly = "talks about digging";
                                c.chatReactionHostile = "\"Die!\"";
                                c.name = "gnome";), tribe, factory);
    case CreatureId::IMP: return get(ViewId::IMP, CATTR(
                                c.speed = 200;
                                c.size = CreatureSize::SMALL;
                                c.strength = 8;
                                c.dexterity = 15;
                                c.barehandedDamage = 3;
                                c.humanoid = true;
                                c.weight = 30;
                                c.courage = 0.1;
                                c.carryingMultiplier = 10;
                                c.skills.insert(Skill::construction);
                                c.chatReactionFriendly = "talks about digging";
                                c.chatReactionHostile = "\"Die!\"";
                                c.name = "imp";), tribe, factory);
    case CreatureId::BILE_DEMON: return get(ViewId::BILE_DEMON, CATTR(
                                c.speed = 80;
                                c.size = CreatureSize::LARGE;
                                c.strength = 24;
                                c.dexterity = 14;
                                c.barehandedDamage = 10;
                                c.humanoid = false;
                                c.weight = 140;
                                c.courage = 1;
                                c.firstName = NameGenerator::demonNames.getNext();
                                c.name = "ogre";), tribe, factory);
    case CreatureId::HELL_HOUND: return get(ViewId::HELL_HOUND, CATTR(
                                c.speed = 160;
                                c.size = CreatureSize::MEDIUM;
                                c.strength = 16;
                                c.dexterity = 17;
                                c.barehandedDamage = 5;
                                c.humanoid = false;
                                c.weight = 50;
                                c.courage = 1;
                                c.firstName = NameGenerator::dogNames.getNext();
                                c.name = "hell hound";), tribe, factory);
    case CreatureId::CHICKEN: return get(ViewId::CHICKEN, CATTR(
                                c.speed = 50;
                                c.size = CreatureSize::SMALL;
                                c.strength = 2;
                                c.dexterity = 2;
                                c.barehandedDamage = 5;
                                c.humanoid = false;
                                c.weight = 3;
                                c.courage = 1;
                                c.walker = false;
                                c.isFood = true;
                                c.name = "chicken";), tribe, factory);
    case CreatureId::DUNGEON_HEART: return get(ViewId::DUNGEON_HEART, CATTR(
                                c.speed = 100;
                                c.size = CreatureSize::LARGE;
                                c.strength = 23;
                                c.dexterity = 14;
                                c.barehandedDamage = 5;
                                c.humanoid = false;
                                c.legs = c.arms = c.heads = 0;
                                c.weight = 100;
                                c.walker = true;
                                c.stationary = true;
                                c.breathing = false;
                                c.damageMultiplier = 0.1;
                                c.name = "dungeon heart";), tribe, Monster::getFactory(MonsterAIFactory::idle()));
    case CreatureId::LEPRECHAUN: return get(ViewId::LEPRECHAUN, CATTR(
                                c.speed = 160;
                                c.size = CreatureSize::MEDIUM;
                                c.strength = 10;
                                c.dexterity = 16;
                                c.barehandedDamage = 3;
                                c.humanoid = true;
                                c.weight = 35;
                                c.courage = 20;
                                c.skills.insert(Skill::stealing);
                                c.chatReactionFriendly = "discusses the weather";
                                c.chatReactionHostile = "discusses the weather";
                                c.name = "leprechaun";), tribe, factory);
    case CreatureId::DWARF: return get(ViewId::DWARF, CATTR(
                                c.speed = 80;
                                c.size = CreatureSize::MEDIUM;
   //                             c.firstName = NameGenerator::dwarfNames.getNext();
                                c.strength = 18;
                                c.dexterity = 13;
                                c.barehandedDamage = 3;
                                c.humanoid = true;
                                c.weight = 90;
                                c.skills.insert(Skill::twoHandedWeapon);
                                c.chatReactionFriendly = "curses all goblins";
                                c.chatReactionHostile = "\"Die!\"";
                                c.name = "dwarf";), Tribe::dwarven, factory);
    case CreatureId::DWARF_BARON: return PCreature(new VillageElder(
                                {},
                                {},
                                ViewObject(ViewId::DWARF_BARON, ViewLayer::CREATURE, "Dwarf baron"), Tribe::dwarven, 
                                CATTR(
                                c.speed = 80;
                                c.size = CreatureSize::MEDIUM;
 //                               c.firstName = NameGenerator::dwarfNames.getNext();
                                c.strength = 22;
                                c.dexterity = 13;
                                c.barehandedDamage = 5;
                                c.humanoid = true;
                                c.weight = 120;
                                c.skills.insert(Skill::twoHandedWeapon);
                                c.chatReactionFriendly = "curses all goblins";
                                c.chatReactionHostile = "\"Die!\"";
                                c.name = "dwarf baron";), factory));
    case CreatureId::ELF: return get(ViewId::ELF, CATTR(
                                c.speed = 120;
                                c.size = CreatureSize::MEDIUM;
                                c.strength = 13;
                                c.dexterity = 15;
                                c.barehandedDamage = 3;
                                c.humanoid = true;
                                c.weight = 50;
                                c.skills.insert(Skill::archery);
                                c.chatReactionFriendly = "curses all dwarves";
                                c.chatReactionHostile = "\"Die!\"";
                                c.name = "elf";), Tribe::elven, factory);
    case CreatureId::ELF_CHILD: return get(ViewId::ELF_CHILD, CATTR(
                                c.speed = 150;
                                c.size = CreatureSize::MEDIUM;
                                c.strength = 7;
                                c.dexterity = 17;
                                c.barehandedDamage = 0;
                                c.humanoid = true;
                                c.weight = 25;
                                c.skills.insert(Skill::archery);
                                c.chatReactionFriendly = "curses all dwarves";
                                c.chatReactionHostile = "\"Help!\"";
                                c.name = "elf child";), Tribe::elven, factory);
    case CreatureId::ELF_LORD: return PCreature(new VillageElder(
                                {},
                                {Quest::bandits},
                                ViewObject(ViewId::ELF_LORD, ViewLayer::CREATURE, "Elf lord"), Tribe::elven, CATTR(
                                c.speed = 140;
                                c.size = CreatureSize::MEDIUM;
                                c.strength = 13;
                                c.dexterity = 18;
                                c.barehandedDamage = 3;
                                c.humanoid = true;
                                c.healer = true;
                                c.weight = 50;
                                c.skills.insert(Skill::archery);
                                c.chatReactionFriendly = "curses all dwarves";
                                c.chatReactionHostile = "\"Die!\"";
                                c.name = "elf lord";), factory));
    case CreatureId::HORSE: return get(ViewId::HORSE, CATTR(
                                c.speed = 100;
                                c.size = CreatureSize::LARGE;
                                c.strength = 10;
                                c.dexterity = 17;
                                c.humanoid = false;
                                c.weight = 500;
                                c.animal = true;
                                c.name = "horse";), tribe, factory);
    case CreatureId::COW: return get(ViewId::COW, CATTR(
                                c.speed = 40;
                                c.size = CreatureSize::LARGE;
                                c.strength = 10;
                                c.dexterity = 12;
                                c.humanoid = false;
                                c.weight = 400;
                                c.animal = true;
                                c.name = "cow";), tribe, factory);
    case CreatureId::SHEEP: return get(ViewId::SHEEP, CATTR(
                                c.speed = 80;
                                c.size = CreatureSize::MEDIUM;
                                c.strength = 5;
                                c.dexterity = 12;
                                c.humanoid = false;
                                c.weight = 40;
                                c.animal = true;
                                c.name = "sheep";), tribe, factory);
    case CreatureId::PIG: return get(ViewId::PIG, CATTR(
                                c.speed = 60;
                                c.size = CreatureSize::MEDIUM;
                                c.strength = 12;
                                c.dexterity = 8;
                                c.humanoid = false;
                                c.weight = 150;
                                c.animal = true;
                                c.name = "pig";), tribe, factory);
    case CreatureId::JACKAL: return get(ViewId::JACKAL, CATTR(
                                c.speed = 120;
                                c.size = CreatureSize::SMALL;
                                c.strength = 10;
                                c.dexterity = 15;
                                c.barehandedDamage = 2;
                                c.humanoid = false;
                                c.weight = 10;
                                c.animal = true;
                                c.name = "jackal";), tribe, factory);
    case CreatureId::DEER: return get(ViewId::DEER, CATTR(
                                c.speed = 200;
                                c.size = CreatureSize::LARGE;
                                c.strength = 10;
                                c.dexterity = 17;
                                c.humanoid = false;
                                c.weight = 400;
                                c.animal = true;
                                c.name = "deer";), tribe, factory);
    case CreatureId::BOAR: return get(ViewId::BOAR, CATTR(
                                c.speed = 180;
                                c.size = CreatureSize::MEDIUM;
                                c.strength = 18;
                                c.dexterity = 15;
                                c.humanoid = false;
                                c.weight = 200;
                                c.animal = true;
                                c.name = "boar";), tribe, factory);
    case CreatureId::FOX: return get(ViewId::FOX, CATTR(
                                c.speed = 140;
                                c.size = CreatureSize::SMALL;
                                c.strength = 10;
                                c.dexterity = 15;
                                c.barehandedDamage = 2;
                                c.humanoid = false;
                                c.weight = 10;
                                c.animal = true;
                                c.name = "fox";), tribe, factory);
    case CreatureId::CAVE_BEAR: return get(ViewId::BEAR, CATTR(
                                c.speed = 120;
                                c.size = CreatureSize::MEDIUM;
                                c.strength = 18;
                                c.dexterity = 15;
                                c.barehandedDamage = 5;
                                c.weight = 250;
                                c.humanoid = false;
                                c.animal = true;
                                c.name = "cave bear";), tribe, factory);
    case CreatureId::RAT: return get(ViewId::RAT, CATTR(
                                c.speed = 180;
                                c.size = CreatureSize::SMALL;
                                c.strength = 2;
                                c.dexterity = 12;
                                c.barehandedDamage = 2;
                                c.humanoid = false;
                                c.weight = 1;
                                c.animal = true;
                                c.skills.insert(Skill::swimming);
                                c.name = "rat";), Tribe::pest, factory);
    case CreatureId::SPIDER: { bool spider = Random.roll(2);
                             return get(spider ? ViewId::SPIDER : ViewId::SCORPION, CATTR(
                                c.speed = 100;
                                c.size = CreatureSize::SMALL;
                                c.strength = 9;
                                c.dexterity = 16;
                                c.barehandedDamage = 12;
                                c.attackEffect = EffectType::POISON;
                                c.humanoid = false;
                                c.weight = 0.3;
                                c.legs = 8;
                                c.arms = 0;
                                c.animal = true;
                                c.name = spider ? "spider" : "scorpion";), Tribe::pest, factory); }
    case CreatureId::SNAKE: return get(ViewId::SNAKE, CATTR(
                                c.speed = 100;
                                c.size = CreatureSize::SMALL;
                                c.strength = 2;
                                c.dexterity = 14;
                                c.barehandedDamage = 15;
                                c.humanoid = false;
                                c.weight = 2;
                                c.animal = true;
                                c.attackEffect = EffectType::POISON;
                                c.skills.insert(Skill::swimming);
                                c.name = "snake";), Tribe::pest, factory);
    case CreatureId::VULTURE: return get(ViewId::VULTURE, CATTR(
                                c.speed = 80;
                                c.size = CreatureSize::SMALL;
                                c.strength = 2;
                                c.dexterity = 12;
                                c.barehandedDamage = 2;
                                c.humanoid = false;
                                c.weight = 5;
                                c.arms = 0;
                                c.legs = 0;
                                c.wings = 2;
                                c.animal = true;
                                c.flyer = true;
                                c.name = "vulture";), Tribe::pest, factory);
    case CreatureId::WOLF: return get(ViewId::WOLF, CATTR(
                                c.speed = 160;
                                c.size = CreatureSize::MEDIUM;
                                c.strength = 10;
                                c.dexterity = 15;
                                c.barehandedDamage = 10;
                                c.humanoid = false;
                                c.animal = true;
                                c.weight = 35;
                                c.name = "wolf";), tribe, factory);
    case CreatureId::FIRE_SPHERE: return get(ViewId::FIRE_SPHERE, CATTR(
                                c.speed = 100;
                                c.size = CreatureSize::LARGE;
                                c.strength = 5;
                                c.dexterity = 15;
                                c.barehandedDamage = 10;
                                c.humanoid = false;
                                c.arms = 0;
                                c.legs = 0;
                                c.heads = 0;
                                c.noBody = true;
                                c.breathing = false;
                                c.fireResistant = true;
                                c.flyer = true;
                                c.weight = 10;
                                c.courage = 100;
                                c.name = "fire sphere";), tribe, ControllerFactory([=](Creature* c) {
                                      return new KamikazeController(c, actorFactory);
                                    }));
    case CreatureId::KRAKEN: return get(ViewId::KRAKEN, CATTR(
                                c.speed = 40;
                                c.size = CreatureSize::LARGE;
                                c.strength = 15;
                                c.dexterity = 15;
                                c.barehandedDamage = 10;
                                c.humanoid = false;
                                c.arms = 0;
                                c.legs = 0;
                                c.heads = 0;
                                c.skills.insert(Skill::swimming);
                                c.weight = 100;
                                c.name = "kraken";), tribe, ControllerFactory([=](Creature* c) {
                                      return new KrakenController(c);
                                    }));
    case CreatureId::NIGHTMARE: /*return PCreature(new Shapechanger(
                                   ViewObject(ViewId::NIGHTMARE, ViewLayer::CREATURE, "nightmare"),
                                   Tribe::monster,
                                CATTR(
                                c.speed = 100;
                                c.size = CreatureSize::LARGE;
                                c.strength = 5;
                                c.dexterity = 25;
                                c.barehandedDamage = 10;
                                c.humanoid = false;
                                c.arms = 0;
                                c.legs = 0;
                                c.flyer = true;
                                c.weight = 100;
                                c.name = "nightmare";), {
                                    CreatureId::CAVE_BEAR,
                                    CreatureId::WOLF,
                                    CreatureId::ZOMBIE,
                                    CreatureId::VAMPIRE,
                                    CreatureId::MUMMY,
                                    CreatureId::FIRE_SPHERE,
                                    CreatureId::SPECIAL_MONSTER,
                                    }));*/
    case CreatureId::BAT: return get(ViewId::BAT, CATTR(
                                c.speed = 150;
                                c.size = CreatureSize::SMALL;
                                c.strength = 3;
                                c.dexterity = 16;
                                c.barehandedDamage = 12;
                                c.humanoid = false;
                                c.legs = 0;
                                c.arms = 0;
                                c.wings = 2;
                                c.weight = 1;
                                c.flyer = true;
                                c.animal = true;
                                c.name = "bat";), tribe, factory);
    case CreatureId::DEATH: return get(ViewId::DEATH, CATTR(
                                c.speed = 95;
                                c.size = CreatureSize::LARGE;
                                c.strength = 100;
                                c.dexterity = 35;
                                c.barehandedDamage = 10;
                                c.chatReactionFriendly = c.chatReactionHostile = "\"IN ORDER TO HAVE A CHANGE OF FORTUNE AT THE LAST MINUTE YOU HAVE TO TAKE YOUR FORTUNE TO THE LAST MINUTE.\"";
                                c.humanoid = true;
                                c.weight = 30;
                                c.breathing = false;
                                c.name = "Death";), Tribe::killEveryone, factory);
    case CreatureId::SPECIAL_MONSTER: return getSpecial(NameGenerator::creatureNames.getNext(),
                                                        tribe, Random.roll(2), factory);
    case CreatureId::SPECIAL_MONSTER_HUMANOID: return getSpecial(NameGenerator::creatureNames.getNext(),
                                                        tribe, true, factory);
  }
  Debug(FATAL) << "unhandled case";
  return PCreature(nullptr);
}

ItemId randomHealing() {
  return chooseRandom({ItemId::HEALING_POTION, ItemId::FIRST_AID_KIT});
}

ItemId randomBackup() {
  return chooseRandom({ItemId::TELE_SCROLL, randomHealing()}, {1, 4});
}

ItemId randomArmor() {
  return chooseRandom({ItemId::LEATHER_ARMOR, ItemId::CHAIN_ARMOR}, {4, 1});
}

class ItemList {
  public:
  ItemList& maybe(double chance, ItemId id, int num = 1) {
    if (Random.getDouble() <= chance)
      add(id, num);
    return *this;
  }

  ItemList& maybe(double chance, const vector<ItemId>& ids) {
    if (Random.getDouble() <= chance)
      for (ItemId id : ids)
        add(id);
    return *this;
  }

  ItemList& add(ItemId id, int num = 1) {
    for (int i : Range(num))
      ret.push_back(id);
    return *this;
  }

  operator vector<ItemId>() {
    return ret;
  }

  private:
  vector<ItemId> ret;
};

vector<ItemId> getInventory(CreatureId id) {
  switch (id) {
    case CreatureId::DEATH: return ItemList()
            .add(ItemId::SCYTHE);
    case CreatureId::LEPRECHAUN: return ItemList()
            .add(ItemId::TELE_SCROLL, Random.getRandom(1, 4));
    case CreatureId::GNOME: return ItemList()
            .add(chooseRandom({ItemId::KNIFE}))
            .maybe(0.3, ItemId::LEATHER_BOOTS)
            .maybe(0.05, ItemList().add(ItemId::BOW).add(ItemId::ARROW, Random.getRandom(20, 36)));
    case CreatureId::ARCHER: return ItemList()
            .add(ItemId::BOW).add(ItemId::ARROW, Random.getRandom(20, 36))
            .add(ItemId::KNIFE)
            .add(ItemId::LEATHER_ARMOR)
            .add(ItemId::LEATHER_BOOTS)
            .add(randomHealing())
            .add(ItemId::GOLD_PIECE, Random.getRandom(20, 50));
    case CreatureId::CASTLE_GUARD:
    case CreatureId::KNIGHT: return ItemList()
            .add(ItemId::SWORD)
            .add(ItemId::CHAIN_ARMOR)
            .add(ItemId::LEATHER_BOOTS)
            .add(randomHealing())
            .add(ItemId::GOLD_PIECE, Random.getRandom(30, 80));
    case CreatureId::DEVIL: return ItemList()
            .add(chooseRandom({ItemId::BLINDNESS_POTION, ItemId::SLEEP_POTION, ItemId::SLOW_POTION}));
    case CreatureId::DARK_KNIGHT:
    case CreatureId::AVATAR: return ItemList()
            .add(ItemId::SPECIAL_BATTLE_AXE)
            .add(ItemId::CHAIN_ARMOR)
            .add(ItemId::IRON_HELM)
            .add(ItemId::IRON_BOOTS)
            .add(ItemId::HEALING_POTION, Random.getRandom(3, 7))
            .add(ItemId::GOLD_PIECE, Random.getRandom(200, 300));
    case CreatureId::BANDIT:
    case CreatureId::GOBLIN: return ItemList()
            .add(chooseRandom({ItemId::SWORD}, {3}))
            .add(ItemId::LEATHER_ARMOR)
            .maybe(0.3, randomBackup())
            .maybe(0.2, ItemId::GOLD_PIECE, Random.getRandom(10, 30));
    case CreatureId::GREAT_GOBLIN: return ItemList()
            .add(chooseRandom({ItemId::SPECIAL_BATTLE_AXE, ItemId::SPECIAL_WAR_HAMMER}, {1, 1}))
            .add(ItemId::IRON_HELM)
            .add(ItemId::IRON_BOOTS)
            .add(ItemId::CHAIN_ARMOR)
            .add(randomBackup())
            .add(ItemId::KNIFE, Random.getRandom(2, 5))
            .add(ItemId::GOLD_PIECE, Random.getRandom(100, 200));
    case CreatureId::DWARF: return ItemList()
            .add(chooseRandom({ItemId::BATTLE_AXE, ItemId::WAR_HAMMER}, {1, 1}))
            .maybe(0.6, randomBackup())
            .add(ItemId::CHAIN_ARMOR)
            .maybe(0.5, ItemId::IRON_HELM)
            .maybe(0.3, ItemId::IRON_BOOTS)
            .maybe(0.2, ItemId::GOLD_PIECE, Random.getRandom(10, 30));
    case CreatureId::DWARF_BARON: return ItemList()
            .add(chooseRandom({ItemId::SPECIAL_BATTLE_AXE, ItemId::SPECIAL_WAR_HAMMER}, {1, 1}))
            .add(randomBackup())
            .add(ItemId::CHAIN_ARMOR)
            .add(ItemId::IRON_BOOTS)
            .add(ItemId::IRON_HELM);
    case CreatureId::ELF_LORD: return ItemList()
            .add(ItemId::SPECIAL_ELVEN_SWORD)
            .add(ItemId::LEATHER_ARMOR)
            .add(ItemId::BOW)
            .add(ItemId::ARROW, Random.getRandom(20, 36))
            .add(randomBackup());
    case CreatureId::ELF: return ItemList()
            .add(ItemId::ELVEN_SWORD)
            .add(ItemId::LEATHER_ARMOR)
            .add(ItemId::BOW)
            .add(ItemId::ARROW, Random.getRandom(20, 36))
            .add(randomBackup());
    case CreatureId::MUMMY_LORD: return ItemList()
            .add(ItemId::GOLD_PIECE, Random.getRandom(100, 200)).add(
            chooseRandom({ItemId::SPECIAL_BATTLE_AXE, ItemId::SPECIAL_WAR_HAMMER, ItemId::SPECIAL_SWORD}, {1, 1, 1}));
    default: return {};
  }
}

PCreature CreatureFactory::fromId(CreatureId id, Tribe* t, MonsterAIFactory factory) {
  return addInventory(get(id, t, factory), getInventory(id));
}

