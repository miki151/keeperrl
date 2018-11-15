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
#include "profiler.h"
#include "portals.h"
#include "fx_name.h"
#include "roof_support.h"

template <class Archive>
void Position::serialize(Archive& ar, const unsigned int) {
  WLevel SERIAL(levelSerial);
  if (Archive::is_saving::value) {
    if (level)
      levelSerial = level;
  }
  ar(coord, levelSerial, valid);
  if (Archive::is_loading::value) {
    level = levelSerial.get();
  }
}

SERIALIZABLE(Position)
SERIALIZATION_CONSTRUCTOR_IMPL(Position);

int Position::getHash() const {
  return coord.getHash();
}

Vec2 Position::getCoord() const {
  return coord;
}

Level* Position::getLevel() const {
  return level;
}

WModel Position::getModel() const {
  PROFILE;
  if (isValid())
    return level->getModel();
  else
    return nullptr;
}

WGame Position::getGame() const {
  PROFILE;
  if (level)
    return level->getModel()->getGame();
  else
    return nullptr;
}

Position::Position(Vec2 v, WLevel l) : coord(v), level(l.get()), valid(level && level->inBounds(coord)) {
  PROFILE;
}

const static int otherLevel = 1000000;

int Position::dist8(const Position& pos) const {
  PROFILE;
  if (pos.level != level || !isValid() || !pos.isValid())
    return otherLevel;
  return pos.getCoord().dist8(coord);
}

bool Position::isSameLevel(const Position& p) const {
  PROFILE;
  return isValid() && level == p.level;
}

bool Position::isSameModel(const Position& p) const {
  PROFILE;
  return isValid() && p.isValid() && getModel() == p.getModel();
}

bool Position::isSameLevel(WConstLevel l) const {  PROFILE
  return isValid() && l == level;
}

bool Position::isValid() const {
  return valid;
}

Vec2 Position::getDir(const Position& p) const {
  PROFILE;
  CHECK(isSameLevel(p));
  return p.coord - coord;
}

optional<StairKey> Position::getLandingLink() const {
  PROFILE;
  if (isValid())
    return getSquare()->getLandingLink();
  else
    return none;
}

WSquare Position::modSquare() const {
  PROFILE;
  CHECK(isValid());
  return level->modSafeSquare(coord);
}

WConstSquare Position::getSquare() const {
  //PROFILE;
  CHECK(isValid());
  return level->getSafeSquare(coord);
}

WFurniture Position::modFurniture(FurnitureLayer layer) const {
  PROFILE;
  if (isValid())
    return level->furniture->getBuilt(layer).getWritable(coord);
  else
    return nullptr;
}

WFurniture Position::modFurniture(FurnitureType type) const {
  PROFILE;
  if (auto furniture = modFurniture(Furniture::getLayer(type)))
    if (furniture->getType() == type)
      return furniture;
  return nullptr;
}

WConstFurniture Position::getFurniture(FurnitureLayer layer) const {
  PROFILE;
  if (isValid())
    return level->furniture->getBuilt(layer).getReadonly(coord);
  else
    return nullptr;
}

WConstFurniture Position::getFurniture(FurnitureType type) const {
  PROFILE;
  if (auto furniture = getFurniture(Furniture::getLayer(type)))
    if (furniture->getType() == type)
      return furniture;
  return nullptr;
}

vector<WConstFurniture> Position::getFurniture() const {
  PROFILE;
  vector<WConstFurniture> ret;
  for (auto layer : ENUM_ALL(FurnitureLayer))
    if (auto f = getFurniture(layer))
      ret.push_back(f);
  return ret;
}

vector<WFurniture> Position::modFurniture() const {
  PROFILE;
  vector<WFurniture> ret;
  for (auto layer : ENUM_ALL(FurnitureLayer))
    if (auto f = modFurniture(layer))
      ret.push_back(f);
  return ret;
}

optional<int> Position::getDistanceToNearestPortal() const {
  if (level)
    return level->portals->getDistanceToNearest(coord);
  else
    return none;
}

optional<Position> Position::getOtherPortal() const {
  if (level)
    if (auto v = level->portals->getOtherPortal(coord))
      return Position(*v, level);
  return none;
}

