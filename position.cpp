#include "stdafx.h"
#include "position.h"
#include "level.h"
#include "square.h"
#include "creature.h"
#include "trigger.h"
#include "item.h"
#include "view_id.h"
#include "location.h"

template <class Archive> 
void Position::serialize(Archive& ar, const unsigned int version) {
  ar & SVAR(coord) & SVAR(level);
}

SERIALIZABLE(Position);
SERIALIZATION_CONSTRUCTOR_IMPL(Position);

Vec2 Position::getCoord() const {
  return coord;
}

Level* Position::getLevel() const {
  return level;
}

Model* Position::getModel() const {
  return level->getModel();
}

Position::Position(Vec2 v, Level* l) : coord(v), level(l) {
}

const static int otherLevel = 1000000;

int Position::dist8(const Position& pos) const {
  if (pos.level != level || !isValid() || !pos.isValid())
    return otherLevel;
  return pos.getCoord().dist8(coord);
}

bool Position::isSameLevel(const Position& p) const {
  return isValid() && level == p.level;
}

bool Position::isSameLevel(const Level* l) const {
  return isValid() && level == l;
}

bool Position::isValid() const {
  return level && level->inBounds(coord);
}

Vec2 Position::getDir(const Position& p) const {
  CHECK(isSameLevel(p));
  return p.coord - coord;
}

Square* Position::getSquare() const {
  CHECK(isValid());
  return level->getSafeSquare(coord);
}

Creature* Position::getCreature() const {
  if (isValid())
    return getSquare()->getCreature();
  else
    return nullptr;
}

void Position::removeCreature() {
  CHECK(isValid());
  getSquare()->removeCreature();
}

bool Position::operator == (const Position& o) const {
  return coord == o.coord && level == o.level;
}

bool Position::operator != (const Position& o) const {
  return !(o == *this);
}

Position& Position::operator = (const Position& o) {
  coord = o.coord;
  level = o.level;
  return *this;
}

bool Position::operator < (const Position& p) const {
  if (!isValid())
    return p.isValid();
  if (!p.isValid())
    return true;
  if (level->getUniqueId() == p.level->getUniqueId())
    return coord < p.coord;
  else
    return level->getUniqueId() < p.level->getUniqueId();
}

void Position::globalMessage(const PlayerMessage& playerCanSee, const PlayerMessage& cannot) const {
  if (isValid())
    level->globalMessage(coord, playerCanSee, cannot);
}

void Position::globalMessage(const PlayerMessage& playerCanSee) const {
  if (isValid())
    level->globalMessage(coord, playerCanSee);
}

void Position::globalMessage(const Creature* c, const PlayerMessage& playerCanSee,
    const PlayerMessage& cannot) const {
  if (isValid())
    level->globalMessage(c, playerCanSee, cannot);
}

vector<Position> Position::neighbors8() const {
  vector<Position> ret;
  for (Vec2 v : coord.neighbors8())
    ret.push_back(Position(v, level));
  return ret;
}

vector<Position> Position::neighbors4() const {
  vector<Position> ret;
  for (Vec2 v : coord.neighbors4())
    ret.push_back(Position(v, level));
  return ret;
}

vector<Position> Position::neighbors8(RandomGen& random) const {
  vector<Position> ret;
  for (Vec2 v : coord.neighbors8(random))
    ret.push_back(Position(v, level));
  return ret;
}

vector<Position> Position::neighbors4(RandomGen& random) const {
  vector<Position> ret;
  for (Vec2 v : coord.neighbors4(random))
    ret.push_back(Position(v, level));
  return ret;
}

vector<Position> Position::getRectangle(Rectangle rect) const {
  vector<Position> ret;
  for (Vec2 v : rect.translate(coord))
    ret.emplace_back(v, level);
  return ret;
}

void Position::addCreature(PCreature c, double delay) {
  if (isValid())
    level->addCreature(coord, std::move(c), delay);
}

Position Position::plus(Vec2 v) const {
  return Position(coord + v, level);
}

Position Position::minus(Vec2 v) const {
  return Position(coord - v, level);
}

const Location* Position::getLocation() const {
  if (isValid())
    for (auto location : level->getAllLocations())
      if (location->contains(*this))
        return location;
  return nullptr;
} 

bool Position::canEnter(const Creature* c) const {
  return isValid() && getSquare()->canEnter(c);
}

