/* Copyright (C) 2013-2014 Michal Brzozowski (rusolis@poczta.fm)

   This file is part of KeeperRL.

   KeeperRL is free software; you can redistribute it and/or modify it under the terms of the
   GNU General Public License as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   KeeperRL is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
   even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along with this program.
   If not, see http://www.gnu.org/licenses/ . */

#include "stdafx.h"

#include "square_factory.h"
#include "square.h"
#include "creature.h"
#include "level.h"
#include "item_factory.h"
#include "creature_factory.h"
#include "effect.h"
#include "view_object.h"
#include "view_id.h"
#include "model.h"
#include "monster_ai.h"
#include "player_message.h"
#include "square_apply_type.h"
#include "tribe.h"
#include "creature_name.h"
#include "modifier_type.h"
#include "movement_set.h"
#include "movement_type.h"
#include "stair_key.h"
#include "view.h"
#include "sound.h"
#include "creature_attributes.h"
#include "square_interaction.h"
#include "game.h"
#include "event_listener.h"

class Magma : public Square {
  public:
  Magma(const ViewObject& object, const string& name) : Square(object,
      CONSTRUCT(Square::Params,
        c.name = name;
        c.vision = VisionId::NORMAL;
        c.constructions = ConstructionsId::BRIDGE;
        c.movementSet = MovementSet()
            .addTrait(MovementTrait::FLY)
            .addForcibleTrait(MovementTrait::WALK);)) {}

  virtual void onEnterSpecial(Creature* c) override {
    MovementType realMovement = c->getMovementType();
    realMovement.setForced(false);
    if (!canEnterEmpty(realMovement)) {
      c->you(MsgType::BURN, getName());
      c->die("burned to death", false);
    }
  }

  virtual void dropItems(Position position, vector<PItem> items) override {
    for (auto elem : Item::stackItems(extractRefs(items)))
      position.globalMessage(elem.second[0]->getPluralTheNameAndVerb(elem.second.size(),
            "burns", "burn") + " in the magma.");
  }

  SERIALIZE_SUBCLASS(Square);
  SERIALIZATION_CONSTRUCTOR(Magma);
};

class Water : public Square {
  public:
  Water(ViewObject object, const string& name, double _depth)
      : Square(object.setAttribute(ViewObject::Attribute::WATER_DEPTH, _depth),
          CONSTRUCT(Square::Params,
            c.name = name;
            c.vision = VisionId::NORMAL;
            c.constructions = ConstructionsId::BRIDGE;
            c.movementSet = getMovement(_depth);
          )) {}

  virtual void onEnterSpecial(Creature* c) override {
    MovementType realMovement = c->getMovementType();
    realMovement.setForced(false);
    if (!canEnterEmpty(realMovement)) {
      c->you(MsgType::DROWN, getName());
      c->die("drowned", false);
    }
  }

  virtual void dropItems(Position position, vector<PItem> items) override {
    for (auto elem : Item::stackItems(extractRefs(items)))
      position.globalMessage(elem.second[0]->getPluralTheNameAndVerb(elem.second.size(),
            "sinks", "sink") + " in the water.", "You hear a splash.");
  }

  SERIALIZE_SUBCLASS(Square);
  SERIALIZATION_CONSTRUCTOR(Water);
  
  private:
  static MovementSet getMovement(double depth) {
    MovementSet ret;
    ret.addTrait(MovementTrait::SWIM);
    ret.addTrait(MovementTrait::FLY);
    ret.addForcibleTrait(MovementTrait::WALK);
    if (depth < 1.5)
      ret.addTrait(MovementTrait::WADE);
    return ret;
  }
};

class Chest : public Square {
  public:
  Chest(const ViewObject& object, const string& name, const ChestInfo& info, SquareId _openedId,
      const string& _msgItem, const string& _msgMonster, const string& _msgGold, ItemFactory _itemFactory)
    : Square(object,
          CONSTRUCT(Square::Params,
            c.name = name;
            c.vision = VisionId::NORMAL;
            c.canHide = true;
            c.canDestroy = true;
            c.strength = 30;
            c.movementSet = MovementSet().addTrait(MovementTrait::WALK);
            c.flamability = 0.5;
            c.applyType = SquareApplyType::USE_CHEST;
            )), chestInfo(info),
    msgItem(_msgItem), msgMonster(_msgMonster), msgGold(_msgGold), itemFactory(_itemFactory), openedId(_openedId) {}

  virtual void onEnterSpecial(Creature* c) override {
    c->playerMessage(string("There is a ") + (opened ? " opened " : "") + getName() + " here");
  }

  virtual void onConstructNewSquare(Position position, Square* s) const override {
    if (!opened) {
      ItemFactory f(itemFactory);
      s->dropItems(position, f.random());
    }
  }

  virtual bool canApply(const Creature* c) const override {
    return c->getBody().isHumanoid();
  }

  virtual void onApply(Creature* c) override {
    CHECK(!opened);
    c->playerMessage("You open the " + getName());
    opened = true;
    if (chestInfo.creature && chestInfo.creatureChance > 0 && Random.roll(1 / chestInfo.creatureChance)) {
      int numR = chestInfo.numCreatures;
      CreatureFactory factory(*chestInfo.creature);
      for (Position v : c->getPosition().neighbors8(Random)) {
        PCreature rat = factory.random();
        if (v.canEnter(rat.get())) {
          v.addCreature(std::move(rat));
          if (--numR == 0)
            break;
        }
      }
      if (numR < chestInfo.numCreatures)
        c->playerMessage(msgMonster);
    } else {
      c->playerMessage(msgItem);
      vector<PItem> items = itemFactory.random();
      c->getGame()->addEvent([&](EventListener* l) {
          l->onItemsAppearedEvent(c->getPosition(),  extractRefs(items));});
      c->getPosition().dropItems(std::move(items));
    }
    c->getLevel()->replaceSquare(c->getPosition(), SquareFactory::get(openedId));
  }