void Position::registerPortal() {
  if (isValid()) {
    level->portals->registerPortal(*this);
    if (auto other = level->portals->getOtherPortal(coord))
      for (auto& sectors : level->sectors)
        sectors.second.addExtraConnection(coord, *other);
  }
}

void Position::removePortal() {
  if (isValid()) {
    if (auto other = level->portals->getOtherPortal(coord))
      for (auto& sectors : level->sectors)
        sectors.second.removeExtraConnection(coord, *other);
    level->portals->removePortal(*this);
  }
}

optional<int> Position::getPortalIndex() const {
  if (isValid())
    return level->portals->getPortalIndex(coord);
  else
    return none;
}

double Position::getLightingEfficiency() const {
  const double lightBase = 0.5;
  const double flattenVal = 0.9;
  return min(1.0, (lightBase + getLight() * (1 - lightBase)) / flattenVal);
}

WCreature Position::getCreature() const {
  //PROFILE;
  if (isValid())
    return getSquare()->getCreature();
  else
    return nullptr;
}

void Position::removeCreature() {
  PROFILE;
  CHECK(isValid());
  modSquare()->removeCreature(*this);
}

bool Position::operator == (const Position& o) const {
  //PROFILE;
  return coord == o.coord && level == o.level;
}

bool Position::operator != (const Position& o) const {
  //PROFILE;
  return !(o == *this);
}

const static int hearingRange = 30;

void Position::unseenMessage(const PlayerMessage& msg) const {
  PROFILE;
  if (isValid()) {
    for (auto player : level->getPlayers())
      if (player->canSee(*this))
        return;
    for (auto player : level->getPlayers())
      if (dist8(player->getPosition()) < hearingRange) {
        player->privateMessage(msg);
        return;
      }
  }
}

void Position::globalMessage(const PlayerMessage& msg) const {
  PROFILE;
  if (isValid())
    for (auto player : level->getPlayers())
      if (player->canSee(*this)) {
        player->privateMessage(msg);
        break;
      }
}

vector<Position> Position::neighbors8() const {
  //PROFILE;
  vector<Position> ret;
  ret.reserve(8);
  for (Vec2 v : coord.neighbors8())
    ret.push_back(Position(v, level));
  return ret;
}

vector<Position> Position::neighbors4() const {
  //PROFILE;
  vector<Position> ret;
  ret.reserve(4);
  for (Vec2 v : coord.neighbors4())
    ret.push_back(Position(v, level));
  return ret;
}

vector<Position> Position::neighbors8(RandomGen& random) const {
  //PROFILE;
  vector<Position> ret;
  ret.reserve(8);
  for (Vec2 v : coord.neighbors8(random))
    ret.push_back(Position(v, level));
  return ret;
}

vector<Position> Position::neighbors4(RandomGen& random) const {
  //PROFILE;
  vector<Position> ret;
  ret.reserve(4);
  for (Vec2 v : coord.neighbors4(random))
    ret.push_back(Position(v, level));
  return ret;
}

vector<Position> Position::getRectangle(Rectangle rect) const {
  PROFILE;
  vector<Position> ret;
  ret.reserve(rect.width() * rect.height());
  for (Vec2 v : rect.translate(coord))
    ret.emplace_back(v, level);
  return ret;
}

void Position::addCreature(PCreature c) {
  PROFILE;
  if (isValid()) {
    WCreature ref = c.get();
    getModel()->addCreature(std::move(c));
    level->putCreature(coord, ref);
  }
}

void Position::addCreature(PCreature c, TimeInterval delay) {
  PROFILE;
  if (isValid()) {
    WCreature ref = c.get();
    getModel()->addCreature(std::move(c), delay);
    level->putCreature(coord, ref);
  }
}

Position Position::plus(Vec2 v) const {
  PROFILE;
  return Position(coord + v, level);
}

Position Position::minus(Vec2 v) const {
  PROFILE;
  return Position(coord - v, level);
}

optional<FurnitureClickType> Position::getClickType() const {
  PROFILE;
  if (auto furniture = getFurniture(FurnitureLayer::MIDDLE))
    return furniture->getClickType();
  else
    return none;
}

void Position::addSound(const Sound& sound1) const {
  PROFILE;
  Sound sound(sound1);
  sound.setPosition(*this);
  getGame()->getView()->addSound(sound);
}

