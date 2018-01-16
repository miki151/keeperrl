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
#include "minion_activity_map.h"
#include "collective_config.h"
#include "creature.h"
#include "creature_attributes.h"
#include "body.h"
#include "collective.h"
#include "equipment.h"

bool MinionActivityMap::canChooseRandomly(WConstCreature c, MinionActivity t) const {
  switch (t) {
    case MinionActivity::BE_EXECUTED:
    case MinionActivity::BE_WHIPPED:
    case MinionActivity::BE_TORTURED:
      return false;
    case MinionActivity::SLEEP:
      return !c->isAffected(LastingEffect::RESTED);
    case MinionActivity::EAT:
      return !c->isAffected(LastingEffect::SATIATED);
    default:
      return true;
  }
}

static bool canLock(MinionActivity t) {
  switch (t) {
    case MinionActivity::IDLE:
      return false;
    default:
      return true;
  }
}

bool MinionActivityMap::isAvailable(WConstCollective col, WConstCreature c, MinionActivity t, bool ignoreTaskLock) const {
  if (locked.contains(t) && !ignoreTaskLock)
    return false;
  switch (t) {
    case MinionActivity::IDLE:
      return true;
    case MinionActivity::TRAIN:
      return !c->getAttributes().isTrainingMaxedOut(ExperienceType::MELEE);
    case MinionActivity::STUDY:
      return !c->getAttributes().isTrainingMaxedOut(ExperienceType::SPELL) ||
          (col->getConfig().getRegenerateMana() && c->getAttributes().getMaxExpLevel()[ExperienceType::SPELL] > 0);
    case MinionActivity::ARCHERY:
      return !c->getAttributes().isTrainingMaxedOut(ExperienceType::ARCHERY) &&
          !c->getEquipment().getItems(ItemIndex::RANGED_WEAPON).empty();
    case MinionActivity::BE_WHIPPED:
      return !c->getBody().isImmuneTo(LastingEffect::ENTANGLED) &&
          !c->getBody().isMinionFood() &&
          c->getBody().isHumanoid();
    case MinionActivity::THRONE:
      return col->getLeader() == c;
    case MinionActivity::BE_EXECUTED:
    case MinionActivity::BE_TORTURED:
      return col->hasTrait(c, MinionTrait::PRISONER);
    case MinionActivity::CRAFT:
      return c->getAttributes().getSkills().getValue(SkillId::FORGE) > 0 ||
          c->getAttributes().getSkills().getValue(SkillId::WORKSHOP) > 0 ||
          c->getAttributes().getSkills().getValue(SkillId::FURNACE) > 0 ||
          c->getAttributes().getSkills().getValue(SkillId::JEWELER) > 0 ||
          c->getAttributes().getSkills().getValue(SkillId::LABORATORY) > 0;
    case MinionActivity::SLEEP:
      return c->getBody().needsToSleep();
    case MinionActivity::EAT:
      return c->getBody().needsToEat();
    case MinionActivity::COPULATE:
      return c->getAttributes().getSkills().hasDiscrete(SkillId::COPULATION);
    case MinionActivity::RITUAL:
      return c->getBody().isHumanoid();
    case MinionActivity::CROPS:
      return c->getAttributes().getSkills().hasDiscrete(SkillId::CROPS);
    case MinionActivity::SPIDER:
      return c->getAttributes().getSkills().hasDiscrete(SkillId::SPIDER);
    case MinionActivity::EXPLORE_CAVES:
      return c->getAttributes().getSkills().hasDiscrete(SkillId::EXPLORE_CAVES);
    case MinionActivity::EXPLORE:
      return c->getAttributes().getSkills().hasDiscrete(SkillId::EXPLORE);
    case MinionActivity::EXPLORE_NOCTURNAL:
      return c->getAttributes().getSkills().hasDiscrete(SkillId::EXPLORE_NOCTURNAL);
    case MinionActivity::CONSTRUCTION:
      return c->getAttributes().getSkills().hasDiscrete(SkillId::CONSTRUCTION);
    case MinionActivity::WORKING:
      return c->getBody().isHumanoid() && col->hasTrait(c, MinionTrait::WORKER);
    case MinionActivity::HAULING:
      return !c->getBody().getCarryLimit() && col->hasTrait(c, MinionTrait::WORKER);
    case MinionActivity::DIGGING:
      return c->getAttributes().getSkills().getValue(SkillId::DIGGING) > 0 && col->hasTrait(c, MinionTrait::WORKER);
  }
}

void MinionActivityMap::toggleLock(MinionActivity task) {
  if (canLock(task))
    locked.toggle(task);
}

optional<bool> MinionActivityMap::isLocked(MinionActivity task) const {
  if (canLock(task))
    return locked.contains(task);
  else
    return none;
}

SERIALIZE_DEF(MinionActivityMap, locked);