  SERIALIZE_ALL2(Square, chestInfo, msgItem, msgMonster, msgGold, itemFactory, openedId);
  SERIALIZATION_CONSTRUCTOR(Chest);

  private:
  ChestInfo SERIAL(chestInfo);
  string SERIAL(msgItem);
  string SERIAL(msgMonster);
  string SERIAL(msgGold);
  bool opened = false;
  ItemFactory SERIAL(itemFactory);
  SquareId SERIAL(openedId);
};

class Fountain : public Square {
  public:
  Fountain(const ViewObject& object) : Square(object,
      CONSTRUCT(Square::Params,
        c.name = "fountain";
        c.vision = VisionId::NORMAL;
        c.canHide = true;
        c.canDestroy = true;
        c.movementSet = MovementSet().addTrait(MovementTrait::WALK);
        c.strength = 100;
        c.applyType = SquareApplyType::DRINK;
        )) {}

  virtual void onEnterSpecial(Creature* c) override {
    c->playerMessage("There is a " + getName() + " here");
  }

  virtual void onApply(Creature* c) override {
    c->playerMessage("You drink from the fountain.");
    PItem potion = getOnlyElement(ItemFactory::potions().random(seed));
    potion->apply(c);
  }


  SERIALIZE_ALL2(Square, seed);
  SERIALIZATION_CONSTRUCTOR(Fountain);

  private:
  int SERIAL(seed) = Random.get(123456);
};

class Tree : public Square {
  public:
  Tree(const ViewObject& object, const string& name, VisionId vision, int _numWood, CreatureId c) : Square(object,
        CONSTRUCT(Square::Params,
          c.name = name;
          c.vision = vision;
          c.canHide = true;
          c.strength = 100;
          c.canDestroy = true;
          c.flamability = 0.4;
          c.movementSet = MovementSet().addTrait(MovementTrait::WALK);
          c.constructions = ConstructionsId::CUT_TREE;)), numWood(_numWood), creature(c) {}

  virtual void destroy(Position position) override {
    position.globalMessage("The tree falls.");
    position.getLevel()->replaceSquare(position, SquareFactory::get(SquareId::TREE_TRUNK));
  }

  virtual void onConstructNewSquare(Position position, Square* s) const override {
    if (numWood > 0) {
      position.dropItems(ItemFactory::fromId(ItemId::WOOD_PLANK, numWood));
      position.getModel()->addWoodCount(numWood);
      int numCut = position.getModel()->getWoodCount();
      if (numCut > 1500 && Random.roll(max(150, (3000 - numCut) / 5))) {
        CreatureFactory f = CreatureFactory::singleType(TribeId::getHostile(), creature);
        Effect::summon(position, f, 1, 100000);
      }
    }
  }

  virtual void burnOut(Position pos) override {
    numWood = 0;
    pos.getLevel()->replaceSquare(pos, SquareFactory::get(SquareId::BURNT_TREE));
  }

  SERIALIZE_ALL2(Square, creature, numWood);
  SERIALIZATION_CONSTRUCTOR(Tree);

  private:
  int SERIAL(numWood);
  CreatureId SERIAL(creature);
};

class TrapSquare : public Square {
  public:
  TrapSquare(const ViewObject& object, EffectType e) : Square(object,
      CONSTRUCT(Square::Params,
        c.name = "floor";
        c.movementSet = MovementSet().addTrait(MovementTrait::WALK);
        c.vision = VisionId::NORMAL;)), effect(e) {
  }

  virtual void onEnterSpecial(Creature* c) override {
    if (active && c->isPlayer()) {
      c->you(MsgType::TRIGGER_TRAP, "");
      Effect::applyToCreature(c, effect, EffectStrength::NORMAL);
      active = false;
    }
  }

  SERIALIZE_ALL2(Square, active, effect);
  SERIALIZATION_CONSTRUCTOR(TrapSquare);

  private:
  bool SERIAL(active) = true;
  EffectType SERIAL(effect);
};

class Door : public Square {
  public:
  Door(const ViewObject& object, Square::Params params) : Square(object, params) {}

  virtual void onEnterSpecial(Creature* c) override {
    c->playerMessage("You open the door.");
  }


  SERIALIZE_SUBCLASS(Square);
  SERIALIZATION_CONSTRUCTOR(Door);
};

class TribeDoor : public Door {
  public:

  TribeDoor(const ViewObject& object, TribeId t, int destStrength, Square::Params params)
    : Door(object, CONSTRUCT(Square::Params,
          c = params;
          c.movementSet->addTraitForTribe(t, MovementTrait::WALK).removeTrait(MovementTrait::WALK);
          c.applyType = SquareApplyType::LOCK;
          )),
      tribe(t), destructionStrength(destStrength) {
  }

  virtual void destroyBy(Position pos, Creature* c) override {
    destructionStrength -= c->getAttr(AttrType::STRENGTH);
    if (destructionStrength <= 0) {
      Door::destroyBy(pos, c);
    } else
      c->addSound(SoundId::BANG_DOOR);
  }

  virtual void onApply(Creature* c) override {
    onApply(c->getPosition());
  }

  virtual void onApply(Position pos) override {
    locked = !locked;
    if (locked) {
      modViewObject().setModifier(ViewObject::Modifier::LOCKED);
      removeTraitForTribe(pos, tribe, MovementTrait::WALK);
    } else {
      modViewObject().removeModifier(ViewObject::Modifier::LOCKED);
      addTraitForTribe(pos, tribe, MovementTrait::WALK);
    }
    setDirty(pos);
  }

  SERIALIZE_ALL2(Door, tribe, destructionStrength, locked);
  SERIALIZATION_CONSTRUCTOR(TribeDoor);

  private:
  TribeId SERIAL(tribe);
  int SERIAL(destructionStrength);
  bool SERIAL(locked) = false;
};