string Position::getName() const {
  PROFILE;
  for (auto layer : ENUM_ALL_REVERSE(FurnitureLayer))
    if (auto furniture = getFurniture(layer))
      return furniture->getName();
  return "";
}

void Position::getViewIndex(ViewIndex& index, WConstCreature viewer) const {
  PROFILE;
  if (isValid()) {
    getSquare()->getViewIndex(index, viewer);
    if (isUnavailable())
      index.setHighlight(HighlightType::UNAVAILABLE);
    if (isCovered() > 0)
      index.setHighlight(HighlightType::INDOORS);
    for (auto furniture : getFurniture())
      if (furniture->isVisibleTo(viewer) && furniture->getViewObject()) {
        auto obj = *furniture->getViewObject();
        obj.setGenericId(level->getUniqueId() + coord.x * 2000 + coord.y);
        index.insert(std::move(obj));
      }
    if (index.noObjects())
      index.insert(ViewObject(ViewId::EMPTY, ViewLayer::FLOOR_BACKGROUND));
  }
}

const vector<WItem>& Position::getItems() const {
  PROFILE;
  if (isValid())
    return getSquare()->getInventory().getItems();
  else {
    static vector<WItem> empty;
    return empty;
  }
}

const vector<WItem>& Position::getItems(ItemIndex index) const {
  PROFILE;
  if (isValid())
    return getSquare()->getInventory().getItems(index);
  else {
    static vector<WItem> empty;
    return empty;
  }
}

PItem Position::removeItem(WItem it) const {
  CHECK(isValid());
  return modSquare()->removeItem(*this, it);
}

Inventory& Position::modInventory() const {
  PROFILE;
  if (!isValid()) {
    static Inventory empty;
    return empty;
  } else
    return modSquare()->getInventory();
}

const Inventory& Position::getInventory() const {
  PROFILE;
  if (!isValid()) {
    static Inventory empty;
    return empty;
  } else
    return getSquare()->getInventory();
}

vector<PItem> Position::removeItems(vector<WItem> it) {
  PROFILE;
  CHECK(isValid());
  return modSquare()->removeItems(*this, it);
}

bool Position::isUnavailable() const {
  PROFILE;
  return !isValid() || level->isUnavailable(coord);
}

bool Position::canEnter(WConstCreature c) const {
  PROFILE;
  return !isUnavailable() && !getCreature() && canEnterEmpty(c->getMovementType());
}

bool Position::canEnter(const MovementType& t) const {
  PROFILE;
  return !isUnavailable() && !getCreature() && canEnterEmpty(t);
}

bool Position::canEnterEmpty(WConstCreature c) const {
  PROFILE;
  return canEnterEmpty(c->getMovementType());
} 

bool Position::canEnterEmpty(const MovementType& t, optional<FurnitureLayer> ignore) const {
  PROFILE;
  if (isUnavailable())
    return false;
  auto square = getSquare();
  bool result = true;
  for (auto furniture : getFurniture()) {
    if (ignore == furniture->getLayer())
      continue;
    bool canEnter =
        furniture->getMovementSet().canEnter(t, isCovered(), square->isOnFire(), square->getForbiddenTribe());
    if (furniture->overridesMovement())
      return canEnter;
    else
      result &= canEnter;
  }
  return result;
}

void Position::onEnter(WCreature c) {
  PROFILE;
  for (auto layer : ENUM_ALL_REVERSE(FurnitureLayer))
    if (auto f = getFurniture(layer)) {
      bool overrides = f->overridesMovement();
      // f can be removed in onEnter so don't use it after
      f->onEnter(c);
      if (overrides)
        break;
    }
}

void Position::dropItem(PItem item) {
  PROFILE;
  dropItems(makeVec(std::move(item)));
}

