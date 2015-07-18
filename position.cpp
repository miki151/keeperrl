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
  if (pos.level != level)
    return otherLevel;
  return pos.getCoord().dist8(coord);
}

bool Position::isSameLevel(const Position& p) const {
  return level == p.level;
}

bool Position::isInBounds() const {
  return level->inBounds(coord);
}

Vec2 Position::getDir(const Position& p) const {
  CHECK(isSameLevel(p));
  return p.coord - coord;
}

Square* Position::getSquare() const {
  CHECK(isInBounds());
  return level->getSafeSquare(coord);
}

Creature* Position::getCreature() const {
  if (isInBounds())
    return getSquare()->getCreature();
  else
    return nullptr;
}

void Position::removeCreature() {
  CHECK(isInBounds());
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
  if (level->getUniqueId() == p.level->getUniqueId())
    return coord < p.coord;
  else
    return level->getUniqueId() < p.level->getUniqueId();
}

void Position::globalMessage(const PlayerMessage& playerCanSee, const PlayerMessage& cannot) const {
  level->globalMessage(coord, playerCanSee, cannot);
}

void Position::globalMessage(const PlayerMessage& playerCanSee) const {
  level->globalMessage(coord, playerCanSee);
}

vector<Position> Position::neighbors8(bool shuffle) const {
  vector<Position> ret;
  for (Vec2 v : coord.neighbors8(shuffle))
    ret.push_back(Position(v, level));
  return ret;
}

vector<Position> Position::neighbors4(bool shuffle) const {
  vector<Position> ret;
  for (Vec2 v : coord.neighbors4(shuffle))
    ret.push_back(Position(v, level));
  return ret;
}

vector<Position> Position::getRectangle(Rectangle rect) const {
  vector<Position> ret;
  for (Vec2 v : rect.translate(coord))
    ret.emplace_back(v, level);
  return ret;
}

void Position::addCreature(PCreature c) {
  if (isInBounds())
    level->addCreature(coord, std::move(c));
}

Position Position::plus(Vec2 v) const {
  return Position(coord + v, level);
}

Position Position::minus(Vec2 v) const {
  return Position(coord - v, level);
}

const Location* Position::getLocation() const {
  for (auto location : level->getAllLocations())
    if (location->contains(*this))
      return location;
  return nullptr;
} 

bool Position::canEnter(const Creature* c) const {
  return isInBounds() && getSquare()->canEnter(c);
}

bool Position::canEnter(const MovementType& t) const {
  return isInBounds() && getSquare()->canEnter(t);
}

optional<SquareApplyType> Position::getApplyType() const {
  return isInBounds() ? getSquare()->getApplyType() : none;
}

optional<SquareApplyType> Position::getApplyType(const Creature* c) const {
  return isInBounds() ? getSquare()->getApplyType() : none;
}

void Position::onApply(Creature* c) {
  if (isInBounds())
    getSquare()->onApply(c);
}

double Position::getApplyTime() const {
  CHECK(isInBounds());
  return getSquare()->getApplyTime();
}

bool Position::canHide() const {
  return isInBounds() && getSquare()->canHide();
}

string Position::getName() const {
  if (isInBounds())
    return getSquare()->getName();
  else
    return "";
}

void Position::getViewIndex(ViewIndex& index, const Tribe* tribe) const {
  if (isInBounds())
    getSquare()->getViewIndex(index, tribe);
}

vector<Trigger*> Position::getTriggers() const {
  if (isInBounds())
    return getSquare()->getTriggers();
  else
    return {};
}

PTrigger Position::removeTrigger(Trigger* trigger) {
  CHECK(isInBounds());
  return getSquare()->removeTrigger(trigger);
}

vector<PTrigger> Position::removeTriggers() {
  if (isInBounds())
    return getSquare()->removeTriggers();
  else
    return {};
}

void Position::addTrigger(PTrigger t) {
  if (isInBounds())
    getSquare()->addTrigger(std::move(t));
}

const vector<Item*>& Position::getItems() const {
  if (isInBounds())
    return getSquare()->getItems();
  else {
    static vector<Item*> empty;
    return empty;
  }
}

vector<Item*> Position::getItems(function<bool (Item*)> predicate) const {
  if (isInBounds())
    return getSquare()->getItems(predicate);
  else
    return {};
}

const vector<Item*>& Position::getItems(ItemIndex index) const {
  if (isInBounds())
    return getSquare()->getItems(index);
  else {
    static vector<Item*> empty;
    return empty;
  }
}

PItem Position::removeItem(Item* it) {
  CHECK(isInBounds());
  return getSquare()->removeItem(it);
}

vector<PItem> Position::removeItems(vector<Item*> it) {
  CHECK(isInBounds());
  return getSquare()->removeItems(it);
}

