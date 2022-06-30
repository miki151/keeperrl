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
#include "draw_line.h"
#include "game_event.h"
#include "content_factory.h"
#include "shortest_path.h"
#include "bucket_map.h"
#include "vision.h"
#include "tile_gas.h"
#include "tile_gas_info.h"
#include "attack.h"
#include "attack_level.h"

template <class Archive>
void Position::serialize(Archive& ar, const unsigned int) {
  ar(coord, level, valid);
}

SERIALIZABLE(Position)
SERIALIZATION_CONSTRUCTOR_IMPL(Position);

int Position::getHash() const {
  return coord.getHash() + (level ? level->levelId : 0);
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

Position::Position(Vec2 v, Level* l) : coord(v), level(l), valid(level && level->inBounds(coord)) {
}

Position::Position(Vec2 v, Level* l, IsValid) : coord(v), level(l), valid(true) {
}

optional<int> Position::dist8(const Position& pos) const {
  PROFILE;
  if (pos.level != level || !isValid() || !pos.isValid())
    return none;
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

bool Position::isSameLevel(const Level* l) const {  PROFILE
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

void Position::removeLandingLink() const {
  if (auto link = getSquare()->getLandingLink()) {
    auto& positions = level->landingSquares.at(*link);
    positions.removeElement(*this);
    if (positions.empty())
      level->landingSquares.erase(*link);
    modSquare()->setLandingLink(none);
  }
}

void Position::setLandingLink(StairKey key) const {
  if (isValid()) {
    removeLandingLink();
    level->landingSquares[key].push_back(*this);
    modSquare()->setLandingLink(key);
  }
}

Square* Position::modSquare() const {
  PROFILE;
  CHECK(isValid());
  return level->modSafeSquare(coord);
}

const Square* Position::getSquare() const {
  //PROFILE;
  CHECK(isValid());
  return level->getSafeSquare(coord);
}

Furniture* Position::modFurniture(FurnitureLayer layer) const {
  PROFILE;
  if (isValid())
    return level->furniture->getBuilt(layer).getWritable(coord);
  else
    return nullptr;
}

Furniture* Position::modFurniture(FurnitureType type) const {
  PROFILE;
  if (auto furniture = modFurniture(getGame()->getContentFactory()->furniture.getData(type).getLayer()))
    if (furniture->getType() == type)
      return furniture;
  return nullptr;
}

const Furniture* Position::getFurniture(FurnitureLayer layer) const {
  PROFILE;
  if (isValid())
    return level->furniture->getBuilt(layer).getReadonly(coord);
  else
    return nullptr;
}

const Furniture* Position::getFurniture(FurnitureType type) const {
  PROFILE;
  if (auto furniture = getFurniture(getGame()->getContentFactory()->furniture.getData(type).getLayer()))
    if (furniture->getType() == type)
      return furniture;
  return nullptr;
}

vector<const Furniture*> Position::getFurniture() const {
  PROFILE;
  vector<const Furniture*> ret;
  for (auto layer : ENUM_ALL(FurnitureLayer))
    if (auto f = getFurniture(layer))
      ret.push_back(f);
  return ret;
}

vector<Furniture*> Position::modFurniture() const {
  PROFILE;
  vector<Furniture*> ret;
  for (auto layer : ENUM_ALL(FurnitureLayer))
    if (auto f = modFurniture(layer))
      ret.push_back(f);
  return ret;
}

optional<short> Position::getDistanceToNearestPortal() const {
  return getModel()->portals->getDistanceToNearest(*this);
}

optional<Position> Position::getOtherPortal() const {
  if (isValid())
    return getModel()->portals->getOtherPortal(*this);
  return none;
}

void Position::registerPortal() {
  if (isValid()) {
    auto& portals = getModel()->portals;
    portals->registerPortal(*this);
    if (auto other = portals->getOtherPortal(*this)) {
      if (isSameLevel(*other)) {
        for (auto& sectors : level->sectors)
          sectors.second.addExtraConnection(coord, other->coord);
      } else {
        auto key = StairKey::getNew();
        setLandingLink(key);
        other->setLandingLink(key);
        getModel()->calculateStairNavigation();
      }
    }
  }
}

void Position::removePortal() {
  if (isValid()) {
    auto& portals = getModel()->portals;
    if (auto other = portals->getOtherPortal(*this)) {
      if (isSameLevel(*other)) {
        for (auto& sectors : level->sectors)
          sectors.second.removeExtraConnection(coord, other->coord);
      } else {
        removeLandingLink();
        other->removeLandingLink();
      }
    }
    portals->removePortal(*this);
  }
}

optional<int> Position::getPortalIndex() const {
  if (isValid())
    return getModel()->portals->getPortalIndex(*this);
  else
    return none;
}

double Position::getLightingEfficiency() const {
  const double lightBase = 0.5;
  const double flattenVal = 0.9;
  return min(1.0, (lightBase + getLight() * (1 - lightBase)) / flattenVal);
}

bool Position::isDirEffectBlocked(VisionId id) const {
  return !canSeeThru(id, getGame()->getContentFactory());
}

Creature* Position::getCreature() const {
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
      if (dist8(player->getPosition()).value_or(10000) < hearingRange) {
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

void Position::addCreature(PCreature c) const {
  PROFILE;
  if (isValid()) {
    Creature* ref = c.get();
    getModel()->addCreature(std::move(c));
    level->putCreature(coord, ref);
  }
}

bool Position::landCreature(PCreature c) const {
  return isValid() && getModel()->landCreature({*this}, std::move(c));
}

void Position::addCreature(PCreature c, TimeInterval delay) const {
  PROFILE;
  if (isValid()) {
    Creature* ref = c.get();
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

void Position::getViewIndex(ViewIndex& index, const Creature* viewer) const {
  PROFILE;
  if (isValid()) {
    getSquare()->getViewIndex(getGame()->getContentFactory(), index, viewer);
    if (isUnavailable())
      index.setHighlight(HighlightType::UNAVAILABLE);
    if (isCovered())
      index.setHighlight(HighlightType::INDOORS);
    for (auto furniture : getFurniture()) {
      if (furniture->doesHideItems())
        index.removeObject(ViewLayer::ITEM);
      if (furniture->isVisibleTo(viewer) && furniture->getViewObject()) {
        auto obj = *furniture->getViewObject();
        obj.setGenericId(level->getUniqueId() + coord.x * 2000 + coord.y);
        if (auto& id = furniture->getEmptyViewId())
          if (getInventory().isEmpty())
            obj.setId(*id);
        index.insert(std::move(obj));
      }
    }
    if (index.noObjects())
      index.insert(ViewObject(ViewId("empty"), ViewLayer::FLOOR_BACKGROUND));
    if (viewer) {
      if (auto& effects = level->furnitureEffects[viewer->getTribeId().getKey()])
        if (!effects->operator[](coord).friendly.empty())
          index.setHighlight(HighlightType::ALLIED_TOTEM);
      for (auto tribeKey : ENUM_ALL(TribeId::KeyType))
        if (tribeKey != viewer->getTribeId().getKey())
          if (auto& effects = level->furnitureEffects[tribeKey])
            if (!effects->operator[](coord).hostile.empty())
              index.setHighlight(HighlightType::HOSTILE_TOTEM);
    }
  }
}

const vector<Item*>& Position::getItems() const {
  PROFILE;
  if (isValid())
    return getSquare()->getInventory().getItems();
  else {
    static vector<Item*> empty;
    return empty;
  }
}

const vector<Item*>& Position::getItems(ItemIndex index) const {
  static vector<Item*> emptyItems;
  if (isValid())
    return getSquare()->getInventory().getItems(index);
  else
    return emptyItems;
}

const vector<Item*>& Position::getItems(CollectiveResourceId id) const {
  static vector<Item*> emptyItems;
  if (isValid())
    return getSquare()->getInventory().getItems(id);
  else
    return emptyItems;
}

PItem Position::removeItem(Item* it) const {
  CHECK(isValid());
  return modSquare()->removeItem(*this, it);
}

const Inventory& Position::getInventory() const {
  PROFILE;
  if (!isValid()) {
    static Inventory empty;
    return empty;
  } else
    return getSquare()->getInventory();
}

vector<PItem> Position::removeItems(vector<Item*> it) {
  PROFILE;
  CHECK(isValid());
  return modSquare()->removeItems(*this, it);
}

bool Position::isUnavailable() const {
  PROFILE;
  return !isValid() || level->unavailable[coord];
}

bool Position::canEnter(const Creature* c) const {
  PROFILE;
  return !isUnavailable() && !getCreature() && canEnterEmpty(c->getMovementType(getGame()));
}

bool Position::canEnter(const MovementType& t) const {
  PROFILE;
  return !isUnavailable() && !getCreature() && canEnterEmpty(t);
}

bool Position::canEnterEmpty(const Creature* c) const {
  PROFILE;
  return canEnterEmpty(c->getMovementType(getGame()));
}

bool Position::canEnterEmpty(const MovementType& movement) const {
  auto onlyMovement = movement;
  onlyMovement.setCanBuildBridge(false).setDestroyActions({});
  return canNavigate(onlyMovement);
}

bool Position::canEnterEmptyCalc(const MovementType& t, optional<FurnitureLayer> ignore) const {
  PROFILE;
  if (isUnavailable())
    return false;
  const auto square = getSquare();
  bool result = true;
  const bool covered = isCovered() || getSquare()->hasSunlightBlockingGasAmount();
  for (auto layer : ENUM_ALL(FurnitureLayer))
    if (layer != ignore)
      if (auto furniture = level->furniture->getBuilt(layer).getReadonly(coord)) {
        bool canEnter =
            furniture->getMovementSet().canEnter(t, covered, square->isOnFire(), square->getForbiddenTribe());
        if (furniture->overridesMovement())
          return canEnter;
        else
          result &= canEnter;
      }
  return result;
}

void Position::onEnter(Creature* c) const {
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

void Position::dropItem(PItem item) const {
  PROFILE;
  dropItems(makeVec(std::move(item)));
}

void Position::dropItems(vector<PItem> v) const {
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

void Position::addFurniture(PFurniture f) const {
  if (auto prev = getFurniture(f->getLayer()))
    removeFurniture(prev, std::move(f));
  else
    addFurnitureImpl(std::move(f));
}

void Position::addFurnitureImpl(PFurniture f) const {
  PROFILE;
  if (isValid()) {
    CHECK(!getFurniture(f->getLayer()));
    auto furniture = f.get();
    level->setFurniture(coord, std::move(f));
    updateConnectivity();
    if (furniture->blocksAnyVision())
      updateVisibility();
    level->addLightSource(coord, furniture->getLightEmission());
    updateSupportViewId(furniture);
    setNeedsRenderAndMemoryUpdate(true);
    if (auto& effect = furniture->getLastingEffectInfo())
      addFurnitureEffect(furniture->getTribe(), *effect);
  }
}

template <typename Fun1, typename Fun2>
void handleEffect(TribeId tribe, Level::EffectsTable& effectsTable, vector<Position> positions,
    const FurnitureEffectInfo& effect, Fun1 fun1, Fun2 fun2) {
  for (auto pos : positions)
    if (pos.isValid()) {
      auto c = pos.getCreature();
      if (effect.target == FurnitureEffectInfo::Target::ENEMY) {
        fun1(effectsTable[pos.getCoord()].hostile);
        if (c && c->getTribeId() != tribe)
          fun2(c);
      } else {
        fun1(effectsTable[pos.getCoord()].friendly);
        if (c && c->getTribeId() == tribe)
          fun2(c);
      }
    }
}

void Position::addFurnitureEffect(TribeId tribe, const FurnitureEffectInfo& effect) const {
  auto& effectsTable = level->furnitureEffects[tribe.getKey()];
  if (!effectsTable)
    effectsTable = make_unique<Level::EffectsTable>(level->getBounds());
  handleEffect(tribe, *effectsTable, getRectangle(Rectangle::centered(effect.radius)), effect,
      [&](vector<LastingOrBuff>& effects) { effects.push_back(effect.effect); },
      [&](Creature* c) { addPermanentEffect(effect.effect, c, false); });
}

void Position::removeFurnitureEffect(TribeId tribe, const FurnitureEffectInfo& effect) const {
  auto& effectsTable = level->furnitureEffects[tribe.getKey()];
  CHECK(!!effectsTable);
  handleEffect(tribe, *effectsTable, getRectangle(Rectangle::centered(effect.radius)), effect,
      [&](vector<LastingOrBuff>& effects) { effects.removeElement(effect.effect); },
      [&](Creature* c) { removePermanentEffect(effect.effect, c, false); });
}

int Position::countSwarmers() const {
  if (!isValid())
    return 0;
  int result = 0;
  for (auto& map : level->swarmMaps)
    result = max(result, map.second.countElementsInBucket(coord + Vec2(map.first, map.first)));
  return result;
}

Collective* Position::getCollective() const {
  if (!isValid())
    return nullptr;
  return level->territory[coord];
}

optional<Position> Position::getGroundBelow() const {
  if (isUnavailable() && level->below) {
    Position ret(coord, level->below);
    while (ret.isUnavailable() && ret.level->below)
      ret = Position(coord, ret.level->below);
    return ret;
  }
  return none;
}

bool Position::isClosedOff(MovementType movement) const {
  auto sectors = level->getSectors(movement);
  auto topLevel = getModel()->getGroundLevel();
  if (level == topLevel && sectors.isSector(coord, sectors.getLargest()))
    return false;
  for (auto key : level->getAllStairKeys())
    if (sectors.same(coord, level->getLandingSquares(key)[0].getCoord()))
      return false;
  return true;
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

void Position::addSwarmer() {
  if (isValid())
    level->placeSwarmer(coord, NOTNULL(getCreature()));
}

void Position::removeSwarmer() {
  if (isValid())
    level->unplaceSwarmer(coord, NOTNULL(getCreature()));
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

void Position::removeFurniture(FurnitureLayer layer) const {
  if (auto f = getFurniture(layer))
    removeFurniture(f);
}

void Position::removeFurniture(const Furniture* f) const {
  removeFurniture(f, nullptr);
}

void Position::removeFurniture(const Furniture* f, PFurniture replace, Creature* destroyedBy, bool secondPart) const {
  PROFILE;
  f->beforeRemoved(*this);
  if (!secondPart)
    if (auto pos = f->getSecondPart(*this))
      pos->removeFurniture(pos->getFurniture(f->getLayer()), nullptr, destroyedBy, true);
  bool visibilityChanged = (!!replace) ? (f->blocksAnyVision() != replace->blocksAnyVision()) : f->blocksAnyVision();
  level->removeLightSource(coord, f->getLightEmission());
  auto replacePtr = replace.get();
  auto layer = f->getLayer();
  auto type = f->getType();
  CHECK(layer != FurnitureLayer::GROUND || !!replace);
  CHECK(!!getFurniture(layer)) << "null furniture " << type.data() << " " << EnumInfo<FurnitureLayer>::getString(layer);
  CHECK(getFurniture(layer) == f) << getFurniture(layer)->getType().data() << " " << type.data() << " " << EnumInfo<FurnitureLayer>::getString(layer);
  if (auto& effect = f->getLastingEffectInfo())
    removeFurnitureEffect(f->getTribe(), *effect);
  if (replace) {
    level->setFurniture(coord, std::move(replace));
    updateSupportViewId(replacePtr);
    if (auto& effect = replacePtr->getLastingEffectInfo())
      addFurnitureEffect(replacePtr->getTribe(), *effect);
  } else {
    level->furniture->getBuilt(layer).clearElem(coord);
    level->furniture->eraseConstruction(coord, layer);
  }
  updateMovementDueToFire();
  updateConnectivity();
  if (visibilityChanged)
    updateVisibility();
  updateSupport();
  if (replacePtr) {
    level->addLightSource(coord, replacePtr->getLightEmission());
    if (auto c = getCreature())
      replacePtr->onEnter(c);
  }
  setNeedsRenderAndMemoryUpdate(true);
  getGame()->addEvent(EventInfo::FurnitureRemoved{*this, type, layer, destroyedBy});
}

bool Position::canConstruct(FurnitureType type) const {
  PROFILE;
  auto factory = &getGame()->getContentFactory()->furniture;
  return !isUnavailable() && factory->canBuild(type, *this) && factory->getData(type).hasRequiredSupport(*this);
}

bool Position::isWall() const {
  PROFILE;
  if (auto furniture = getFurniture(FurnitureLayer::MIDDLE))
    return furniture->isWall();
  else
    return false;
}

void Position::construct(FurnitureType type, Creature* c) const {
  PROFILE;
  if (construct(type, c->getTribeId()))
    modFurniture(getGame()->getContentFactory()->furniture.getData(type).getLayer())->onConstructedBy(*this, c);
}

bool Position::construct(FurnitureType type, TribeId tribe) const {
  PROFILE;
  CHECK(!isUnavailable());
  CHECK(canConstruct(type));
  auto layer = getGame()->getContentFactory()->furniture.getData(type).getLayer();
  auto construction = level->furniture->getConstruction(coord, layer);
  if (!construction || construction->type != type)
    level->furniture->setConstruction(coord, layer, FurnitureArray::Construction{type, 10});
  else if (--construction->time == 0) {
    addFurniture(getGame()->getContentFactory()->furniture.getFurniture(type, tribe));
    return true;
  }
  return false;
}

bool Position::isActiveConstruction(FurnitureLayer layer) const {
  PROFILE;
  return !isUnavailable() && !!level->furniture->getConstruction(coord, layer);
}

bool Position::isBurning() const {
  PROFILE;
  auto check = [] (const Position& pos) {
    for (auto furniture : pos.getFurniture())
      if (furniture->getFire() && furniture->getFire()->isBurning())
        return true;
    return false;
  };
  if (check(*this))
    return true;
  for (auto& pos : neighbors8())
    if (check(pos))
      return true;
  return false;
}

void Position::updateMovementDueToFire() const {
  PROFILE;
  auto update = [] (const Position& pos) {
    if (pos.isValid()) {
      if (pos.isBurning()) {
        if (!pos.getSquare()->isOnFire()) {
          pos.modSquare()->setOnFire(true);
          pos.updateConnectivity();
        }
      } else
        if (pos.getSquare()->isOnFire()) {
          pos.modSquare()->setOnFire(false);
          pos.updateConnectivity();
        }
    }
  };
  update(*this);
  for (auto& v : neighbors8())
    update(v);
}

bool Position::fireDamage(int amount) const {
  PROFILE;
  bool res = false;
  for (auto furniture : modFurniture())
    if (Random.chance(0.05 * amount))
      res |= furniture->fireDamage(*this);
  if (Creature* creature = getCreature())
    creature->takeDamage(Attack(nullptr, Random.choose<AttackLevel>(), AttackType::HIT, amount,
        AttrType("FIRE_DAMAGE"), {}, "The fire is harmless"));
  for (Item* it : getItems())
    if (Random.chance(0.05 * amount))
      it->fireDamage(*this);
  return res;
}

bool Position::iceDamage(int amount) const {
  PROFILE;
  bool res = false;
  for (auto furniture : modFurniture())
    if (Random.chance(0.05 * amount))
      res |= furniture->iceDamage(*this);
  if (Creature* creature = getCreature())
    creature->takeDamage(Attack(nullptr, Random.choose<AttackLevel>(), AttackType::HIT, amount,
        AttrType("COLD_DAMAGE"), {}, "The cold is harmless"));
  for (Item* it : getItems())
    if (Random.chance(0.05 * amount))
      it->iceDamage(*this);
  return res;
}

bool Position::acidDamage(int amount) const {
  PROFILE;
  bool res = false;
  for (auto furniture : modFurniture())
    if (Random.chance(0.05 * amount))
      res |= furniture->acidDamage(*this);
  if (Creature* creature = getCreature())
    creature->takeDamage(Attack(nullptr, Random.choose<AttackLevel>(), AttackType::HIT, amount,
        AttrType("ACID_DAMAGE"), {}, "The acid is harmless"));    
  /*for (Item* it : getItems())
    if (Random.chance(amount))
      it->acidDamage(*this);*/
  return res;
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

void Position::setNeedsRenderAndMemoryUpdate(bool s) const {
  PROFILE;
  if (isValid()) {
    level->setNeedsRenderUpdate(getCoord(), s);
    level->setNeedsMemoryUpdate(getCoord(), s);
  }
}

ViewId Position::getTopViewId() const {
  PROFILE;
  for (auto layer : ENUM_ALL_REVERSE(FurnitureLayer))
    if (auto furniture = getFurniture(layer))
      if (auto& obj = furniture->getViewObject())
        return obj->id();
  return ViewId("empty");
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

void Position::addGas(TileGasType type, double amount) {
  PROFILE;
  if (isValid())
    modSquare()->addGas(*this, type, amount);
}

double Position::getGasAmount(TileGasType type) const {
  PROFILE;
  if (isValid())
    return getSquare()->getGasAmount(type);
  else
    return 0;
}

bool Position::isCovered() const {
  PROFILE;
  if (isValid()) {
    if (level->covered[coord])
      return true;
    auto ground = getFurniture(FurnitureLayer::GROUND);
    auto middle = getFurniture(FurnitureLayer::MIDDLE);
    return (ground && ground->getType() == FurnitureType("FLOOR")) || (middle && middle->isWall());
  } else
    return false;
}

bool Position::sunlightBurns() const {
  PROFILE;
  return isValid() && !isCovered() && level->lightCapAmount[coord] >= 1 &&
      getGame()->getSunlightInfo().getState() == SunlightState::DAY && !getSquare()->hasSunlightBlockingGasAmount();
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

void Position::throwItem(vector<PItem> item, const Attack& attack, int maxDist, Position target, VisionId vision) {
  PROFILE;
  if (isValid()) {
    CHECK(!item.empty());
    CHECK(isSameLevel(target));
    if (target == *this) {
      modSquare()->onItemLands(*this, std::move(item), attack);
      return;
    }
    int cnt = 1;
    vector<Position> trajectory;
    for (auto v : drawLine(coord, target.coord))
      if (v != coord)
        trajectory.push_back(Position(v, level));
    Position prev = *this;
    for (auto& pos : trajectory) {
      if (pos.stopsProjectiles(vision)) {
        item[0]->onHitSquareMessage(pos, attack, item.size());
        trajectory.pop_back();
        getGame()->addEvent(
            EventInfo::Projectile{none, item[0]->getViewObject().id(), *this, prev, SoundId::SHOOT_BOW});
        if (!item[0]->isDiscarded())
          prev.dropItems(std::move(item));
        return;
      }
      if (++cnt > maxDist || pos.getCreature() || pos == trajectory.back()) {
        getGame()->addEvent(
            EventInfo::Projectile{none, item[0]->getViewObject().id(), *this, pos, SoundId::SHOOT_BOW});
        pos.modSquare()->onItemLands(pos, std::move(item), attack);
        return;
      }
      prev = pos;
    }
  }
}

void Position::updateConnectivity() const {
  PROFILE;
  // It's important that sectors aren't generated at this point, because we need stale data to detect change.
  auto movementEventPredicate = [this] { return level->getSectorsDontCreate({MovementTrait::WALK}).contains(coord); };
  bool couldEnter = movementEventPredicate();
  if (isValid()) {
    for (auto& elem : level->sectors)
      if (canNavigateCalc(elem.first))
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
    for (auto layer : ENUM_ALL(FurnitureLayer))
      if (auto f = pos.modFurniture(layer))
        if (!getGame()->getContentFactory()->furniture.getData(f->getType()).hasRequiredSupport(pos))
          f->destroy(pos, DestroyAction::Type::BASH);
}

void Position::updateSupportViewId(Furniture* furniture) const {
  if (auto id = furniture->getSupportViewId(*this))
    if (*id != furniture->getViewObject()->id())
      furniture->getViewObject()->setId(*id);
}

const vector<Position>& Position::getLandingAtNextLevel(StairKey stairKey) {
  return NOTNULL(getModel()->getLinkedLevel(level, stairKey))->getLandingSquares(stairKey);
}

bool Position::canNavigateToOrNeighbor(Position from, const MovementType& type) const {
  if (canNavigateTo(from, type))
    return true;
  for (Position v : neighbors8())
    if (v.canNavigateTo(from, type))
      return true;
  return false;
}

bool Position::canNavigateTo(Position from, const MovementType& type) const {
  if (!isSameModel(from))
    return false;
  if (isConnectedTo(from, type))
    return true;
  for (auto key1 : level->getAllStairKeys()) {
    auto pos1 = level->getLandingSquares(key1)[0];
    if (isConnectedTo(pos1, type))
      for (auto key2 : from.level->getAllStairKeys()) {
        auto pos2 = from.level->getLandingSquares(key2)[0];
        if (from.isConnectedTo(pos2, type) && level->getModel()->areConnected(key1, key2, type))
          return true;
      }
  }
  return false;
}

optional<DestroyAction> Position::getBestDestroyAction(const MovementType& movement) const {
  PROFILE;
  if (auto furniture = getFurniture(FurnitureLayer::MIDDLE))
    if (canEnterEmptyCalc(movement, FurnitureLayer::MIDDLE)) {
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

double Position::getNavigationCost(const MovementType& movement, const Sectors& onltMovementSectors) const {
  PROFILE;
  if (onltMovementSectors.contains(coord)) {
    if (level->getSafeSquare(coord)->getCreature()) {
      return 5.0;
    } else
      return 1.0;
  }
  if (auto destroyAction = getBestDestroyAction(movement))
    return 1.0 + *getFurniture(FurnitureLayer::MIDDLE)->getStrength(*destroyAction) / 10;
  if (movement.canBuildBridge() && canConstruct(FurnitureType("BRIDGE")) &&
      !movement.isCompatible(getFurniture(FurnitureLayer::GROUND)->getTribe()))
    return 10;
  return ShortestPath::infinity;
}

bool Position::canNavigate(const MovementType& type) const {
  PROFILE;
  return isValid() && level->getSectors(type).contains(coord);
}

bool Position::canNavigateCalc(const MovementType& type) const {
  PROFILE;
  optional<FurnitureLayer> ignore;
  if (auto furniture = getFurniture(FurnitureLayer::MIDDLE))
    for (DestroyAction action : type.getDestroyActions())
      if (furniture->canDestroy(type, action))
        ignore = FurnitureLayer::MIDDLE;
  if (type.canBuildBridge() && canConstruct(FurnitureType("BRIDGE")) &&
      !type.isCompatible(getFurniture(FurnitureLayer::GROUND)->getTribe()))
    return true;
  return canEnterEmptyCalc(type, ignore);
}

bool Position::canSeeThruIgnoringGas(VisionId id) const {
  PROFILE;
  if (!isValid())
    return false;
  if (auto furniture = level->furniture->getBuilt(FurnitureLayer::MIDDLE).getReadonly(coord))
    return furniture->canSeeThru(id);
  return true;
}

bool Position::canSeeThru(VisionId id) const {
  return canSeeThru(id, getGame()->getContentFactory());
}

bool Position::canSeeThru(VisionId id, const ContentFactory* factory) const {
  PROFILE;
  if (!isValid() || !canSeeThruIgnoringGas(id))
    return false;
  for (auto& type : factory->tileGasTypes)
    if (type.second.blocksVision && getSquare()->getGasAmount(type.first) >= TileGas::getFogVisionCutoff())
      return false;
  return true;
}

bool Position::stopsProjectiles(VisionId id) const {
  PROFILE;
  if (auto furniture = getFurniture(FurnitureLayer::MIDDLE))
    return furniture->stopsProjectiles(id);
  else
    return !isValid();
}

bool Position::isVisibleBy(const Creature* c) const {
  return c->getPosition().canSee(*this, c->getVision());
}

bool Position::canSee(Position other, const Vision& vision) const {
  PROFILE;
  return isValid() && other.isSameLevel(*this) && level->canSee(coord, other.coord, vision);
}

void Position::clearItemIndex(ItemIndex index) const {
  PROFILE;
  if (isValid())
    modSquare()->clearItemIndex(index);
}

bool Position::isConnectedTo(Position pos, const MovementType& movement) const {
  PROFILE;
  return isValid() && pos.isValid() && level == pos.level &&
      level->getSectors(movement).same(coord, pos.coord);
}

vector<Creature*> Position::getAllCreatures(int range) const {
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
  else pos.landCreature(getModel()->extractCreature(getCreature()));
  if (teleportEffect)
    getGame()->addEvent(EventInfo::FX{pos, FXName::TELEPORT_IN});
}

void Position::moveCreature(Vec2 direction) {
  PROFILE;
  CHECK(isValid());
  level->moveCreature(getCreature(), direction);
}

static bool canPass(Position position, const Creature* c) {
  PROFILE;
  return position.canEnterEmpty(c) && (!position.getCreature() ||
      !position.getCreature()->getAttributes().isBoulder());
}

bool Position::canMoveCreature(Vec2 direction) const {
  PROFILE;
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

optional<pair<Position, int>> Position::getStairsTo(Position targetPos, const MovementType& movement,
    bool includeNeighbors) const {
  PROFILE;
  unordered_map<StairKey, int, CustomHash<StairKey>> distance;
  using QueueElem = tuple<StairKey, Level*, Level*, int>;
  auto queueCmp = [](auto& elem1, auto& elem2) { return std::get<3>(elem1) > std::get<3>(elem2); };
  priority_queue<QueueElem, vector<QueueElem>, decltype(queueCmp)> q(queueCmp);
  auto model = targetPos.getModel();
  auto relaxKey = [model, &movement, &distance, &q](int thisValue, Position thisPos) {
    auto level = thisPos.level;
    for (auto key : level->getAllStairKeys())
      if (auto linked = model->getLinkedLevel(level, key)) {
        auto stairpos = level->getLandingSquares(key)[0];
        if (thisPos.isConnectedTo(stairpos, movement)) {
          auto value = thisValue + *thisPos.dist8(stairpos) + 1;
          auto cur = getValueMaybe(distance, key);
          if (!cur || *cur > value) {
            distance[key] = value;
            q.push(make_tuple(key, level, linked, value));
          }
        }
      }
  };
  relaxKey(0, targetPos);
  if (includeNeighbors)
    for (auto neighbor : targetPos.neighbors8())
      relaxKey(0, neighbor);
  while (!q.empty()) {
    auto cur = q.top();
    q.pop();
    auto thisKey = std::get<0>(cur);
    auto l1 = std::get<1>(cur);
    auto l2 = std::get<2>(cur);
    auto thisValue = std::get<3>(cur);
    if (thisValue == distance.at(thisKey)) {
      relaxKey(thisValue, l1->getLandingSquares(thisKey)[0]);
      relaxKey(thisValue, l2->getLandingSquares(thisKey)[0]);
    }
  }
  optional<pair<Position, int>> ret;
  for (auto key : level->getAllStairKeys())
    if (auto keyRes = getValueMaybe(distance, key)) {
      auto stairPos = level->getLandingSquares(key)[0];
      if (isConnectedTo(stairPos, movement)) {
        auto res = *keyRes + *dist8(stairPos);
        if (!ret || res < ret->second)
          ret = make_pair(stairPos, res);
      }
    }
  return ret;
}

void Position::swapCreatures(Creature* c) {
  PROFILE;
  CHECK(isValid() && getCreature());
  level->swapCreatures(getCreature(), c);
}

Position Position::withCoord(Vec2 newCoord) const {
  PROFILE;
  CHECK(isValid());
  return Position(newCoord, level);
}

void Position::putCreature(Creature* c) {
  PROFILE;
  CHECK(isValid());
  level->putCreature(coord, c);
}