class Barricade : public Square {
  public:
  Barricade(const ViewObject& object, TribeId t, int destStrength) : Square(object,
      CONSTRUCT(Square::Params,
        c.name = "barricade";
        c.vision = VisionId::NORMAL;
        c.canDestroy = true;
        c.flamability = 0.5;
        c.owner = t;)),
      destructionStrength(destStrength) {
  }

  virtual void destroyBy(Position pos, Creature* c) override {
    destructionStrength -= c->getAttr(AttrType::STRENGTH);
    if (destructionStrength <= 0) {
      Square::destroyBy(pos, c);
    }
  }

  SERIALIZE_ALL2(Square, destructionStrength);
  SERIALIZATION_CONSTRUCTOR(Barricade);

  private:
  int SERIAL(destructionStrength);
};

class Furniture : public Square {
  public:
  Furniture(ViewObject object, const string& name, double flamability,
      optional<SquareApplyType> applyType = none, optional<SoundId> applySound = none) 
      : Square(object.setModifier(ViewObject::Modifier::ROUND_SHADOW),
          CONSTRUCT(Square::Params,
            c.name = name;
            c.vision = VisionId::NORMAL;
            c.canHide = true;
            c.strength = 100;
            c.canDestroy = true;
            c.applySound = applySound;
            c.movementSet = MovementSet().addTrait(MovementTrait::WALK);
            c.flamability = flamability;
            c.applyType = applyType;
            )) {}

  virtual void onApply(Creature* c) override {}

  SERIALIZE_SUBCLASS(Square);
  SERIALIZATION_CONSTRUCTOR(Furniture);
};

class Bed : public Furniture {
  public:
  Bed(const ViewObject& object, const string& name, double flamability = 1) : 
      Furniture(object, name, flamability, SquareApplyType::SLEEP) {}

  virtual void onApply(Creature* c) override {
    Effect::applyToCreature(c, {EffectId::LASTING, LastingEffect::SLEEP}, EffectStrength::STRONG);
    c->getPosition().getLevel()->addTickingSquare(c->getPosition().getCoord());
  }

  virtual void tickSpecial(Position) override {
    if (getCreature() && getCreature()->isAffected(LastingEffect::SLEEP))
      getCreature()->heal(0.005);
  }

  SERIALIZE_SUBCLASS(Furniture);
  SERIALIZATION_CONSTRUCTOR(Bed);
};

class Grave : public Bed {
  public:
  Grave(const ViewObject& object, const string& name) : Bed(object, name, 0) {}

  virtual bool canApply(const Creature* c) const override {
    return c->getBody().isUndead();
  }

  virtual void onApply(Creature* c) override {
    CHECK(c->getBody().isUndead());
    Bed::onApply(c);
  }

  SERIALIZE_SUBCLASS(Bed);
  SERIALIZATION_CONSTRUCTOR(Grave);
};

class Altar : public Square {
  public:
  Altar(const ViewObject& object) : Square(object,
      CONSTRUCT(Square::Params,
        c.name = "shrine";
        c.vision = VisionId::NORMAL;
        c.canHide = true;
        c.canDestroy = true;
        c.movementSet = MovementSet().addTrait(MovementTrait::WALK);
        c.strength = 100;
        c.applyType = SquareApplyType::PRAY;
        )) {
  }

  virtual bool canApply(const Creature* c) const override {
    return c->getBody().isHumanoid();
  }

 /* virtual void onKilled(Creature* victim, Creature* killer) override {
    if (killer) {
      recentKiller = killer;
      recentVictim = victim;
      killTime = killer->getTime();
    }
  }*/

  virtual void onPrayer(Creature* c) = 0;
  virtual void onSacrifice(Creature* c) = 0;
  virtual string getName() = 0;

  virtual void onApply(Creature* c) override {
/*    if (c == recentKiller && recentVictim && killTime >= c->getTime() - sacrificeTimeout)
      for (Item* it : getItems(Item::classPredicate(ItemClass::CORPSE)))
        if (it->getCorpseInfo()->victim == recentVictim->getUniqueId()) {
          c->you(MsgType::SACRIFICE, getName());
          c->globalMessage(it->getTheName() + " is consumed in a flash of light!");
          removeItem(it);
          onSacrifice(c);
          return;
        }*/
    c->playerMessage("You pray to " + getName());
    onPrayer(c);
  }

  SERIALIZE_ALL2(Square, recentKiller, recentVictim, killTime);
  SERIALIZATION_CONSTRUCTOR(Altar);

  private:
  const Creature* SERIAL(recentKiller) = nullptr;
  const Creature* SERIAL(recentVictim) = nullptr;
  double SERIAL(killTime) = -100;
//  const double sacrificeTimeout = 50;
};

class CreatureAltar : public Altar {
  public:
  CreatureAltar(const ViewObject& object, const Creature* c) : Altar(ViewObject(object.id(), object.layer(),
        "Shrine to " + c->getName().bare())), creature(c) {
  }

  virtual void onEnterSpecial(Creature* c) override {
    if (c->getBody().isHumanoid()) {
      c->playerMessage("This is a shrine to " + creature->getName().bare());
      c->playerMessage(creature->getAttributes().getDescription());
    }
  }

  virtual void destroyBy(Position pos, Creature* c) override {
  //  GlobalEvents.addWorshipCreatureEvent(c, creature, WorshipType::DESTROY_ALTAR);
  //  Altar::destroyBy(c);
  }

  virtual string getName() override {
    return creature->getName().bare();
  }

  virtual void onPrayer(Creature* c) override {
//    GlobalEvents.addWorshipCreatureEvent(c, creature, WorshipType::PRAYER);
  }

  virtual void onSacrifice(Creature* c) override {
 //   GlobalEvents.addWorshipCreatureEvent(c, creature, WorshipType::SACRIFICE);
  }

  SERIALIZE_ALL2(Altar, creature);
  SERIALIZATION_CONSTRUCTOR(CreatureAltar);

