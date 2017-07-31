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

bool MinionTaskMap::canChooseRandomly(WConstCreature c, MinionTask t) const {
  switch (t) {
    case MinionTask::BE_EXECUTED:
    case MinionTask::BE_WHIPPED:
    case MinionTask::BE_TORTURED:
      return false;
    case MinionTask::LAIR:
    case MinionTask::GRAVE:
    case MinionTask::SLEEP: {
      constexpr int sleepNeededAfterTurns = 1000;
      if (auto lastTime = c->getLastAffected(LastingEffect::SLEEP))
        return *lastTime < c->getGlobalTime() - sleepNeededAfterTurns;
      else
        return true;
    }
    default:
      return true;
  }
}

bool MinionTaskMap::isAvailable(WConstCollective col, WConstCreature c, MinionTask t, bool ignoreTaskLock) const {
  if (locked.contains(t) && !ignoreTaskLock)
    return false;
  switch (t) {
    case MinionTask::TRAIN:
      return !c->getAttributes().isTrainingMaxedOut(ExperienceType::MELEE);
    case MinionTask::STUDY:
      return !c->getAttributes().isTrainingMaxedOut(ExperienceType::SPELL);
    case MinionTask::BE_WHIPPED:
      return !c->getBody().isImmuneTo(LastingEffect::ENTANGLED) &&
          !c->getBody().isMinionFood() &&
          c->getBody().isHumanoid();
    case MinionTask::THRONE:
      return col->getLeader() == c;
    case MinionTask::BE_EXECUTED:
    case MinionTask::PRISON:
    case MinionTask::BE_TORTURED:
      return col->hasTrait(c, MinionTrait::PRISONER);
    case MinionTask::CRAFT:
      return c->getAttributes().getSkills().getValue(SkillId::FORGE) > 0 ||
          c->getAttributes().getSkills().getValue(SkillId::WORKSHOP) > 0 ||
          c->getAttributes().getSkills().getValue(SkillId::FURNACE) > 0 ||
          c->getAttributes().getSkills().getValue(SkillId::JEWELER) > 0 ||
          c->getAttributes().getSkills().getValue(SkillId::LABORATORY) > 0;
    case MinionTask::SLEEP:
      return c->getAttributes().getSpawnType() == SpawnType::HUMANOID ||
          (c->getBody().isHumanoid() && col->getConfig().hasVillainSleepingTask());
    case MinionTask::GRAVE:
      return c->getAttributes().getSpawnType() == SpawnType::UNDEAD;
    case MinionTask::LAIR:
      return c->getAttributes().getSpawnType() == SpawnType::BEAST;
    case MinionTask::EAT:
      return c->getBody().needsToEat();
    case MinionTask::COPULATE:
      return c->getAttributes().getSkills().hasDiscrete(SkillId::COPULATION);
    case MinionTask::RITUAL:
      return c->getAttributes().getSpawnType() == SpawnType::DEMON;
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
      return c->getAttributes().getSkills().hasDiscrete(SkillId::CONSTRUCTION);
  }
}

void MinionTaskMap::toggleLock(MinionTask task) {
  locked.toggle(task);
}

bool MinionTaskMap::isLocked(MinionTask task) const {
  return locked.contains(task);
}

SERIALIZE_DEF(MinionTaskMap, locked);
