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
#include "minion_task_map.h"
#include "collective_config.h"
#include "creature.h"
#include "creature_attributes.h"
#include "body.h"
#include "collective.h"
#include "equipment.h"

bool MinionTaskMap::canChooseRandomly(WConstCreature c, MinionTask t) const {
  switch (t) {
    case MinionTask::BE_EXECUTED:
    case MinionTask::BE_WHIPPED:
    case MinionTask::BE_TORTURED:
      return false;
    case MinionTask::SLEEP:
      return !c->isAffected(LastingEffect::RESTED);
    case MinionTask::EAT:
      return !c->isAffected(LastingEffect::SATIATED);
    default:
      return true;
  }
}

static bool canLock(MinionTask t) {
  switch (t) {
    case MinionTask::IDLE:
      return false;
    default:
      return true;
  }
}

bool MinionTaskMap::isAvailable(WConstCollective col, WConstCreature c, MinionTask t, bool ignoreTaskLock) const {
  if (locked.contains(t) && !ignoreTaskLock)
    return false;
  switch (t) {
    case MinionTask::IDLE:
      return true;
    case MinionTask::TRAIN:
      return !c->getAttributes().isTrainingMaxedOut(ExperienceType::MELEE);
    case MinionTask::STUDY:
      return !c->getAttributes().isTrainingMaxedOut(ExperienceType::SPELL) ||
          (col->getConfig().getRegenerateMana() && c->getAttributes().getMaxExpLevel()[ExperienceType::SPELL] > 0);
    case MinionTask::ARCHERY:
      return !c->getAttributes().isTrainingMaxedOut(ExperienceType::ARCHERY) &&
          !c->getEquipment().getItems(ItemIndex::RANGED_WEAPON).empty();
    case MinionTask::BE_WHIPPED:
      return !c->getBody().isImmuneTo(LastingEffect::ENTANGLED) &&
          !c->getBody().isMinionFood() &&
          c->getBody().isHumanoid();
    case MinionTask::THRONE:
      return col->getLeader() == c;
    case MinionTask::BE_EXECUTED:
    case MinionTask::BE_TORTURED:
      return col->hasTrait(c, MinionTrait::PRISONER);
    case MinionTask::CRAFT:
      return c->getAttributes().getSkills().getValue(SkillId::FORGE) > 0 ||
          c->getAttributes().getSkills().getValue(SkillId::WORKSHOP) > 0 ||
          c->getAttributes().getSkills().getValue(SkillId::FURNACE) > 0 ||
          c->getAttributes().getSkills().getValue(SkillId::JEWELER) > 0 ||
          c->getAttributes().getSkills().getValue(SkillId::LABORATORY) > 0;
    case MinionTask::SLEEP:
      return c->getBody().needsToSleep();
    case MinionTask::EAT:
      return c->getBody().needsToEat();
    case MinionTask::COPULATE:
      return c->getAttributes().getSkills().hasDiscrete(SkillId::COPULATION);
    case MinionTask::RITUAL:
      return c->getBody().isHumanoid();
    case MinionTask::CROPS:
      return c->getAttributes().getSkills().hasDiscrete(SkillId::CROPS);
    case MinionTask::SPIDER:
      return c->getAttributes().getSkills().hasDiscrete(SkillId::SPIDER);
    case MinionTask::EXPLORE_CAVES:
      return c->getAttributes().getSkills().hasDiscrete(SkillId::EXPLORE_CAVES);
    case MinionTask::EXPLORE:
      return c->getAttributes().getSkills().hasDiscrete(SkillId::EXPLORE);
    case MinionTask::EXPLORE_NOCTURNAL:
      return c->getAttributes().getSkills().hasDiscrete(SkillId::EXPLORE_NOCTURNAL);
    case MinionTask::WORKER:
      return c->getAttributes().getSkills().hasDiscrete(SkillId::CONSTRUCTION) ||
          col->hasTrait(c, MinionTrait::PRISONER);
  }
}

void MinionTaskMap::toggleLock(MinionTask task) {
  if (canLock(task))
    locked.toggle(task);
}

optional<bool> MinionTaskMap::isLocked(MinionTask task) const {
  if (canLock(task))
    return locked.contains(task);
  else
    return none;
}

SERIALIZE_DEF(MinionTaskMap, locked);