bool Position::canEnter(const MovementType& t) const {
  return isValid() && getSquare()->canEnter(t);
}

optional<SquareApplyType> Position::getApplyType() const {
  return isValid() ? getSquare()->getApplyType() : none;
}

optional<SquareApplyType> Position::getApplyType(const Creature* c) const {
  return isValid() ? getSquare()->getApplyType() : none;
}

void Position::onApply(Creature* c) {
  if (isValid())
    getSquare()->onApply(c);
}

double Position::getApplyTime() const {
  CHECK(isValid());
  return getSquare()->getApplyTime();
}

bool Position::canHide() const {
  return isValid() && getSquare()->canHide();
}

string Position::getName() const {
  if (isValid())
    return getSquare()->getName();
  else
    return "";
}

void Position::getViewIndex(ViewIndex& index, const Tribe* tribe) const {
  if (isValid())
    getSquare()->getViewIndex(index, tribe);
}

vector<Trigger*> Position::getTriggers() const {
  if (isValid())
    return getSquare()->getTriggers();
  else
    return {};
}

PTrigger Position::removeTrigger(Trigger* trigger) {
  CHECK(isValid());
  return getSquare()->removeTrigger(trigger);
}

vector<PTrigger> Position::removeTriggers() {
  if (isValid())
    return getSquare()->removeTriggers();
  else
    return {};
}

void Position::addTrigger(PTrigger t) {
  if (isValid())
    getSquare()->addTrigger(std::move(t));
}

const vector<Item*>& Position::getItems() const {
  if (isValid())
    return getSquare()->getItems();
  else {
    static vector<Item*> empty;
    return empty;
  }
}

vector<Item*> Position::getItems(function<bool (Item*)> predicate) const {
  if (isValid())
    return getSquare()->getItems(predicate);
  else
    return {};
}

const vector<Item*>& Position::getItems(ItemIndex index) const {
  if (isValid())
    return getSquare()->getItems(index);
  else {
    static vector<Item*> empty;
    return empty;
  }
}

PItem Position::removeItem(Item* it) {
  CHECK(isValid());
  return getSquare()->removeItem(it);
}

vector<PItem> Position::removeItems(vector<Item*> it) {
  CHECK(isValid());
  return getSquare()->removeItems(it);
}

bool Position::canConstruct(const SquareType& type) const {
  return isValid() && getSquare()->canConstruct(type);
}

bool Position::canDestroy(const Creature* c) const {
  return isValid() && getSquare()->canDestroy(c);
}

bool Position::isDestroyable() const {
  return isValid() && getSquare()->isDestroyable();
}

bool Position::canEnterEmpty(const Creature* c) const {
  return isValid() && getSquare()->canEnterEmpty(c);
}

bool Position::canEnterEmpty(const MovementType& t) const {
  return isValid() && getSquare()->canEnterEmpty(t);
}

void Position::dropItem(PItem item) {
  if (isValid())
    getSquare()->dropItem(std::move(item));
}

void Position::dropItems(vector<PItem> v) {
  if (isValid())
    getSquare()->dropItems(std::move(v));
}

void Position::destroyBy(Creature* c) {
  if (isValid())
    getSquare()->destroyBy(c);
}

void Position::destroy() {
  if (isValid())
    getSquare()->destroy();
}

bool Position::construct(const SquareType& type) {
  return isValid() && getSquare()->construct(type);
}

bool Position::canLock() const {
  return isValid() && getSquare()->canLock();
}

bool Position::isLocked() const {
  return isValid() && getSquare()->isLocked();
}

void Position::lock() {
  if (isValid())
    getSquare()->lock();
}

bool Position::isBurning() const {
  return isValid() && getSquare()->isBurning();
}

void Position::setOnFire(double amount) {
  if (isValid())
    getSquare()->setOnFire(amount);
}

bool Position::needsMemoryUpdate() const {
  return isValid() && getSquare()->needsMemoryUpdate();
}

void Position::setMemoryUpdated() {
  if (isValid())
    getSquare()->setMemoryUpdated();
}

const ViewObject& Position::getViewObject() const {
  if (isValid())
    return getSquare()->getViewObject();
  else {
    static ViewObject v(ViewId::EMPTY, ViewLayer::FLOOR, "");
    return v;
  }
}

void Position::forbidMovementForTribe(const Tribe* t) {
  if (isValid())
    getSquare()->forbidMovementForTribe(t);
}

