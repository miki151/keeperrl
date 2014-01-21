#include "stdafx.h"

#include "square.h"
#include "square_factory.h"
#include "level.h"

using namespace std;

Square::Square(const ViewObject& vo, const string& n, bool see, bool canHide, int s, double f,
    map<SquareType, int> construct, bool tick) 
    : name(n), viewObject(vo), seeThru(see), hide(canHide), strength(s), fire(strength, f),
    constructions(construct), ticking(tick) {
}

void Square::putCreature(Creature* c) {
  CHECK(canEnter(c));
  creature = c;
  onEnter(c);
}

void Square::setPosition(Vec2 v) {
  position = v;
}

Vec2 Square::getPosition() const {
  return position;
}

string Square::getName() const {
  return name;
}

void Square::setName(const string& s) {
  name = s;
}

void Square::setLandingLink(StairDirection direction, StairKey key) {
  landingLink = make_pair(direction, key);
}

bool Square::isLandingSquare(StairDirection direction, StairKey key) {
  return landingLink == make_pair(direction, key);
}

Optional<pair<StairDirection, StairKey>> Square::getLandingLink() const {
  return landingLink;
}

void Square::setCovered(bool c) {
  covered = c;
}

bool Square::isCovered() const {
  return covered;
}

void Square::setHeight(double h) {
  viewObject.setHeight(h);
  height = h;
}

void Square::addTravelDir(Vec2 dir) {
  if (!findElement(travelDir, dir))
    travelDir.push_back(dir);
}

bool Square::canConstruct(SquareType type) const {
  return constructions.count(type);
}

void Square::construct(SquareType type) {
  CHECK(canConstruct(type));
  if (--constructions[type] == 0) {
    PSquare newSquare = PSquare(SquareFactory::get(type));
    level->replaceSquare(position, std::move(newSquare));
  }
}

void Square::destroy(int strength) {
  CHECK(canDestroy());
  getLevel()->globalMessage(getPosition(), "The " + getName() + " is destroyed.");
  EventListener::addSquareReplacedEvent(getLevel(), getPosition());
  getLevel()->replaceSquare(getPosition(), PSquare(SquareFactory::get(SquareType::FLOOR)));
}

void Square::burnOut() {
  if (canDestroy())
    destroy(12345);
}

const vector<Vec2>& Square::getTravelDir() const {
  return travelDir;
}

void Square::putCreatureSilently(Creature* c) {
  CHECK(canEnter(c));
  creature = c;
}

void Square::setLevel(Level* l) {
  level = l;
  if (ticking || !inventory.isEmpty())
    level->addTickingSquare(position);
}

const Level* Square::getConstLevel() const {
  return level;
}

Level* Square::getLevel() {
  return level;
}

void Square::setViewObject(const ViewObject& o) {
  viewObject = o;
}

void Square::setFog(double val) {
  fog = val;
}

void Square::tick(double time) {
  if (!inventory.isEmpty())
    for (Item* item : inventory.getItems()) {
      item->tick(time, level, position);
      if (item->isDiscarded())
        inventory.removeItem(item);
    }
  poisonGas.tick(level, position);
  if (creature && poisonGas.getAmount() > 0.2) {
    creature->poisonWithGas(min(1.0, poisonGas.getAmount()));
  }
  if (fire.isBurning()) {
    viewObject.setBurning(fire.getSize());
    Debug() << getName() << " burning " << fire.getSize();
    for (Vec2 v : position.neighbors8(true))
      if (fire.getSize() > Random.getDouble() * 60)
        level->getSquare(v)->setOnFire(fire.getSize() / 20);
    fire.tick(level, position);
    if (fire.isBurntOut()) {
      level->globalMessage(position, "The " + getName() + " burns out");
      burnOut();
      return;
    }
    if (creature)
      creature->setOnFire(fire.getSize());
    for (Item* it : getItems())
      it->setOnFire(fire.getSize(), level, position);
  }
  for (PTrigger& t : triggers)
    t->tick(time);
  tickSpecial(time);
}

bool Square::itemLands(Item* item, const Attack& attack) {
  if (creature) {
    if (!creature->dodgeAttack(attack))
      return true;
    else {
      creature->you(MsgType::MISS_THROWN_ATTACK, item->getTheName());
      return false;
    }
  }
  for (PTrigger& t : triggers)
    if (t->interceptsFlyingItem(item))
      return true;
  return false;
}

bool Square::itemBounces(Item* item) const {
  return !canEnterEmpty(Creature::getDefault());
}

void Square::onItemLands(PItem item, const Attack& attack, int remainingDist, Vec2 dir) {
  if (creature) {
    item->onHitCreature(creature, attack);
    if (!item->isDiscarded())
      dropItem(std::move(item));
    return;
  }
  for (PTrigger& t : triggers)
    if (t->interceptsFlyingItem(item.get())) {
      t->onInterceptFlyingItem(std::move(item), attack, remainingDist, dir);
      return;
    }

  item->onHitSquare(position, this);
  if (!item->isDiscarded())
    dropItem(std::move(item));
}