bool Position::canConstruct(const SquareType& type) const {
  return isInBounds() && getSquare()->canConstruct(type);
}

bool Position::canDestroy(const Creature* c) const {
  return isInBounds() && getSquare()->canDestroy(c);
}

bool Position::isDestroyable() const {
  return isInBounds() && getSquare()->isDestroyable();
}

bool Position::canEnterEmpty(const Creature* c) const {
  return isInBounds() && getSquare()->canEnterEmpty(c);
}

bool Position::canEnterEmpty(const MovementType& t) const {
  return isInBounds() && getSquare()->canEnterEmpty(t);
}

void Position::dropItem(PItem item) {
  if (isInBounds())
    getSquare()->dropItem(std::move(item));
}

void Position::dropItems(vector<PItem> v) {
  if (isInBounds())
    getSquare()->dropItems(std::move(v));
}

void Position::destroyBy(Creature* c) {
  if (isInBounds())
    getSquare()->destroyBy(c);
}

void Position::destroy() {
  if (isInBounds())
    getSquare()->destroy();
}

bool Position::construct(const SquareType& type) {
  return isInBounds() && getSquare()->construct(type);
}

bool Position::canLock() const {
  return isInBounds() && getSquare()->canLock();
}

bool Position::isLocked() const {
  return isInBounds() && getSquare()->isLocked();
}

void Position::lock() {
  if (isInBounds())
    getSquare()->lock();
}

bool Position::isBurning() const {
  return isInBounds() && getSquare()->isBurning();
}

void Position::setOnFire(double amount) {
  if (isInBounds())
    getSquare()->setOnFire(amount);
}

bool Position::needsMemoryUpdate() const {
  return isInBounds() && getSquare()->needsMemoryUpdate();
}

void Position::setMemoryUpdated() {
  if (isInBounds())
    getSquare()->setMemoryUpdated();
}

const ViewObject& Position::getViewObject() const {
  if (isInBounds())
    return getSquare()->getViewObject();
  else {
    static ViewObject v(ViewId::EMPTY, ViewLayer::FLOOR, "");
    return v;
  }
}

void Position::forbidMovementForTribe(const Tribe* t) {
  if (isInBounds())
    getSquare()->forbidMovementForTribe(t);
}

void Position::allowMovementForTribe(const Tribe* t) {
  if (isInBounds())
    getSquare()->allowMovementForTribe(t);
}

bool Position::isTribeForbidden(const Tribe* t) const {
  return isInBounds() && getSquare()->isTribeForbidden(t);
}

const Tribe* Position::getForbiddenTribe() const {
  if (isInBounds())
    return getSquare()->getForbiddenTribe();
  else
    return nullptr;
}

vector<Position> Position::getVisibleTiles(VisionId vision) {
  if (isInBounds())
    return transform2<Position>(getLevel()->getVisibleTiles(coord, vision),
        [this] (Vec2 v) { return Position(v, getLevel()); });
  else
    return {};
}

void Position::onKilled(Creature* victim, Creature* attacker) {
  if (isInBounds())
    getSquare()->onKilled(victim, attacker);
}

void Position::addPoisonGas(double amount) {
  if (isInBounds())
    getSquare()->addPoisonGas(amount);
}

double Position::getPoisonGasAmount() const {
  if (isInBounds())
    return getSquare()->getPoisonGasAmount();
  else
    return 0;
}

CoverInfo Position::getCoverInfo() const {
  if (isInBounds())
    return level->getCoverInfo(coord);
  else
    return CoverInfo {};
}

bool Position::sunlightBurns() const {
  return isInBounds() && getSquare()->sunlightBurns();
}

void Position::throwItem(PItem item, const Attack& attack, int maxDist, Vec2 direction, VisionId vision) {
  if (isInBounds())
    level->throwItem(std::move(item), attack, maxDist, coord, direction, vision);
}

void Position::throwItem(vector<PItem> item, const Attack& attack, int maxDist, Vec2 direction, VisionId vision) {
  if (isInBounds())
    level->throwItem(std::move(item), attack, maxDist, coord, direction, vision);
}

bool Position::canNavigate(const MovementType& t) const {
  return isInBounds() && getSquare()->canNavigate(t);
}

bool Position::landCreature(Creature* c) {
  return isInBounds() && level->landCreature({*this}, c);
}

const vector<Vec2>& Position::getTravelDir() const {
  if (isInBounds())
    return getSquare()->getTravelDir();
  else {
    static vector<Vec2> v;
    return v;
  }
}

int Position::getStrength() const {
  if (isInBounds())
    return getSquare()->getStrength();
  else
    return 1000000;
}

bool Position::canSeeThru(VisionId id) const {
  if (isInBounds())
    return getSquare()->canSeeThru(id);
  else
    return false;
}

