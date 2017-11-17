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

#include "controller.h"
#include "map_memory.h"
#include "creature.h"
#include "player_message.h"
#include "message_generator.h"

SERIALIZE_DEF(DoNothingController, SUBCLASS(Controller))
SERIALIZATION_CONSTRUCTOR_IMPL(DoNothingController);

SERIALIZE_DEF(Controller, SUBCLASS(OwnedObject<Controller>), creature)
SERIALIZATION_CONSTRUCTOR_IMPL(Controller);

Controller::Controller(WCreature c) : creature(c) {
}

ControllerFactory::ControllerFactory(function<PController(WCreature)> f) : fun(f) {}

PController ControllerFactory::get(WCreature c) const {
  return fun(c);
}

DoNothingController::DoNothingController(WCreature c) : Controller(c) {}

bool DoNothingController::isPlayer() const {
  return false;
}

void DoNothingController::makeMove() {
  getCreature()->wait().perform(getCreature());
}

void DoNothingController::onBump(WCreature c) {
}


MessageGenerator& DoNothingController::getMessageGenerator() const {
  static MessageGenerator messageGenerator(MessageGenerator::NONE);
  return messageGenerator;
}

WCreature Controller::getCreature() const {
  return creature;
}