bool Square::canEnter(const Creature* c) const {
  return creature == nullptr && canEnterSpecial(c);
}

bool Square::canEnterEmpty(const Creature* c) const {
  return canEnterSpecial(c);
}

bool Square::canEnterSpecial(const Creature* c) const {
  return c->canWalk();
}

void Square::setOnFire(double amount) {
  bool burning = fire.isBurning();
  fire.set(amount);
  if (!burning && fire.isBurning()) {
    level->addTickingSquare(position);
    level->globalMessage(position, "The " + getName() + " catches fire.");
    viewObject.setBurning(fire.getSize());
  }
  if (creature)
    creature->setOnFire(amount);
  for (Item* it : getItems())
    it->setOnFire(amount, level, position);
}

void Square::addPoisonGas(double amount) {
  if (canSeeThru()) {
    poisonGas.addAmount(amount);
    level->addTickingSquare(position);
  }
}

double Square::getPoisonGasAmount() const {
  return poisonGas.getAmount();
}

bool Square::isBurnt() const {
  return fire.isBurntOut();
}

ViewObject Square::getViewObject() const {
  return viewObject;
}

void Square::setBackground(const Square* square) {
  const ViewObject& obj = square->backgroundObject ? (*square->backgroundObject) : square->viewObject;
  backgroundObject = ViewObject(obj.id(), ViewLayer::FLOOR_BACKGROUND, "", false);
}

static ViewObject addFire(const ViewObject& obj, double fire) {
  ViewObject r(obj);
  r.setBurning(fire);
  return r;
}

ViewIndex Square::getViewIndex(const CreatureView* c) const {
  double fireSize = 0;
  for (Item* it : inventory.getItems())
    fireSize = max(fireSize, it->getFireSize());
  fireSize = max(fireSize, fire.getSize());
  ViewIndex ret;
  if (creature && (c->canSee(creature) || creature->isPlayer())) {
    ret.insert(addFire(creature->getViewObject(), fireSize));
  }
  else if (creature && contains(c->getUnknownAttacker(), creature))
    ret.insert(addFire(ViewObject::unknownMonster(), fireSize));

  if (c->canSee(position)) {
    if (backgroundObject)
      ret.insert(*backgroundObject);
    ret.insert(getViewObject());
    for (const PTrigger& t : triggers)
      if (auto obj = t->getViewObject())
        ret.insert(addFire(*obj, fireSize));
    if (Item* it = getTopItem())
      ret.insert(addFire(it->getViewObject(), fireSize));
  }
  if (c->canSee(position)) {
    if (poisonGas.getAmount() > 0)
      ret.setHighlight(HighlightType::POISON_GAS, min(1.0, poisonGas.getAmount()));
    if (fog)
      ret.setHighlight(HighlightType::FOG, fog);
  }
  return ret;
}

void Square::onEnter(Creature* c) {
  for (PTrigger& t : triggers)
    t->onCreatureEnter(c);
  onEnterSpecial(c);
}

void Square::dropItem(PItem item) {
  if (level)  // if level == null, then it's being constructed, square will be added later
    level->addTickingSquare(getPosition());
  inventory.addItem(std::move(item));
}

void Square::dropItems(vector<PItem> items) {
  for (PItem& it : items)
    dropItem(std::move(it));
}

bool Square::hasItem(Item* it) const {
  return inventory.hasItem(it);
}

Creature* Square::getCreature() {
  return creature;
}

void Square::addTrigger(PTrigger t) {
  level->addTickingSquare(position);
  triggers.push_back(std::move(t));
}

PTrigger Square::removeTrigger(Trigger* trigger) {
  PTrigger ret;
  for (PTrigger& t : triggers)
    if (t.get() == trigger) {
      ret = std::move(t);
      removeElement(triggers, t);
    }
  return ret;
}

const Creature* Square::getCreature() const {
  return creature;
}

void Square::removeCreature() {
  CHECK(creature);
  creature = 0;
}

bool SolidSquare::canEnterSpecial(const Creature*) const {
  return false;
}

bool Square::canSeeThru() const {
  return seeThru;
}

void Square::setCanSeeThru(bool f) {
  seeThru = f;
}

bool Square::canHide() const {
  return hide;
}

int Square::getStrength() const {
  return strength;
}

Item* Square::getTopItem() const {
  Item* last = nullptr;
  if (!inventory.isEmpty())
  for (Item* it : inventory.getItems()) {
    last = it;
    if (it->getViewObject().layer() == ViewLayer::LARGE_ITEM)
      return it;
  }
  return last;
}

vector<Item*> Square::getItems(function<bool (Item*)> predicate) {
  return inventory.getItems(predicate);
}

PItem Square::removeItem(Item* it) {
  return inventory.removeItem(it);
}

vector<PItem> Square::removeItems(vector<Item*> it) {
  return inventory.removeItems(it);
}

