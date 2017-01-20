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
#include "tribe.h"
#include "creature_name.h"
#include "modifier_type.h"
#include "movement_set.h"
#include "movement_type.h"
#include "stair_key.h"
#include "view.h"
#include "sound.h"
#include "creature_attributes.h"
#include "game.h"
#include "event_listener.h"
#include "square_type.h"
#include "item_type.h"
#include "body.h"
#include "item.h"

class Magma : public Square {
  public:
  Magma(const ViewObject& object, const string& name) : Square(object,
      CONSTRUCT(Square::Params,
        c.name = name;
        c.vision = VisionId::NORMAL;
        c.movementSet = MovementSet()
            .addTrait(MovementTrait::FLY)
            .addForcibleTrait(MovementTrait::WALK);)) {}

  virtual void onEnterSpecial(Creature* c) override {
    if (c->getPosition().getFurniture().empty()) { // check if there is a bridge
      MovementType realMovement = c->getMovementType();
      realMovement.setForced(false);
      if (!c->getPosition().canEnterEmpty(realMovement)) {
        c->you(MsgType::BURN, getName());
        c->die("burned to death", false);
      }
    }
  }

  virtual void dropItems(Position position, vector<PItem> items) override {
    if (position.getFurniture().empty()) { // check if there is a bridge
      for (auto elem : Item::stackItems(extractRefs(items)))
        position.globalMessage(elem.second[0]->getPluralTheNameAndVerb(elem.second.size(),
              "burns", "burn") + " in the magma.");
    } else
      Square::dropItems(position, std::move(items));
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
            c.movementSet = getMovement(_depth);
          )) {}

  virtual void onEnterSpecial(Creature* c) override {
    if (c->getPosition().getFurniture().empty()) { // check if there is a bridge
      MovementType realMovement = c->getMovementType();
      realMovement.setForced(false);
      if (!c->getPosition().canEnterEmpty(realMovement)) {
        c->you(MsgType::DROWN, getName());
        c->die("drowned", false);
      }
    }
  }

  virtual void dropItems(Position position, vector<PItem> items) override {
    if (position.getFurniture().empty()) { // check if there is a bridge
      for (auto elem : Item::stackItems(extractRefs(items)))
        position.globalMessage(elem.second[0]->getPluralTheNameAndVerb(elem.second.size(),
              "sinks", "sink") + " in the water.", "You hear a splash.");
    } else
      Square::dropItems(position, std::move(items));
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

template <class Archive>
void SquareFactory::registerTypes(Archive& ar, int version) {
  REGISTER_TYPE(ar, Magma);
  REGISTER_TYPE(ar, Water);
}

REGISTER_TYPES(SquareFactory::registerTypes);

PSquare SquareFactory::get(SquareType s) {
  return PSquare(getPtr(s));
}
 
Square* SquareFactory::getPtr(SquareType s) {
  switch (s.getId()) {
    case SquareId::FLOOR:
        return new Square(ViewObject(ViewId::FLOOR, ViewLayer::FLOOR_BACKGROUND),
            CONSTRUCT(Square::Params,
              c.name = "floor";
              c.vision = VisionId::NORMAL;
              c.movementSet = MovementSet().addTrait(MovementTrait::WALK);
            ));
    case SquareId::BLACK_FLOOR:
        return new Square(ViewObject(ViewId::EMPTY, ViewLayer::FLOOR_BACKGROUND, "Floor"),
            CONSTRUCT(Square::Params,
              c.name = "floor";
              c.vision = VisionId::NORMAL;
              c.movementSet = MovementSet().addTrait(MovementTrait::WALK);));
    case SquareId::GRASS:
        return new Square(ViewObject(ViewId::GRASS, ViewLayer::FLOOR_BACKGROUND),
            CONSTRUCT(Square::Params,
              c.name = "grass";
              c.movementSet = MovementSet().addTrait(MovementTrait::WALK);
              c.vision = VisionId::NORMAL;
            ));
    case SquareId::MUD:
        return new Square(ViewObject(ViewId::MUD, ViewLayer::FLOOR_BACKGROUND),
            CONSTRUCT(Square::Params,
              c.name = "mud";
              c.vision = VisionId::NORMAL;
              c.movementSet = MovementSet().addTrait(MovementTrait::WALK);));
    case SquareId::BLACK_WALL:
        return new Square(ViewObject(ViewId::EMPTY, ViewLayer::FLOOR, "Wall"),
            CONSTRUCT(Square::Params, c.name = "wall";));
    case SquareId::HILL:
        return new Square(ViewObject(ViewId::HILL, ViewLayer::FLOOR_BACKGROUND),
          CONSTRUCT(Square::Params,
            c.name = "hill";
            c.vision = VisionId::NORMAL;
            c.movementSet = MovementSet().addTrait(MovementTrait::WALK);));
    case SquareId::WATER:
        return new Water(ViewObject(ViewId::WATER, ViewLayer::FLOOR_BACKGROUND), "water", 100);
    case SquareId::WATER_WITH_DEPTH:
        return new Water(ViewObject(ViewId::WATER, ViewLayer::FLOOR_BACKGROUND), "water", s.get<double>());
    case SquareId::MAGMA: 
        return new Magma(ViewObject(ViewId::MAGMA, ViewLayer::FLOOR), "magma");
    case SquareId::SAND:
        return new Square(ViewObject(ViewId::SAND, ViewLayer::FLOOR_BACKGROUND),
          CONSTRUCT(Square::Params,
            c.name = "sand";
            c.movementSet = MovementSet().addTrait(MovementTrait::WALK);
            c.vision = VisionId::NORMAL;));
    case SquareId::BORDER_GUARD:
        return new Square(ViewObject(ViewId::BORDER_GUARD, ViewLayer::FLOOR),
          CONSTRUCT(Square::Params, c.name = "wall";));
  }
  return 0;
}

SquareFactory::SquareFactory(const vector<SquareType>& s, const vector<double>& w) : squares(s), weights(w) {
}

SquareFactory::SquareFactory(const vector<SquareType>& f, const vector<SquareType>& s, const vector<double>& w)
    : first(f), squares(s), weights(w) {
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
