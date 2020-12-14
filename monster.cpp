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

#include "monster.h"
#include "map_memory.h"
#include "creature.h"
#include "player_message.h"
#include "creature_name.h"
#include "gender.h"
#include "monster_ai.h"
#include "creature_attributes.h"
#include "message_generator.h"
#include "task.h"

SERIALIZE_DEF(Monster, SUBCLASS(Controller), monsterAI, dragTask)
SERIALIZATION_CONSTRUCTOR_IMPL(Monster);

Monster::Monster(Creature* c, const MonsterAIFactory& f)
    : Controller(c), monsterAI(f.getMonsterAI(c)) {}

ControllerFactory Monster::getFactory(MonsterAIFactory f) {
  return ControllerFactory([=](Creature* c) { return makeOwner<Monster>(c, f);});
}

void Monster::makeMove() {
  if (dragTask && dragTask->isDone())
    dragTask.clear();
  if (dragTask)
    if (auto move = dragTask->getMove(creature)) {
      move.getMove().perform(creature);
      return;
    }
  monsterAI->makeMove();
}

bool Monster::isPlayer() const {
  return false;
}

const MapMemory& Monster::getMemory() const {
  return MapMemory::empty();
}

MessageGenerator& Monster::getMessageGenerator() const {
  static MessageGenerator messageGenerator(MessageGenerator::THIRD_PERSON);
  return messageGenerator;
}

void Monster::setDragTask(PTask task) {
  dragTask = std::move(task);
}

Task* Monster::getDragTask() const {
  return dragTask.get();
}
