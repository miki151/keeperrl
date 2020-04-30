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
#include "game.h"

bool MinionActivityMap::canChooseRandomly(const Creature* c, MinionActivity t) const {
  PROFILE;
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

bool MinionActivityMap::isAvailable(const Collective* col, const Creature* c, MinionActivity t, bool ignoreTaskLock) const {
  if ((locked.contains(t) || col->getGroupLockedActivities(c).contains(t)) && !ignoreTaskLock)
    return false;
  switch (t) {
    case MinionActivity::IDLE:
      return true;
    case MinionActivity::TRAIN:
      return !c->getAttributes().isTrainingMaxedOut(ExperienceType::MELEE) &&
          !col->hasTrait(c, MinionTrait::PRISONER);
    case MinionActivity::STUDY:
      return !col->hasTrait(c, MinionTrait::PRISONER) &&
          !c->getAttributes().isTrainingMaxedOut(ExperienceType::SPELL);
    case MinionActivity::ARCHERY:
      return !col->hasTrait(c, MinionTrait::PRISONER) &&
          !c->getAttributes().isTrainingMaxedOut(ExperienceType::ARCHERY) &&
          !c->getEquipment().getItems(ItemIndex::RANGED_WEAPON).empty();
    case MinionActivity::BE_WHIPPED:
      return !c->getBody().isImmuneTo(LastingEffect::ENTANGLED) &&
          !c->getBody().isMinionFood() &&
          c->getBody().isHumanoid();
    case MinionActivity::POETRY:
      return col->hasTrait(c, MinionTrait::LEADER);
    case MinionActivity::BE_EXECUTED:
    case MinionActivity::BE_TORTURED:
      return col->hasTrait(c, MinionTrait::PRISONER);
    case MinionActivity::CRAFT:
      return !c->getAttributes().getSkills().getWorkshopValues().empty();
    case MinionActivity::SLEEP:
      return c->getBody().needsToSleep() && !c->isAffected(LastingEffect::POISON);
    case MinionActivity::EAT:
      return c->getBody().needsToEat() && !col->hasTrait(c, MinionTrait::PRISONER);
    case MinionActivity::COPULATE:
      return c->isAffected(LastingEffect::COPULATION_SKILL);
    case MinionActivity::RITUAL:
      return c->getBody().canPerformRituals() && !col->hasTrait(c, MinionTrait::WORKER);
    case MinionActivity::GUARDING:
      return col->hasTrait(c, MinionTrait::FIGHTER);
    case MinionActivity::CROPS:
      return c->isAffected(LastingEffect::CROPS_SKILL);
    case MinionActivity::SPIDER:
      return c->isAffected(LastingEffect::SPIDER_SKILL);
    case MinionActivity::EXPLORE_CAVES:
      return c->isAffected(LastingEffect::EXPLORE_CAVES_SKILL);
    case MinionActivity::EXPLORE:
      return c->isAffected(LastingEffect::EXPLORE_SKILL);
    case MinionActivity::EXPLORE_NOCTURNAL:
      return c->isAffected(LastingEffect::EXPLORE_NOCTURNAL_SKILL);
    case MinionActivity::CONSTRUCTION:
    case MinionActivity::WORKING:
    case MinionActivity::HAULING:
      return c->getBody().canPickUpItems() && col->hasTrait(c, MinionTrait::WORKER);
    case MinionActivity::DIGGING:
      return c->getAttributes().getSkills().getValue(SkillId::DIGGING) > 0 && col->hasTrait(c, MinionTrait::WORKER);
    case MinionActivity::MINION_ABUSE:
      return col->hasTrait(c, MinionTrait::LEADER) && col == col->getGame()->getPlayerCollective();
    case MinionActivity::DISTILLATION:
      return c->isAffected(LastingEffect::DISTILLATION_SKILL);
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

#include "pretty_archive.h"
template<>
void MinionActivityMap::serialize(PrettyInputArchive&, unsigned) {
}