void Position::allowMovementForTribe(const Tribe* t) {
  if (isValid())
    getSquare()->allowMovementForTribe(t);
}

bool Position::isTribeForbidden(const Tribe* t) const {
  return isValid() && getSquare()->isTribeForbidden(t);
}

const Tribe* Position::getForbiddenTribe() const {
  if (isValid())
    return getSquare()->getForbiddenTribe();
  else
    return nullptr;
}

vector<Position> Position::getVisibleTiles(VisionId vision) {
  if (isValid())
    return transform2<Position>(getLevel()->getVisibleTiles(coord, vision),
        [this] (Vec2 v) { return Position(v, getLevel()); });
  else
    return {};
}

void Position::addPoisonGas(double amount) {
  if (isValid())
    getSquare()->addPoisonGas(amount);
}

double Position::getPoisonGasAmount() const {
  if (isValid())
    return getSquare()->getPoisonGasAmount();
  else
    return 0;
}

CoverInfo Position::getCoverInfo() const {
  if (isValid())
    return level->getCoverInfo(coord);
  else
    return CoverInfo {};
}

bool Position::sunlightBurns() const {
  return isValid() && getSquare()->sunlightBurns();
}

void Position::throwItem(PItem item, const Attack& attack, int maxDist, Vec2 direction, VisionId vision) {
  if (isValid())
    level->throwItem(std::move(item), attack, maxDist, coord, direction, vision);
}

void Position::throwItem(vector<PItem> item, const Attack& attack, int maxDist, Vec2 direction, VisionId vision) {
  if (isValid())
    level->throwItem(std::move(item), attack, maxDist, coord, direction, vision);
}

bool Position::canNavigate(const MovementType& t) const {
  return isValid() && getSquare()->canNavigate(t);
}

bool Position::landCreature(Creature* c) {
  return isValid() && level->landCreature({*this}, c);
}

const vector<Vec2>& Position::getTravelDir() const {
  if (isValid())
    return getSquare()->getTravelDir();
  else {
    static vector<Vec2> v;
    return v;
  }
}

int Position::getStrength() const {
  if (isValid())
    return getSquare()->getStrength();
  else
    return 1000000;
}

bool Position::canSeeThru(VisionId id) const {
  if (isValid())
    return getSquare()->canSeeThru(id);
  else
    return false;
}

bool Position::isVisibleBy(const Creature* c) {
  return isValid() && level->canSee(c, coord);
}

void Position::clearItemIndex(ItemIndex index) {
  if (isValid())
    getSquare()->clearItemIndex(index);
}

bool Position::isChokePoint(const MovementType& movement) const {
  return isValid() && level->isChokePoint(coord, movement);
}

bool Position::isConnectedTo(Position pos, const MovementType& movement) const {
  return isValid() && pos.isValid() && level == pos.level &&
      level->areConnected(pos.coord, coord, movement);
}

vector<Creature*> Position::getAllCreatures(int range) const {
  if (isValid())
    return level->getAllCreatures(Rectangle::centered(coord, range));
  else
    return {};
}

void Position::moveCreature(Position pos) {
  CHECK(isValid());
  if (isSameLevel(pos))
    level->moveCreature(getCreature(), getDir(pos));
  else
    level->changeLevel(pos, getCreature());
}

void Position::moveCreature(Vec2 direction) {
  CHECK(isValid());
  level->moveCreature(getCreature(), direction);
}

bool Position::canMoveCreature(Vec2 direction) const {
  return isValid() && level->canMoveCreature(getCreature(), direction);
}

bool Position::canMoveCreature(Position pos) const {
  return !isSameLevel(pos) || canMoveCreature(getDir(pos));
}

double Position::getLight() const {
  if (isValid())
    return level->getLight(coord);
  else
    return 0;
}

optional<Position> Position::getStairsTo(Position pos) const {
  CHECK(isValid() && pos.isValid());
  CHECK(!isSameLevel(pos));
  return level->getStairsTo(pos.level);
  
}

void Position::swapCreatures(Creature* c) {
  CHECK(isValid() && getCreature());
  level->swapCreatures(getCreature(), c);
}

Position Position::withCoord(Vec2 newCoord) const {
  CHECK(isValid());
  return Position(newCoord, level);
}

void Position::putCreature(Creature* c) {
  CHECK(isValid());
  level->putCreature(coord, c);
}
