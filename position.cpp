#include "stdafx.h"
#include "position.h"
#include "level.h"
#include "square.h"
#include "creature.h"
#include "item.h"
#include "view_id.h"
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
#include "furniture_array.h"
#include "inventory.h"

SERIALIZE_DEF(Position, coord, level)
SERIALIZATION_CONSTRUCTOR_IMPL(Position);

int Position::getHash() const {
  return combineHash(coord, level->getUniqueId());
}

Vec2 Position::getCoord() const {
  return coord;
}

WLevel Position::getLevel() const {
  return level;
}

WModel Position::getModel() const {
  if (isValid())
    return level->getModel();
  else
    return nullptr;
}

WGame Position::getGame() const {
  if (level)
    return level->getModel()->getGame();
  else
    return nullptr;
}

Position::Position(Vec2 v, WLevel l) : coord(v), level(l) {
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

bool Position::isSameLevel(WConstLevel l) const {
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

WSquare Position::modSquare() const {
  CHECK(isValid());
  return level->modSafeSquare(coord);
}

WConstSquare Position::getSquare() const {
  CHECK(isValid());
  return level->getSafeSquare(coord);
}

WFurniture Position::modFurniture(FurnitureLayer layer) const {
  if (isValid())
    return level->furniture->getBuilt(layer).getWritable(coord);
  else
    return nullptr;
}

WFurniture Position::modFurniture(FurnitureType type) const {
  if (auto furniture = modFurniture(Furniture::getLayer(type)))
    if (furniture->getType() == type)
      return furniture;
  return nullptr;
}

WConstFurniture Position::getFurniture(FurnitureLayer layer) const {
  if (isValid())
    return level->furniture->getBuilt(layer).getReadonly(coord);
  else
    return nullptr;
}

WConstFurniture Position::getFurniture(FurnitureType type) const {
  if (auto furniture = getFurniture(Furniture::getLayer(type)))
    if (furniture->getType() == type)
      return furniture;
  return nullptr;
}

vector<WConstFurniture> Position::getFurniture() const {
  vector<WConstFurniture> ret;
  for (auto layer : ENUM_ALL(FurnitureLayer))
    if (auto f = getFurniture(layer))
      ret.push_back(f);
  return ret;
}

vector<WFurniture> Position::modFurniture() const {
  vector<WFurniture> ret;
  for (auto layer : ENUM_ALL(FurnitureLayer))
    if (auto f = modFurniture(layer))
      ret.push_back(f);
  return ret;
}

WCreature Position::getCreature() const {
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

void Position::globalMessage(WConstCreature c, const PlayerMessage& playerCanSee,
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
    WCreature ref = c.get();
    getModel()->addCreature(std::move(c));
    level->putCreature(coord, ref);
  }
}

void Position::addCreature(PCreature c, double delay) {
  if (isValid()) {
    WCreature ref = c.get();
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

optional<FurnitureClickType> Position::getClickType() const {
  if (auto furniture = getFurniture(FurnitureLayer::MIDDLE))
    return furniture->getClickType();
  else
    return none;
}

void Position::addSound(const Sound& sound1) const {
  Sound sound(sound1);
  sound.setPosition(*this);
  getGame()->getView()->addSound(sound);
}

string Position::getName() const {
  for (auto layer : ENUM_ALL_REVERSE(FurnitureLayer))
    if (auto furniture = getFurniture(layer))
      return furniture->getName();
  return "";
}

void Position::getViewIndex(ViewIndex& index, WConstCreature viewer) const {
  if (isValid()) {
    getSquare()->getViewIndex(index, viewer);
    if (isUnavailable())
      index.setHighlight(HighlightType::UNAVAILABLE);
    for (auto furniture : getFurniture())
      if (furniture->isVisibleTo(viewer) && furniture->getViewObject())
        index.insert(*furniture->getViewObject());
    if (index.noObjects())
      index.insert(ViewObject(ViewId::EMPTY, ViewLayer::FLOOR_BACKGROUND));
  }
}

const vector<WItem>& Position::getItems() const {
  if (isValid())
    return getSquare()->getItems();
  else {
    static vector<WItem> empty;
    return empty;
  }
}

vector<WItem> Position::getItems(function<bool (WItem)> predicate) const {
  if (isValid())
    return getSquare()->getItems(predicate);
  else
    return {};
}

const vector<WItem>& Position::getItems(ItemIndex index) const {
  if (isValid())
    return getSquare()->getItems(index);
  else {
    static vector<WItem> empty;
    return empty;
  }
}

PItem Position::removeItem(WItem it) {
  CHECK(isValid());
  return modSquare()->removeItem(*this, it);
}

Inventory& Position::modInventory() const {
  if (!isValid()) {
    static Inventory empty;
    return empty;
  } else
    return modSquare()->getInventory();
}

const Inventory& Position::getInventory() const {
  if (!isValid()) {
    static Inventory empty;
    return empty;
  } else
    return getSquare()->getInventory();
}

vector<PItem> Position::removeItems(vector<WItem> it) {
  CHECK(isValid());
  return modSquare()->removeItems(*this, it);
}

bool Position::isUnavailable() const {
  return !isValid() || level->isUnavailable(coord);
}

bool Position::canEnter(WConstCreature c) const {
  return !isUnavailable() && !getCreature() && canEnterEmpty(c->getMovementType());
}

bool Position::canEnter(const MovementType& t) const {
  return !isUnavailable() && !getCreature() && canEnterEmpty(t);
}

bool Position::canEnterEmpty(WConstCreature c) const {
  return canEnterEmpty(c->getMovementType());
} 

bool Position::canEnterEmpty(const MovementType& t, optional<FurnitureLayer> ignore) const {
  if (isUnavailable())
    return false;
  auto square = getSquare();
  bool result = true;
  for (auto furniture : getFurniture()) {
    if (ignore == furniture->getLayer())
      continue;
    bool canEnter =
        furniture->getMovementSet().canEnter(t, level->covered[coord], square->isOnFire(), square->getForbiddenTribe());
    if (furniture->overridesMovement())
      return canEnter;
    else
      result &= canEnter;
  }
  return result;
}

void Position::onEnter(WCreature c) {
  for (auto layer : ENUM_ALL_REVERSE(FurnitureLayer))
    if (auto f = getFurniture(layer)) {
      f->onEnter(c);
      if (f->overridesMovement())
        break;
    }
}

void Position::dropItem(PItem item) {
  dropItems(makeVec(std::move(item)));
}

void Position::dropItems(vector<PItem> v) {
  if (isValid()) {
    for (auto layer : ENUM_ALL_REVERSE(FurnitureLayer))
      if (auto f = getFurniture(layer)) {
        v = f->dropItems(*this, std::move(v));
        if (v.empty())
          return;
        if (f->overridesMovement())
          break;
      }
    modSquare()->dropItems(*this, std::move(v));
  }
}

void Position::removeFurniture(WConstFurniture f) const {
  level->removeLightSource(coord, f->getLightEmission());
  auto layer = f->getLayer();
  CHECK(getFurniture(layer) == f);
  level->furniture->getBuilt(layer).clearElem(coord);
  level->furniture->getConstruction(coord, layer).reset();
  updateConnectivity();
  updateVisibility();
  updateSupport();
  setNeedsRenderUpdate(true);
}

void Position::addFurniture(PFurniture f) const {
  auto furniture = f.get();
  level->setFurniture(coord, std::move(f));
  updateConnectivity();
  updateVisibility();
  level->addLightSource(coord, furniture->getLightEmission());
  setNeedsRenderUpdate(true);
}

void Position::replaceFurniture(WConstFurniture prev, PFurniture next) const {
  level->removeLightSource(coord, prev->getLightEmission());
  auto furniture = next.get();
  auto layer = next->getLayer();
  CHECK(prev->getLayer() == layer);
  CHECK(getFurniture(layer) == prev);
  level->setFurniture(coord, std::move(next));
  updateConnectivity();
  updateVisibility();
  updateSupport();
  level->addLightSource(coord, furniture->getLightEmission());
  setNeedsRenderUpdate(true);
}

bool Position::canConstruct(FurnitureType type) const {
  return !isUnavailable() && FurnitureFactory::canBuild(type, *this) && FurnitureFactory::hasSupport(type, *this);
}

bool Position::isWall() const {
  if (auto furniture = getFurniture(FurnitureLayer::MIDDLE))
    return furniture->isWall();
  else
    return false;
}

void Position::construct(FurnitureType type, WCreature c) {
  if (construct(type, c->getTribeId()))
    modFurniture(Furniture::getLayer(type))->onConstructedBy(c);
}

bool Position::construct(FurnitureType type, TribeId tribe) {
  CHECK(!isUnavailable());
  CHECK(canConstruct(type));
  auto& construction = level->furniture->getConstruction(coord, Furniture::getLayer(type));
  if (!construction || construction->type != type)
    construction = FurnitureArray::Construction{type, 10};
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
      if (!getSquare()->isOnFire()) {
        modSquare()->setOnFire(true);
        updateConnectivity();
      }
    } else
      if (getSquare()->isOnFire()) {
        modSquare()->setOnFire(false);
        updateConnectivity();
      }
  }
}

void Position::fireDamage(double amount) {
  for (auto furniture : modFurniture())
    furniture->fireDamage(*this, amount);
  if (WCreature creature = getCreature())
    creature->fireDamage(amount);
  for (WItem it : getItems())
    it->fireDamage(amount, *this);
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

const ViewObject& Position::getViewObject() const {
  for (auto layer : ENUM_ALL_REVERSE(FurnitureLayer))
    if (auto furniture = getFurniture(layer))
      if (auto& obj = furniture->getViewObject())
        return *obj;
  return ViewObject::empty();
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
    return getLevel()->getVisibleTiles(coord, vision).transform([this] (Vec2 v) { return Position(v, getLevel()); });
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

void Position::updateSupport() const {
  for (auto pos : neighbors8())
    for (auto f : pos.modFurniture())
      if (!FurnitureFactory::hasSupport(f->getType(), pos))
        f->destroy(pos, DestroyAction::Type::BASH);
}

bool Position::canNavigate(const MovementType& type) const {
  optional<FurnitureLayer> ignore;
  if (auto furniture = getFurniture(FurnitureLayer::MIDDLE))
    if (furniture->canDestroy(type, DestroyAction::Type::BASH))
      ignore = FurnitureLayer::MIDDLE;
  return canEnterEmpty(type, ignore);
}

bool Position::canSeeThru(VisionId id) const {
  if (auto furniture = getFurniture(FurnitureLayer::MIDDLE))
    return furniture->canSeeThru(id);
  else
    return true;
}

bool Position::isVisibleBy(WConstCreature c) {
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

vector<WCreature> Position::getAllCreatures(int range) const {
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

static bool canPass(Position position, WConstCreature c) {
  return position.canEnterEmpty(c) && (!position.getCreature() ||
      !position.getCreature()->getAttributes().isBoulder());
}

bool Position::canMoveCreature(Vec2 direction) const {
  if (!isUnavailable()) {
    WCreature creature = getCreature();
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

void Position::swapCreatures(WCreature c) {
  CHECK(isValid() && getCreature());
  level->swapCreatures(getCreature(), c);
}

Position Position::withCoord(Vec2 newCoord) const {
  CHECK(isValid());
  return Position(newCoord, level);
}

void Position::putCreature(WCreature c) {
  CHECK(isValid());
  level->putCreature(coord, c);
}

