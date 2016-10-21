#include "stdafx.h"
#include "position.h"
#include "level.h"
#include "square.h"
#include "creature.h"
#include "trigger.h"
#include "item.h"
#include "view_id.h"
#include "location.h"
#include "model.h"
#include "view_index.h"
#include "sound.h"
#include "game.h"
#include "view.h"
#include "view_object.h"
#include "furniture.h"
#include "furniture_factory.h"
#include "creature_name.h"
#include "player_message.h"
#include "creature_attributes.h"
#include "fire.h"
#include "movement_set.h"
#include "square_type.h"
#include "furniture_array.h"

template <class Archive> 
void Position::serialize(Archive& ar, const unsigned int version) {
  ar & SVAR(coord) & SVAR(level);
}

SERIALIZABLE(Position);
SERIALIZATION_CONSTRUCTOR_IMPL(Position);

int Position::getHash() const {
  return combineHash(coord, level->getUniqueId());
}

Vec2 Position::getCoord() const {
  return coord;
}

Level* Position::getLevel() const {
  return level;
}

Model* Position::getModel() const {
  if (isValid())
    return level->getModel();
  else
    return nullptr;
}

Game* Position::getGame() const {
  if (isValid())
    return level->getModel()->getGame();
  else
    return nullptr;
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

bool Position::isSameModel(const Position& p) const {
  return isValid() && p.isValid() && getModel() == p.getModel();
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

optional<StairKey> Position::getLandingLink() const {
  if (isValid())
    return getSquare()->getLandingLink();
  else
    return none;
}

Square* Position::modSquare() const {
  CHECK(isValid());
  return level->modSafeSquare(coord);
}

const Square* Position::getSquare() const {
  CHECK(isValid());
  return level->getSafeSquare(coord);
}

Furniture* Position::modFurniture(FurnitureLayer layer) const {
  if (isValid())
    return level->furniture->getBuilt(layer).getWritable(coord);
  else
    return nullptr;
}

Furniture* Position::modFurniture(FurnitureType type) const {
  if (auto furniture = modFurniture(Furniture::getLayer(type)))
    if (furniture->getType() == type)
      return furniture;
  return nullptr;
}

const Furniture* Position::getFurniture(FurnitureLayer layer) const {
  if (isValid())
    return level->furniture->getBuilt(layer).getReadonly(coord);
  else
    return nullptr;
}

const Furniture* Position::getFurniture(FurnitureType type) const {
  if (auto furniture = getFurniture(Furniture::getLayer(type)))
    if (furniture->getType() == type)
      return furniture;
  return nullptr;
}

vector<const Furniture*> Position::getFurniture() const {
  vector<const Furniture*> ret;
  for (auto layer : ENUM_ALL(FurnitureLayer))
    if (auto f = getFurniture(layer))
      ret.push_back(f);
  return ret;
}

vector<Furniture*> Position::modFurniture() const {
  vector<Furniture*> ret;
  for (auto layer : ENUM_ALL(FurnitureLayer))
    if (auto f = modFurniture(layer))
      ret.push_back(f);
  return ret;
}

Creature* Position::getCreature() const {
  if (isValid())
    return getSquare()->getCreature();
  else
    return nullptr;
}

void Position::removeCreature() {
  CHECK(isValid());
  modSquare()->removeCreature(*this);
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
  if (!level)
    return !!p.level;
  if (!p.level)
    return false;
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

void Position::addCreature(PCreature c) {
  if (isValid()) {
    Creature* ref = c.get();
    getModel()->addCreature(std::move(c));
    level->putCreature(coord, ref);
  }
}

void Position::addCreature(PCreature c, double delay) {
  if (isValid()) {
    Creature* ref = c.get();
    getModel()->addCreature(std::move(c), delay);
    level->putCreature(coord, ref);
  }
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

optional<FurnitureUsageType> Position::getUsageType() const {
  if (auto furniture = getFurniture(FurnitureLayer::MIDDLE))
    return furniture->getUsageType();
  else
    return none;
}

optional<FurnitureClickType> Position::getClickType() const {
  if (auto furniture = getFurniture(FurnitureLayer::MIDDLE))
    return furniture->getClickType();
  else
    return none;
}

void Position::apply(Creature* c) const {
  if (auto f = getFurniture(FurnitureLayer::MIDDLE))
    f->use(*this, c);
}

double Position::getApplyTime() const {
  if (auto f = getFurniture(FurnitureLayer::MIDDLE))
    return f->getUsageTime();
  else
    return 1;
}

void Position::addSound(const Sound& sound1) const {
  Sound sound(sound1);
  sound.setPosition(*this);
  getGame()->getView()->addSound(sound);
}

bool Position::canHide() const {
  return isValid() && getSquare()->canHide();
}

string Position::getName() const {
  if (auto furniture = getFurniture(FurnitureLayer::MIDDLE))
    return furniture->getName();
  else if (isValid())
    return getSquare()->getName();
  else
    return "";
}

void Position::getViewIndex(ViewIndex& index, const Creature* viewer) const {
  if (isValid()) {
    getSquare()->getViewIndex(index, viewer);
    if (!index.hasObject(ViewLayer::FLOOR_BACKGROUND))
      if (auto& obj = level->getBackgroundObject(coord))
        index.insert(*obj);
    if (isValid() && isUnavailable())
      index.setHighlight(HighlightType::UNAVAILABLE);
    for (auto furniture : getFurniture())
      index.insert(furniture->getViewObject());
  }
}

vector<Trigger*> Position::getTriggers() const {
  if (isValid())
    return getSquare()->getTriggers();
  else
    return {};
}

PTrigger Position::removeTrigger(Trigger* trigger) {
  CHECK(isValid());
  return modSquare()->removeTrigger(*this, trigger);
}

vector<PTrigger> Position::removeTriggers() {
  if (isValid())
    return modSquare()->removeTriggers(*this);
  else
    return {};
}

void Position::addTrigger(PTrigger t) {
  if (isValid())
    modSquare()->addTrigger(*this, std::move(t));
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
  return modSquare()->removeItem(*this, it);
}

vector<PItem> Position::removeItems(vector<Item*> it) {
  CHECK(isValid());
  return modSquare()->removeItems(*this, it);
}

bool Position::isUnavailable() const {
  return !isValid() || level->isUnavailable(coord);
}

bool Position::canEnter(const Creature* c) const {
  return !isUnavailable() && !getCreature() && canEnterEmpty(c->getMovementType());
}

bool Position::canEnter(const MovementType& t) const {
  return !isUnavailable() && !getCreature() && canEnterEmpty(t);
}

bool Position::canEnterEmpty(const Creature* c) const {
  return canEnterEmpty(c->getMovementType());
} 

bool Position::canEnterEmpty(const MovementType& t) const {
  auto furniture = getFurniture(FurnitureLayer::MIDDLE);
  return !isUnavailable() && (!furniture || furniture->canEnter(t)) &&
      (canEnterSquare(t) || (furniture && furniture->overridesMovement() && furniture->canEnter(t)));
}

bool Position::canEnterSquare(const MovementType& t) const {
  return getSquare()->getMovementSet().canEnter(t, level->covered[coord], getSquare()->getForbiddenTribe());
}

void Position::onEnter(Creature* c) {
  for (auto f : getFurniture())
    f->onEnter(c);
}

void Position::dropItem(PItem item) {
  if (isValid())
    modSquare()->dropItem(*this, std::move(item));
}

void Position::dropItems(vector<PItem> v) {
  if (isValid())
    modSquare()->dropItems(*this, std::move(v));
}

void Position::removeFurniture(const Furniture* f) const {
  auto layer = f->getLayer();
  CHECK(getFurniture(layer) == f);
  level->furniture->getBuilt(layer).clearElem(coord);
  level->furniture->getConstruction(coord, layer).reset();
  updateConnectivity();
  updateVisibility();
  setNeedsRenderUpdate(true);
}

void Position::addFurniture(PFurniture f) const {
  level->setFurniture(coord, std::move(f));
  updateConnectivity();
  updateVisibility();
  setNeedsRenderUpdate(true);
}

void Position::replaceFurniture(const Furniture* prev, PFurniture next) const {
  auto layer = next->getLayer();
  CHECK(prev->getLayer() == layer);
  CHECK(getFurniture(layer) == prev);
  level->setFurniture(coord, std::move(next));
  updateConnectivity();
  updateVisibility();
  setNeedsRenderUpdate(true);
}

bool Position::canConstruct(FurnitureType type) const {
  return !isUnavailable() && FurnitureFactory::canBuild(type, *this);
}

bool Position::isWall() const {
  if (auto furniture = getFurniture(FurnitureLayer::MIDDLE))
    return furniture->isWall();
  else
    return false;
}

void Position::construct(FurnitureType type, Creature* c) {
  if (construct(type, c->getTribeId()))
    getFurniture(Furniture::getLayer(type))->onConstructedBy(c);
}

bool Position::construct(FurnitureType type, TribeId tribe) {
  CHECK(!isUnavailable());
  CHECK(canConstruct(type));
  auto& construction = level->furniture->getConstruction(coord, Furniture::getLayer(type));
  if (!construction || construction->type != type)
    construction = {type, 10};
  if (--construction->time == 0) {
    addFurniture(FurnitureFactory::get(type, tribe));
    return true;
  } else
    return false;
}

bool Position::isActiveConstruction(FurnitureLayer layer) const {
  return !isUnavailable() && !!level->furniture->getConstruction(coord, layer);
}

bool Position::isBurning() const {
  for (auto furniture : getFurniture())
    if (furniture->getFire() && furniture->getFire()->isBurning())
      return true;
  return false;
}

void Position::updateMovement() {
  if (isValid()) {
    if (isBurning()) {
      if (!getSquare()->getMovementSet().isOnFire()) {
        modSquare()->getMovementSet().setOnFire(true);
        updateConnectivity();
      }
    } else
      if (getSquare()->getMovementSet().isOnFire()) {
        modSquare()->getMovementSet().setOnFire(false);
        updateConnectivity();
      }
  }
}

void Position::fireDamage(double amount) {
  for (auto furniture : modFurniture())
    furniture->fireDamage(*this, amount);
  if (Creature* creature = getCreature())
    creature->fireDamage(amount);
  for (Item* it : getItems())
    it->fireDamage(amount, *this);
  for (Trigger* t : getTriggers())
    t->fireDamage(amount);
}

bool Position::needsMemoryUpdate() const {
  return isValid() && level->needsMemoryUpdate(getCoord());
}

void Position::setNeedsMemoryUpdate(bool s) const {
  if (isValid())
    level->setNeedsMemoryUpdate(getCoord(), s);
}

bool Position::needsRenderUpdate() const {
  return isValid() && level->needsRenderUpdate(getCoord());
}

void Position::setNeedsRenderUpdate(bool s) const {
  if (isValid())
    level->setNeedsRenderUpdate(getCoord(), s);
}

ViewObject& Position::modViewObject() {
  CHECK(isValid());
  modSquare()->setDirty(*this);
  return modSquare()->modViewObject();
}

const ViewObject& Position::getViewObject() const {
  if (auto furniture = getFurniture(FurnitureLayer::MIDDLE))
    return furniture->getViewObject();
  if (isValid())
    return getSquare()->getViewObject();
  else {
    static ViewObject v(ViewId::EMPTY, ViewLayer::FLOOR, "");
    return v;
  }
}

void Position::forbidMovementForTribe(TribeId t) {
  if (!isUnavailable())
    modSquare()->forbidMovementForTribe(*this, t);
}

void Position::allowMovementForTribe(TribeId t) {
  if (!isUnavailable())
    modSquare()->allowMovementForTribe(*this, t);
}

bool Position::isTribeForbidden(TribeId t) const {
  return isValid() && getSquare()->isTribeForbidden(t);
}

optional<TribeId> Position::getForbiddenTribe() const {
  if (isValid())
    return getSquare()->getForbiddenTribe();
  else
    return none;
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
    modSquare()->addPoisonGas(*this, amount);
}

double Position::getPoisonGasAmount() const {
  if (isValid())
    return getSquare()->getPoisonGasAmount();
  else
    return 0;
}

bool Position::isCovered() const {
  if (isValid())
    return level->covered[coord];
  else
    return false;
}

bool Position::sunlightBurns() const {
  return isValid() && level->isInSunlight(coord);
}

double Position::getLightEmission() const {
  if (!isValid())
    return 0;
  double ret = 0;
  for (auto t : getSquare()->getTriggers())
    ret += t->getLightEmission();
  for (auto f : getFurniture())
    ret += f->getLightEmission();
  return ret;
}

void Position::throwItem(PItem item, const Attack& attack, int maxDist, Vec2 direction, VisionId vision) {
  if (isValid())
    level->throwItem(std::move(item), attack, maxDist, coord, direction, vision);
}

void Position::throwItem(vector<PItem> item, const Attack& attack, int maxDist, Vec2 direction, VisionId vision) {
  if (isValid())
    level->throwItem(std::move(item), attack, maxDist, coord, direction, vision);
}

void Position::updateConnectivity() const {
  if (isValid()) {
    for (auto& elem : level->sectors)
      if (canNavigate(elem.first))
        elem.second.add(coord);
      else
        elem.second.remove(coord);
  }
}

void Position::updateVisibility() const {
  if (isValid())
    level->updateVisibility(coord);
}

bool Position::canNavigate(const MovementType& type) const {
  MovementType typeForced(type);
  typeForced.setForced();
  Creature* creature = getCreature();
  auto furniture = getFurniture(FurnitureLayer::MIDDLE);
  return canEnterEmpty(type) || 
    // for destroying doors, etc, but not entering forbidden zone
    (furniture && furniture->canDestroy(type, DestroyAction::Type::BASH) && !canEnterEmpty(typeForced));
}

bool Position::canSeeThru(VisionId id) const {
  auto furniture = getFurniture(FurnitureLayer::MIDDLE);
  if (isValid())
    return getSquare()->canSeeThru(id) && (!furniture || furniture->canSeeThru(id));
  else
    return false;
}

bool Position::isVisibleBy(const Creature* c) {
  return isValid() && level->canSee(c, coord);
}

void Position::clearItemIndex(ItemIndex index) {
  if (isValid())
    modSquare()->clearItemIndex(index);
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
  else if (isSameModel(pos))
    level->changeLevel(pos, getCreature());
  else pos.getLevel()->landCreature({pos}, getModel()->extractCreature(getCreature()));
}

void Position::moveCreature(Vec2 direction) {
  CHECK(isValid());
  level->moveCreature(getCreature(), direction);
}

static bool canPass(Position position, const Creature* c) {
  return position.canEnterEmpty(c) && (!position.getCreature() ||
      !position.getCreature()->getAttributes().isBoulder());
}

bool Position::canMoveCreature(Vec2 direction) const {
  if (!isUnavailable()) {
    Creature* creature = getCreature();
    Position destination = plus(direction);
    if (level->noDiagonalPassing && direction.isCardinal8() && !direction.isCardinal4() &&
        !canPass(plus(Vec2(direction.x, 0)), creature) &&
        !canPass(plus(Vec2(0, direction.y)), creature))
      return false;
    return destination.canEnter(getCreature());
  } else
    return false;
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