  private:
  const Creature* SERIAL(creature);
};

class MountainOre : public Square {
  public:
  MountainOre(const ViewObject& object, const string& name, ItemId ore, int dropped) : Square(object,
        CONSTRUCT(Square::Params,
          c.name = name;
          c.movementSet->setCovered(true);
          c.constructions = ConstructionsId::MINING_ORE;)),
      oreId(ore), numDropped(dropped) {}

  virtual void onConstructNewSquare(Position pos, Square* s) const override {
    pos.dropItems(ItemFactory::fromId(oreId, numDropped));
  }

  SERIALIZE_ALL2(Square, oreId, numDropped);
  SERIALIZATION_CONSTRUCTOR(MountainOre);

  private:
  ItemId SERIAL(oreId);
  int SERIAL(numDropped);
};

class Torch : public Furniture {
  public:
  Torch(const ViewObject& object, const string& name) : Furniture(object, name, 1) {}

  double getLightEmission() const override {
    return 8.2;
  }

  SERIALIZE_SUBCLASS(Furniture);
  SERIALIZATION_CONSTRUCTOR(Torch);
};

class Hatchery : public Square {
  public:
  Hatchery(const ViewObject& object, const string& name, CreatureFactory c) : Square(object,
      CONSTRUCT(Square::Params,
        c.name = name;
        c.vision = VisionId::NORMAL;
        c.movementSet = MovementSet().addTrait(MovementTrait::WALK);
        c.canDestroy = true;
        c.ticking = true;
        c.applyType = SquareApplyType::PIGSTY;
        )),
    creature(c) {}

  virtual void tickSpecial(Position pos) override {
    if (getCreature() || !Random.roll(10) || getPoisonGasAmount() > 0)
      return;
    for (Position v : pos.neighbors8())
      if (v.getCreature() && v.getCreature()->getBody().isMinionFood())
        return;
    if (Random.roll(5)) {
      PCreature pig = creature.random(
          MonsterAIFactory::stayInPigsty(pos, SquareApplyType::PIGSTY));
      if (canEnter(pig.get()))
        pos.addCreature(std::move(pig));
    }
  }

  virtual void onApply(Creature* c) override {
  }

  SERIALIZE_ALL2(Square, creature);
  SERIALIZATION_CONSTRUCTOR(Hatchery);

  private:
  CreatureFactory SERIAL(creature);
};

class Laboratory : public Furniture {
  public:
  Laboratory(ViewObject object, const string& name) : Furniture(object, name, 0, SquareApplyType::WORKSHOP) {}

  virtual void onApply(Creature* c) override {
    c->playerMessage("You mix the concoction.");
  }

  SERIALIZE_SUBCLASS(Furniture);
  SERIALIZATION_CONSTRUCTOR(Laboratory);
};

class NoticeBoard : public Furniture {
  public:
  NoticeBoard(ViewObject object, const string& t)
      : Furniture(object, "message board", 1, SquareApplyType::NOTICE_BOARD), text(t) {}

  virtual void onApply(Creature* c) override {
    c->playerMessage(PlayerMessage::announcement("The message board reads:", text));
  }

  SERIALIZE_ALL2(Furniture, text);
  SERIALIZATION_CONSTRUCTOR(NoticeBoard);

  private:
  string SERIAL(text);
};

class Crops : public Square {
  public:
  Crops(const ViewObject& obj, const string& name) : Square(obj,
      CONSTRUCT(Square::Params,
        c.name = name;
        c.vision = VisionId::NORMAL;
        c.movementSet = MovementSet().addTrait(MovementTrait::WALK);
        c.applyType = SquareApplyType::CROPS;
        c.applyTime = 3;
        )) {
  }

  virtual void onApply(Creature* c) override {
    if (Random.roll(3))
      c->globalMessage(c->getName().the() + " scythes the field.");
  }

  SERIALIZE_SUBCLASS(Square);
  SERIALIZATION_CONSTRUCTOR(Crops);
};

class SokobanHole : public Square {
  public:
  SokobanHole(const ViewObject& obj, const string& name, StairKey key) : Square(obj,
      CONSTRUCT(Square::Params,
        c.name = name;
        c.vision = VisionId::NORMAL;
        c.canHide = false;
        c.strength = 10000;
        c.movementSet = MovementSet()
            .addForcibleTrait(MovementTrait::WALK)
            .addTrait(MovementTrait::FLY);)),
    stairKey(key) {
  }

  virtual void onEnterSpecial(Creature* c) override {
    if (c->getAttributes().isStationary()) {
      c->getPosition().globalMessage(c->getName().the() + " fills the " + getName());
      c->die(nullptr, false, false);
      c->getPosition().getLevel()->replaceSquare(c->getPosition(), SquareFactory::get(SquareId::FLOOR));
    } else {
      if (!c->isAffected(LastingEffect::FLYING))
        c->you(MsgType::FALL, "into the " + getName() + "!");
      else
        c->you(MsgType::ARE, "sucked into the " + getName() + "!");
      c->getPosition().getLevel()->changeLevel(stairKey, c);
    }
  }

  SERIALIZE_ALL2(Square, stairKey);
  SERIALIZATION_CONSTRUCTOR(SokobanHole);

  private:
  StairKey SERIAL(stairKey);
};

template <class Archive>
void SquareFactory::registerTypes(Archive& ar, int version) {
  REGISTER_TYPE(ar, Laboratory);
  REGISTER_TYPE(ar, Magma);
  REGISTER_TYPE(ar, Water);
  REGISTER_TYPE(ar, Chest);
  REGISTER_TYPE(ar, Fountain);
  REGISTER_TYPE(ar, Tree);
  REGISTER_TYPE(ar, TrapSquare);
  REGISTER_TYPE(ar, Door);
  REGISTER_TYPE(ar, TribeDoor);
  REGISTER_TYPE(ar, Furniture);
  REGISTER_TYPE(ar, Bed);
  REGISTER_TYPE(ar, Grave);
  REGISTER_TYPE(ar, Barricade);
  REGISTER_TYPE(ar, Torch);
  REGISTER_TYPE(ar, CreatureAltar);
  REGISTER_TYPE(ar, MountainOre);
  REGISTER_TYPE(ar, Hatchery);
  REGISTER_TYPE(ar, Crops);
  REGISTER_TYPE(ar, NoticeBoard);
  REGISTER_TYPE(ar, SokobanHole);
}