void Position::dropItems(vector<PItem> v) {
  PROFILE;
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

constexpr int buildingSupportRadius = 5;

void Position::updateBuildingSupport() const {
  if (isValid()) {
    if (isBuildingSupport())
      level->roofSupport->add(coord);
    else
      level->roofSupport->remove(coord);
  }
}

void Position::addFurniture(PFurniture f) const {
  PROFILE;
  auto furniture = f.get();
  level->setFurniture(coord, std::move(f));
  updateConnectivity();
  updateVisibility();
  updateBuildingSupport();
  level->addLightSource(coord, furniture->getLightEmission());
  setNeedsRenderUpdate(true);
}

void Position::addCreatureLight(bool darkness) {
  PROFILE;
  if (isValid()) {
    if (darkness)
      level->addDarknessSource(coord, Level::getCreatureLightRadius(), 1);
    else
      level->addLightSource(coord, Level::getCreatureLightRadius(), 1);
  }
}

void Position::removeCreatureLight(bool darkness) {
  PROFILE;
  if (isValid()) {
    if (darkness)
      level->addDarknessSource(coord, Level::getCreatureLightRadius(), -1);
    else
      level->addLightSource(coord, Level::getCreatureLightRadius(), -1);
  }
}

void Position::removeFurniture(WConstFurniture f, PFurniture replace) const {
  PROFILE;
  level->removeLightSource(coord, f->getLightEmission());
  auto replacePtr = replace.get();
  auto layer = f->getLayer();
  CHECK(layer != FurnitureLayer::GROUND || !!replace);
  CHECK(getFurniture(layer) == f);
  if (replace)
    level->setFurniture(coord, std::move(replace));
  else {
    level->furniture->getBuilt(layer).clearElem(coord);
    level->furniture->getConstruction(coord, layer).reset();
  }
  updateMovementDueToFire();
  updateConnectivity();
  updateVisibility();
  updateSupport();
  updateBuildingSupport();
  if (replacePtr)
    level->addLightSource(coord, replacePtr->getLightEmission());
  setNeedsRenderUpdate(true);
}

bool Position::canConstruct(FurnitureType type) const {
  PROFILE;
  return !isUnavailable() && FurnitureFactory::canBuild(type, *this) && FurnitureFactory::hasSupport(type, *this);
}

bool Position::isWall() const {
  PROFILE;
  if (auto furniture = getFurniture(FurnitureLayer::MIDDLE))
    return furniture->isWall();
  else
    return false;
}

bool Position::isBuildingSupport() const {
  PROFILE;
  if (auto furniture = getFurniture(FurnitureLayer::MIDDLE))
    return furniture->isBuildingSupport();
  else
    return false;
}

void Position::construct(FurnitureType type, WCreature c) {
  PROFILE;
  if (construct(type, c->getTribeId()))
    modFurniture(Furniture::getLayer(type))->onConstructedBy(c);
}

bool Position::construct(FurnitureType type, TribeId tribe) {
  PROFILE;
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
  PROFILE;
  return !isUnavailable() && !!level->furniture->getConstruction(coord, layer);
}

bool Position::isBurning() const {
  PROFILE;
  for (auto furniture : getFurniture())
    if (furniture->getFire() && furniture->getFire()->isBurning())
      return true;
  return false;
}

void Position::updateMovementDueToFire() const {
  PROFILE;
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
  PROFILE;
  for (auto furniture : modFurniture())
    furniture->fireDamage(*this, amount);
  if (WCreature creature = getCreature())
    creature->affectByFire(amount);
  for (WItem it : getItems())
    it->fireDamage(amount, *this);
}

bool Position::needsMemoryUpdate() const {
  PROFILE;
  return isValid() && level->needsMemoryUpdate(getCoord());
}

void Position::setNeedsMemoryUpdate(bool s) const {
  PROFILE;
  if (isValid())
    level->setNeedsMemoryUpdate(getCoord(), s);
}

bool Position::needsRenderUpdate() const {
  PROFILE;
  return isValid() && level->needsRenderUpdate(getCoord());
}

void Position::setNeedsRenderUpdate(bool s) const {
  PROFILE;
  if (isValid())
    level->setNeedsRenderUpdate(getCoord(), s);
}

ViewId Position::getTopViewId() const {
  PROFILE;
  for (auto layer : ENUM_ALL_REVERSE(FurnitureLayer))
    if (auto furniture = getFurniture(layer))
      if (auto& obj = furniture->getViewObject())
        return obj->id();
  return ViewId::EMPTY;
}

void Position::forbidMovementForTribe(TribeId t) {
  PROFILE;
  if (!isUnavailable())
    modSquare()->forbidMovementForTribe(*this, t);
}

void Position::allowMovementForTribe(TribeId t) {
  PROFILE;
  if (!isUnavailable())
    modSquare()->allowMovementForTribe(*this, t);
}

bool Position::isTribeForbidden(TribeId t) const {
  PROFILE;
  return isValid() && getSquare()->isTribeForbidden(t);
}

optional<TribeId> Position::getForbiddenTribe() const {
  PROFILE;
  if (isValid())
    return getSquare()->getForbiddenTribe();
  else
    return none;
}

vector<Position> Position::getVisibleTiles(const Vision& vision) {
  PROFILE;
  if (isValid())
    return getLevel()->getVisibleTiles(coord, vision).transform([this] (Vec2 v) { return Position(v, getLevel()); });
  else
    return {};
}

void Position::addPoisonGas(double amount) {
  PROFILE;
  if (isValid())
    modSquare()->addPoisonGas(*this, amount);
}

double Position::getPoisonGasAmount() const {
  PROFILE;
  if (isValid())
    return getSquare()->getPoisonGasAmount();
  else
    return 0;
}

bool Position::isCovered() const {
  PROFILE;
  if (isValid())
    return level->covered[coord] || level->roofSupport->isRoof(coord);
  else
    return false;
}

bool Position::sunlightBurns() const {
  PROFILE;
  return isValid() && level->isInSunlight(coord);
}

double Position::getLightEmission() const {
  PROFILE;
  if (!isValid())
    return 0;
  double ret = 0;
  for (auto f : getFurniture())
    ret += f->getLightEmission();
  return ret;
}

void Position::throwItem(PItem item, const Attack& attack, int maxDist, Vec2 direction, VisionId vision) {
  PROFILE;
  if (isValid())
    level->throwItem(std::move(item), attack, maxDist, coord, direction, vision);
}

void Position::throwItem(vector<PItem> item, const Attack& attack, int maxDist, Vec2 direction, VisionId vision) {
  PROFILE;
  if (isValid())
    level->throwItem(std::move(item), attack, maxDist, coord, direction, vision);
}

void Position::updateConnectivity() const {
  PROFILE;
  // It's important that sectors aren't generated at this point, because we need stale data to detect change.
  auto movementEventPredicate = [this] { return level->getSectorsDontCreate({MovementTrait::WALK}).contains(coord); };
  bool couldEnter = movementEventPredicate();
  if (isValid()) {
    for (auto& elem : level->sectors)
      if (canNavigate(elem.first))
        elem.second.add(coord);
      else
        elem.second.remove(coord);
  }
  if (couldEnter != movementEventPredicate())
    if (auto game = getGame())
      game->addEvent(EventInfo::MovementChanged{*this});
}

void Position::updateVisibility() const {
  PROFILE;
  if (isValid())
    level->updateVisibility(coord);
}

void Position::updateSupport() const {
  PROFILE;
  for (auto pos : neighbors8())
    for (auto f : pos.modFurniture())
      if (!FurnitureFactory::hasSupport(f->getType(), pos))
        f->destroy(pos, DestroyAction::Type::BASH);
}

bool Position::canNavigate(const MovementType& type) const {
  PROFILE;
  optional<FurnitureLayer> ignore;
  if (auto furniture = getFurniture(FurnitureLayer::MIDDLE))
    for (DestroyAction action : type.getDestroyActions())
      if (furniture->canDestroy(type, action))
        ignore = FurnitureLayer::MIDDLE;
  if (type.canBuildBridge() && canConstruct(FurnitureType::BRIDGE) &&
      !type.isCompatible(getFurniture(FurnitureLayer::GROUND)->getTribe()))
    return true;
  return canEnterEmpty(type, ignore);
}

bool Position::canNavigateToOrNeighbor(Position from, const MovementType& type) const {
  if (isConnectedTo(from, type))
    return true;
  for (Position v : neighbors8())
    if (v.isConnectedTo(from, type))
      return true;
  return false;
}

optional<DestroyAction> Position::getBestDestroyAction(const MovementType& movement) const {
  PROFILE;
  if (auto furniture = getFurniture(FurnitureLayer::MIDDLE))
    if (canEnterEmpty(movement, FurnitureLayer::MIDDLE)) {
      optional<double> strength;
      optional<DestroyAction> bestAction;
      for (DestroyAction action : movement.getDestroyActions()) {
        if (furniture->canDestroy(movement, action)) {
          double thisStrength = *furniture->getStrength(action);
          if (!strength || thisStrength < *strength) {
            strength = thisStrength;
            bestAction = action;
          }
        }
      }
      return bestAction;
    }
  return none;
}

optional<double> Position::getNavigationCost(const MovementType& movement) const {
  PROFILE;
  if (canEnterEmpty(movement)) {
    if (auto c = getCreature()) {
      if (c->getAttributes().isBoulder())
        return none;
      else
        return 5.0;
    } else
      return 1.0;
  }
  if (auto furniture = getFurniture(FurnitureLayer::MIDDLE))
    if (auto destroyAction = getBestDestroyAction(movement))
      return *furniture->getStrength(*destroyAction) / 10;
  if (movement.canBuildBridge() && canConstruct(FurnitureType::BRIDGE) &&
      !movement.isCompatible(getFurniture(FurnitureLayer::GROUND)->getTribe()))
    return 10;
  return none;
}

bool Position::canSeeThru(VisionId id) const {
  PROFILE;
  if (!isValid())
    return false;
  if (auto furniture = level->furniture->getBuilt(FurnitureLayer::MIDDLE).getReadonly(coord))
    return furniture->canSeeThru(id);
  return true;
}

bool Position::stopsProjectiles(VisionId id) const {
  PROFILE;
  if (auto furniture = getFurniture(FurnitureLayer::MIDDLE))
    return furniture->stopsProjectiles(id);
  else
    return !isValid();
}

bool Position::isVisibleBy(WConstCreature c) const {
  PROFILE;

  return isValid() && c->getPosition().isSameLevel(*this) &&
      level->canSee(c->getPosition().getCoord(), coord, c->getVision());
}

void Position::clearItemIndex(ItemIndex index) const {
  PROFILE;
  if (isValid())
    modSquare()->clearItemIndex(index);
}

bool Position::isChokePoint(const MovementType& movement) const {
  PROFILE;
  return isValid() && level->isChokePoint(coord, movement);
}

bool Position::isConnectedTo(Position pos, const MovementType& movement) const {
  PROFILE;
  return isValid() && pos.isValid() && level == pos.level &&
      level->areConnected(pos.coord, coord, movement);
}

vector<WCreature> Position::getAllCreatures(int range) const {
  PROFILE;
  if (isValid())
    return level->getAllCreatures(Rectangle::centered(coord, range));
  else
    return {};
}

void Position::moveCreature(Position pos, bool teleportEffect) {
  PROFILE;
  CHECK(isValid());
  if (teleportEffect)
    getGame()->addEvent(EventInfo::FX{*this, FXName::TELEPORT_OUT});
  if (isSameLevel(pos))
    level->moveCreature(getCreature(), getDir(pos));
  else if (isSameModel(pos))
    level->changeLevel(pos, getCreature());
  else pos.getLevel()->landCreature({pos}, getModel()->extractCreature(getCreature()));
  if (teleportEffect)
    getGame()->addEvent(EventInfo::FX{pos, FXName::TELEPORT_IN});
}

void Position::moveCreature(Vec2 direction) {
  PROFILE;
  CHECK(isValid());
  level->moveCreature(getCreature(), direction);
}

static bool canPass(Position position, WConstCreature c) {
  PROFILE;
  return position.canEnterEmpty(c) && (!position.getCreature() ||
      !position.getCreature()->getAttributes().isBoulder());
}

bool Position::canMoveCreature(Vec2 direction) const {
  PROFILE;
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
  PROFILE;
  return !isSameLevel(pos) || canMoveCreature(getDir(pos));
}

double Position::getLight() const {
  PROFILE;
  if (isValid())
    return level->getLight(coord);
  else
    return 0;
}

optional<Position> Position::getStairsTo(Position pos) const {
  PROFILE;
  CHECK(isValid() && pos.isValid());
  CHECK(!isSameLevel(pos));
  return level->getStairsTo(pos.level); 
}

void Position::swapCreatures(WCreature c) {
  PROFILE;
  CHECK(isValid() && getCreature());
  level->swapCreatures(getCreature(), c);
}

Position Position::withCoord(Vec2 newCoord) const {
  PROFILE;
  CHECK(isValid());
  return Position(newCoord, level);
}

void Position::putCreature(WCreature c) {
  PROFILE;
  CHECK(isValid());
  level->putCreature(coord, c);
}