REGISTER_TYPES(SquareFactory::registerTypes);

PSquare SquareFactory::get(SquareType s) {
  return PSquare(getPtr(s));
}

static Square* getStairs(const StairInfo& info) {
  Square* ret = new Square(ViewObject(info.direction == info.UP ? ViewId::UP_STAIRCASE : ViewId::DOWN_STAIRCASE,
        ViewLayer::FLOOR), 
      CONSTRUCT(Square::Params,
        c.name = "stairs";
        c.vision = VisionId::NORMAL;
        c.canHide = true;
        c.strength = 10000;
        c.movementSet = MovementSet().addTrait(MovementTrait::WALK);
        c.interaction = SquareInteraction::STAIRS;
        ));
  ret->setLandingLink(info.key);
  return ret;
}
 
Square* SquareFactory::getPtr(SquareType s) {
  switch (s.getId()) {
    case SquareId::FLOOR:
        return new Square(ViewObject(ViewId::FLOOR, ViewLayer::FLOOR_BACKGROUND),
            CONSTRUCT(Square::Params,
              c.name = "floor";
              c.vision = VisionId::NORMAL;
              c.movementSet = MovementSet().addTrait(MovementTrait::WALK);
              c.constructions = ConstructionsId::DUNGEON_ROOMS;
            ));
    case SquareId::BLACK_FLOOR:
        return new Square(ViewObject(ViewId::EMPTY, ViewLayer::FLOOR_BACKGROUND, "Floor"),
            CONSTRUCT(Square::Params,
              c.name = "floor";
              c.vision = VisionId::NORMAL;
              c.movementSet = MovementSet().addTrait(MovementTrait::WALK);));
    case SquareId::BRIDGE:
        return new Square(ViewObject(ViewId::BRIDGE, ViewLayer::FLOOR),
            CONSTRUCT(Square::Params,
              c.name = "rope bridge";
              c.movementSet = MovementSet().addTrait(MovementTrait::WALK);
              c.vision = VisionId::NORMAL;));
    case SquareId::GRASS:
        return new Square(ViewObject(ViewId::GRASS, ViewLayer::FLOOR_BACKGROUND),
            CONSTRUCT(Square::Params,
              c.name = "grass";
              c.movementSet = MovementSet().addTrait(MovementTrait::WALK);
              c.vision = VisionId::NORMAL;
              c.constructions = ConstructionsId::OUTDOOR_INSTALLATIONS;
            ));
    case SquareId::CROPS:
        return new Crops(ViewObject(Random.choose(ViewId::CROPS, ViewId::CROPS2), ViewLayer::FLOOR_BACKGROUND),
            "wheat");
    case SquareId::MUD:
        return new Square(ViewObject(ViewId::MUD, ViewLayer::FLOOR_BACKGROUND),
            CONSTRUCT(Square::Params,
              c.name = "mud";
              c.vision = VisionId::NORMAL;
              c.movementSet = MovementSet().addTrait(MovementTrait::WALK);));
    case SquareId::ROAD:
        return new Square(ViewObject(ViewId::ROAD, ViewLayer::FLOOR)
            .setModifier(ViewObject::Modifier::ROAD),
            CONSTRUCT(Square::Params,
              c.name = "road";
              c.vision = VisionId::NORMAL;
              c.movementSet = MovementSet().addTrait(MovementTrait::WALK);));
    case SquareId::ROCK_WALL:
        return new Square(ViewObject(ViewId::WALL, ViewLayer::FLOOR)
            .setModifier(ViewObject::Modifier::CASTS_SHADOW),
            CONSTRUCT(Square::Params,
              c.name = "wall";
              c.constructions = ConstructionsId::MINING;
            ));
    case SquareId::GOLD_ORE:
        return new MountainOre(ViewObject(ViewId::GOLD_ORE, ViewLayer::FLOOR)
            .setModifier(ViewObject::Modifier::CASTS_SHADOW), "gold ore", ItemId::GOLD_PIECE, Random.get(18, 40));
    case SquareId::IRON_ORE:
        return new MountainOre(ViewObject(ViewId::IRON_ORE, ViewLayer::FLOOR)
            .setModifier(ViewObject::Modifier::CASTS_SHADOW), "iron ore", ItemId::IRON_ORE, Random.get(18, 40));
    case SquareId::STONE:
        return new MountainOre(ViewObject(ViewId::STONE, ViewLayer::FLOOR)
            .setModifier(ViewObject::Modifier::CASTS_SHADOW), "granite", ItemId::ROCK, Random.get(18, 40));
    case SquareId::WOOD_WALL:
        return new Square(ViewObject(ViewId::WOOD_WALL, ViewLayer::FLOOR)
            .setModifier(ViewObject::Modifier::CASTS_SHADOW), CONSTRUCT(Square::Params,
              c.name = "wall";
              c.flamability = 0.4;));
    case SquareId::BLACK_WALL:
        return new Square(ViewObject(ViewId::EMPTY, ViewLayer::FLOOR, "Wall")
            .setModifier(ViewObject::Modifier::CASTS_SHADOW), CONSTRUCT(Square::Params, c.name = "wall";));
    case SquareId::CASTLE_WALL:
        return new Square(ViewObject(ViewId::CASTLE_WALL, ViewLayer::FLOOR)
            .setModifier(ViewObject::Modifier::CASTS_SHADOW), CONSTRUCT(Square::Params, c.name = "wall";));
    case SquareId::MUD_WALL:
        return new Square(ViewObject(ViewId::MUD_WALL, ViewLayer::FLOOR)
            .setModifier(ViewObject::Modifier::CASTS_SHADOW), CONSTRUCT(Square::Params, c.name = "wall";));
    case SquareId::MOUNTAIN:
        return new Square(ViewObject(ViewId::MOUNTAIN, ViewLayer::FLOOR)
            .setModifier(ViewObject::Modifier::CASTS_SHADOW),
          CONSTRUCT(Square::Params,
            c.name = "mountain";
            c.movementSet->setCovered(true);
            c.constructions = ConstructionsId::MOUNTAIN_GEN_ORES;
            ));
    case SquareId::HILL:
        return new Square(ViewObject(ViewId::HILL, ViewLayer::FLOOR_BACKGROUND),
          CONSTRUCT(Square::Params,
            c.name = "hill";
            c.vision = VisionId::NORMAL;
            c.constructions = ConstructionsId::OUTDOOR_INSTALLATIONS;
            c.movementSet = MovementSet().addTrait(MovementTrait::WALK);));
    case SquareId::WATER:
        return new Water(ViewObject(ViewId::WATER, ViewLayer::FLOOR_BACKGROUND), "water", 100);
    case SquareId::WATER_WITH_DEPTH:
        return new Water(ViewObject(ViewId::WATER, ViewLayer::FLOOR_BACKGROUND), "water", s.get<double>());
    case SquareId::MAGMA: 
        return new Magma(ViewObject(ViewId::MAGMA, ViewLayer::FLOOR), "magma");
    case SquareId::ABYSS: 
        FAIL << "Unimplemented";
    case SquareId::SAND:
        return new Square(ViewObject(ViewId::SAND, ViewLayer::FLOOR_BACKGROUND),
          CONSTRUCT(Square::Params,
            c.name = "sand";
            c.movementSet = MovementSet().addTrait(MovementTrait::WALK);
            c.vision = VisionId::NORMAL;));
    case SquareId::CANIF_TREE:
        return new Tree(ViewObject(ViewId::CANIF_TREE, ViewLayer::FLOOR)
                .setModifier(ViewObject::Modifier::ROUND_SHADOW), "tree", VisionId::ELF,
            Random.get(25, 40), s.get<CreatureId>());
    case SquareId::DECID_TREE:
        return new Tree(ViewObject(ViewId::DECID_TREE, ViewLayer::FLOOR)
                .setModifier(ViewObject::Modifier::ROUND_SHADOW),
            "tree", VisionId::ELF, Random.get(25, 40), s.get<CreatureId>());
    case SquareId::BUSH:
        return new Tree(ViewObject(ViewId::BUSH, ViewLayer::FLOOR), "bush",
            VisionId::NORMAL, Random.get(5, 10), s.get<CreatureId>());
    case SquareId::TREE_TRUNK:
        return new Square(ViewObject(ViewId::TREE_TRUNK, ViewLayer::FLOOR),
          CONSTRUCT(Square::Params,
            c.name = "tree trunk";
            c.movementSet = MovementSet().addTrait(MovementTrait::WALK);
            c.vision = VisionId::NORMAL;));
    case SquareId::BURNT_TREE:
        return new Square(ViewObject(ViewId::BURNT_TREE, ViewLayer::FLOOR),
          CONSTRUCT(Square::Params,
            c.name = "burnt tree";
            c.movementSet = MovementSet().addTrait(MovementTrait::WALK);
            c.vision = VisionId::NORMAL;));
    case SquareId::BED:
        return new Bed(ViewObject(ViewId::BED, ViewLayer::FLOOR), "bed");
    case SquareId::DORM:
        return new Square(ViewObject(ViewId::DORM, ViewLayer::FLOOR_BACKGROUND),
          CONSTRUCT(Square::Params,
            c.name = "floor";
            c.movementSet = MovementSet().addTrait(MovementTrait::WALK);
            c.canDestroy = true;
            c.constructions = ConstructionsId::BED;
            c.vision = VisionId::NORMAL;));
    case SquareId::TORCH: return new Torch(ViewObject(ViewId::TORCH, ViewLayer::FLOOR), "torch");
    case SquareId::STOCKPILE:
        return new Square(ViewObject(ViewId::STOCKPILE1, ViewLayer::FLOOR_BACKGROUND),
          CONSTRUCT(Square::Params,
            c.name = "floor";
            c.canDestroy = true;
            c.movementSet = MovementSet().addTrait(MovementTrait::WALK);
            c.vision = VisionId::NORMAL;));
    case SquareId::STOCKPILE_EQUIP:
        return new Square(ViewObject(ViewId::STOCKPILE2, ViewLayer::FLOOR_BACKGROUND),
          CONSTRUCT(Square::Params,
            c.name = "floor";
            c.canDestroy = true;
            c.movementSet = MovementSet().addTrait(MovementTrait::WALK);
            c.vision = VisionId::NORMAL;));
    case SquareId::STOCKPILE_RES:
        return new Square(ViewObject(ViewId::STOCKPILE3, ViewLayer::FLOOR_BACKGROUND),
          CONSTRUCT(Square::Params,
            c.name = "floor";
            c.canDestroy = true;
            c.movementSet = MovementSet().addTrait(MovementTrait::WALK);
            c.vision = VisionId::NORMAL;));
    case SquareId::PRISON:
        return new Square(ViewObject(ViewId::PRISON, ViewLayer::FLOOR_BACKGROUND),
          CONSTRUCT(Square::Params,
            c.name = "floor";
            c.canDestroy = true;
            c.movementSet = MovementSet().addTrait(MovementTrait::WALK);
            c.vision = VisionId::NORMAL;));
    case SquareId::WELL:
        return new Furniture(ViewObject(ViewId::WELL, ViewLayer::FLOOR), "well", 0);
    case SquareId::TORTURE_TABLE:
        return new Furniture(ViewObject(ViewId::TORTURE_TABLE, ViewLayer::FLOOR), 
            "torture room", 0.3, SquareApplyType::TORTURE);
    case SquareId::BEAST_CAGE:
        return new Bed(ViewObject(ViewId::BEAST_CAGE, ViewLayer::FLOOR), "beast cage");
    case SquareId::BEAST_LAIR:
        return new Square(ViewObject(ViewId::BEAST_LAIR, ViewLayer::FLOOR_BACKGROUND, "Beast lair"),
          CONSTRUCT(Square::Params,
            c.name = "floor";
            c.canDestroy = true;
            c.constructions = ConstructionsId::BEAST_CAGE;
            c.movementSet = MovementSet().addTrait(MovementTrait::WALK);
            c.vision = VisionId::NORMAL;));
    case SquareId::TRAINING_ROOM:
        return new Furniture(ViewObject(ViewId::TRAINING_ROOM, ViewLayer::FLOOR), 
            "training room", 1, SquareApplyType::TRAIN, SoundId::MISSED_ATTACK);
    case SquareId::THRONE:
        return new Furniture(ViewObject(ViewId::THRONE, ViewLayer::FLOOR), 
            "throne", 0, SquareApplyType::THRONE);
    case SquareId::WHIPPING_POST:
        return new Furniture(ViewObject(ViewId::WHIPPING_POST, ViewLayer::FLOOR), 
            "whipping post", 0, SquareApplyType::WHIPPING);
    case SquareId::NOTICE_BOARD:
        return new NoticeBoard(ViewObject(ViewId::NOTICE_BOARD, ViewLayer::FLOOR), 
            s.get<string>());
    case SquareId::KEEPER_BOARD:
        return new Square(ViewObject(ViewId::NOTICE_BOARD, ViewLayer::FLOOR),
            CONSTRUCT(Square::Params,
              c.name = "message board";
              c.canDestroy = true;
              c.movementSet = MovementSet().addTrait(MovementTrait::WALK);
              c.vision = VisionId::NORMAL;
              c.interaction = SquareInteraction::KEEPER_BOARD;));

    case SquareId::SOKOBAN_HOLE:
        return new SokobanHole(ViewObject(ViewId::SOKOBAN_HOLE, ViewLayer::FLOOR), "hole",
            s.get<StairKey>());
    case SquareId::RITUAL_ROOM:
        return new Square(ViewObject(ViewId::RITUAL_ROOM, ViewLayer::FLOOR),
          CONSTRUCT(Square::Params,
            c.name = "ritual room";
            c.canDestroy = true;
            c.movementSet = MovementSet().addTrait(MovementTrait::WALK);
            c.vision = VisionId::NORMAL;));
    case SquareId::IMPALED_HEAD:
        return new Square(ViewObject(ViewId::IMPALED_HEAD, ViewLayer::FLOOR)
            .setModifier(ViewObject::Modifier::ROUND_SHADOW),
          CONSTRUCT(Square::Params,
            c.name = "impaled head";
            c.canDestroy = true;
            c.movementSet = MovementSet().addTrait(MovementTrait::WALK);
            c.vision = VisionId::NORMAL;));
    case SquareId::EYEBALL:
        return new Square(ViewObject(ViewId::EYEBALL, ViewLayer::FLOOR)
            .setModifier(ViewObject::Modifier::ROUND_SHADOW),
          CONSTRUCT(Square::Params,
            c.name = "eyeball";
            c.canDestroy = true;
            c.movementSet = MovementSet().addTrait(MovementTrait::WALK);
            c.vision = VisionId::NORMAL;));
    case SquareId::LIBRARY:
        return new Furniture(ViewObject(ViewId::LIBRARY, ViewLayer::FLOOR), "library", 1, SquareApplyType::TRAIN);
    case SquareId::CAULDRON:
        return new Laboratory(ViewObject(ViewId::CAULDRON, ViewLayer::FLOOR), "cauldron");
    case SquareId::LABORATORY:
      return new Laboratory(ViewObject(ViewId::LABORATORY, ViewLayer::FLOOR), "laboratory");
    case SquareId::FORGE:
        return new Furniture(ViewObject(ViewId::FORGE, ViewLayer::FLOOR), "forge", 1,
            SquareApplyType::WORKSHOP);
    case SquareId::WORKSHOP:
      return new Furniture(ViewObject(ViewId::WORKSHOP, ViewLayer::FLOOR), "workshop stand", 1,
            SquareApplyType::WORKSHOP);
    case SquareId::JEWELER:
      return new Furniture(ViewObject(ViewId::JEWELER, ViewLayer::FLOOR), "jeweler stand", 1,
            SquareApplyType::WORKSHOP);
    case SquareId::MINION_STATUE:
      return new Furniture(ViewObject(ViewId::MINION_STATUE, ViewLayer::FLOOR),"statue", 0,
            SquareApplyType::STATUE);
    case SquareId::HATCHERY:
        return new Hatchery(ViewObject(ViewId::MUD, ViewLayer::FLOOR_BACKGROUND, "Pigsty"), "pigsty",
            s.get<CreatureFactory::SingleCreature>());
    case SquareId::CREATURE_ALTAR:
        return new CreatureAltar(ViewObject(ViewId::CREATURE_ALTAR, ViewLayer::FLOOR),
              s.get<const Creature*>());
    case SquareId::ROLLING_BOULDER:
        return new TrapSquare(ViewObject(ViewId::FLOOR, ViewLayer::FLOOR), EffectId::ROLLING_BOULDER);
    case SquareId::POISON_GAS:
        return new TrapSquare(ViewObject(ViewId::FLOOR, ViewLayer::FLOOR), EffectId::EMIT_POISON_GAS);
    case SquareId::FOUNTAIN:
        return new Fountain(ViewObject(ViewId::FOUNTAIN, ViewLayer::FLOOR));
    case SquareId::CHEST:
        return new Chest(ViewObject(ViewId::CHEST, ViewLayer::FLOOR),"chest", s.get<ChestInfo>(),
            SquareId::OPENED_CHEST, "There is an item inside", "It's full of rats!", "There is gold inside",
            ItemFactory::chest());
    case SquareId::OPENED_CHEST:
        return new Square(ViewObject(ViewId::OPENED_CHEST, ViewLayer::FLOOR),
          CONSTRUCT(Square::Params,
            c.name = "opened chest";
            c.movementSet = MovementSet().addTrait(MovementTrait::WALK);
            c.vision = VisionId::NORMAL;));
    case SquareId::TREASURE_CHEST:
        return new Furniture(ViewObject(ViewId::TREASURE_CHEST, ViewLayer::FLOOR), "chest", 1);
    case SquareId::COFFIN:
        return new Chest(ViewObject(ViewId::COFFIN, ViewLayer::FLOOR), "coffin", s.get<ChestInfo>(),
            SquareId::OPENED_COFFIN, "There is a rotting corpse inside. You find an item.",
            "There is a rotting corpse inside. The corpse is alive!",
            "There is a rotting corpse inside. You find some gold.", ItemFactory::chest());
    case SquareId::OPENED_COFFIN:
        return new Square(ViewObject(ViewId::OPENED_COFFIN, ViewLayer::FLOOR),
          CONSTRUCT(Square::Params,
            c.name = "opened coffin";
            c.movementSet = MovementSet().addTrait(MovementTrait::WALK);
            c.vision = VisionId::NORMAL;));
    case SquareId::CEMETERY:
        return new Square(ViewObject(ViewId::CEMETERY, ViewLayer::FLOOR_BACKGROUND),
          CONSTRUCT(Square::Params,
            c.name = "floor";
            c.canDestroy = true;
            c.constructions = ConstructionsId::GRAVE;
            c.movementSet = MovementSet().addTrait(MovementTrait::WALK);
            c.vision = VisionId::NORMAL;));
    case SquareId::GRAVE:
        return new Grave(ViewObject(ViewId::GRAVE, ViewLayer::FLOOR), "grave");
    case SquareId::DOOR:
        return new Door(ViewObject(ViewId::DOOR, ViewLayer::FLOOR).setModifier(ViewObject::Modifier::CASTS_SHADOW),
          CONSTRUCT(Square::Params,
            c.name = "door";
            c.canHide = true;
            c.movementSet = MovementSet().addTrait(MovementTrait::WALK);
            c.strength = 100;
            c.canDestroy = true;
            c.flamability = 1;));
    case SquareId::TRIBE_DOOR:
        return new TribeDoor(ViewObject(ViewId::DOOR, ViewLayer::FLOOR, "Door. Click to lock.")
            .setModifier(ViewObject::Modifier::CASTS_SHADOW), s.get<TribeId>(), 100,
          CONSTRUCT(Square::Params,
            c.name = "door";
            c.canHide = true;
            c.strength = 100;
            c.flamability = 1;
            c.canDestroy = true;
            c.owner = s.get<TribeId>();));
    case SquareId::BARRICADE:
        return new Barricade(ViewObject(ViewId::BARRICADE, ViewLayer::FLOOR)
            .setModifier(ViewObject::Modifier::ROUND_SHADOW), s.get<TribeId>(), 200);
    case SquareId::BORDER_GUARD:
        return new Square(ViewObject(ViewId::BORDER_GUARD, ViewLayer::FLOOR),
          CONSTRUCT(Square::Params, c.name = "wall";));
    case SquareId::STAIRS:
        return getStairs(s.get<StairInfo>());
  }
  return 0;
}

SquareFactory::SquareFactory(const vector<SquareType>& s, const vector<double>& w) : squares(s), weights(w) {
}

SquareFactory::SquareFactory(const vector<SquareType>& f, const vector<SquareType>& s, const vector<double>& w)
    : first(f), squares(s), weights(w) {
}

SquareFactory SquareFactory::roomFurniture(TribeId rats) {
  return SquareFactory({SquareId::BED, SquareId::TORCH,
      {SquareId::CHEST, ChestInfo(CreatureFactory::SingleCreature(rats, CreatureId::RAT), 0.2, 5)}},
      {2, 1, 2});
}

SquareFactory SquareFactory::castleFurniture(TribeId rats) {
  return SquareFactory({SquareId::BED, SquareId::FOUNTAIN, SquareId::TORCH,
      {SquareId::CHEST, ChestInfo(CreatureFactory::SingleCreature(rats, CreatureId::RAT), 0.2, 5)}},
      {2, 1, 1, 2});
}

SquareFactory SquareFactory::dungeonOutside() {
  return SquareFactory({SquareId::TORCH}, {1});
}

SquareFactory SquareFactory::castleOutside() {
  return SquareFactory({{SquareId::WELL}}, {SquareId::TORCH}, {1});
}

SquareFactory SquareFactory::villageOutside(const string& boardText) {
  if (!boardText.empty())
    return SquareFactory({{SquareId::NOTICE_BOARD, boardText}, {SquareId::WELL}}, {SquareId::TORCH}, {1});
  else
    return SquareFactory({{SquareId::WELL}}, {SquareId::TORCH}, {1});
}

SquareFactory SquareFactory::cryptCoffins(TribeId vampire) {
  return SquareFactory(
      {{SquareId::COFFIN, ChestInfo(CreatureFactory::SingleCreature(vampire, CreatureId::VAMPIRE_LORD), 1, 1)}},
      {{SquareId::COFFIN, ChestInfo()}}, {1});
}

SquareFactory SquareFactory::single(SquareType type) {
  return SquareFactory({type}, {1});
}

SquareType SquareFactory::getRandom(RandomGen& random) {
  if (!first.empty()) {
    SquareType ret = first.back();
    first.pop_back();
    return ret;
  }
  return random.choose(squares, weights);
}
