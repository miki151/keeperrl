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
#include "content_factory.h"

bool MinionActivityMap::isActivityAutoGroupLocked(MinionActivity activity) {
  switch (activity) {
    case MinionActivity::GUARDING1:
    case MinionActivity::GUARDING2:
    case MinionActivity::GUARDING3:
      return true;
    default:
      return false;
  }
}

static bool isAutoLocked(const Collective* col, const Creature* c, MinionActivity t) {
  switch (t) {
    case MinionActivity::PHYLACTERY:
      return !col->hasTrait(c, MinionTrait::LEADER);
    default:
      return false;
  }
}

bool MinionActivityMap::canChooseRandomly(const Collective* collective, const Creature* c, MinionActivity t) const {
  PROFILE;
  switch (t) {
    case MinionActivity::BE_EXECUTED:
    case MinionActivity::BE_WHIPPED:
    case MinionActivity::BE_TORTURED:
      return false;
    case MinionActivity::PREACHING:
      return collective->lastMass < collective->getLocalTime() - 1500_visible;
    case MinionActivity::SLEEP:
      return !c->isAffected(LastingEffect::RESTED);
    case MinionActivity::EAT:
      return !c->isAffected(LastingEffect::SATIATED);
    default:
      return true;
  }
}

bool MinionActivityMap::canLock(MinionActivity t) {
  switch (t) {
    case MinionActivity::IDLE:
      return false;
    default:
      return true;
  }
}

bool MinionActivityMap::isAvailable(const Collective* col, const Creature* c, MinionActivity t, bool ignoreTaskLock) const {
  PROFILE;
  if ((isLocked(col, c, t) || col->isActivityGroupLocked(c, t)) && !ignoreTaskLock)
    return false;
  auto game = col->getGame();
  auto factory = game->getContentFactory();
  switch (t) {
    case MinionActivity::IDLE:
      return true;
    case MinionActivity::TRAIN:
      return !c->getAttributes().isTrainingMaxedOut(AttrType("DAMAGE")) &&
          !col->hasTrait(c, MinionTrait::PRISONER);
    case MinionActivity::STUDY:
      return !col->hasTrait(c, MinionTrait::PRISONER) &&
          !c->getAttributes().isTrainingMaxedOut(AttrType("SPELL_DAMAGE"));
    case MinionActivity::ARCHERY:
      return !col->hasTrait(c, MinionTrait::PRISONER) &&
          !c->getAttributes().isTrainingMaxedOut(AttrType("RANGED_DAMAGE")) &&
          !c->getEquipment().getItems(ItemIndex::RANGED_WEAPON).empty();
    case MinionActivity::BE_WHIPPED:
      return !c->getBody().isImmuneTo(LastingEffect::ENTANGLED, factory) &&
          !c->getBody().isFarmAnimal() &&
          c->getBody().isHumanoid();
    case MinionActivity::POETRY:
      return col->hasTrait(c, MinionTrait::LEADER);
    case MinionActivity::BE_EXECUTED:
    case MinionActivity::BE_TORTURED:
      return col->hasTrait(c, MinionTrait::PRISONER);
    case MinionActivity::CRAFT:
      for (auto& workshop : factory->workshopInfo)
        if (c->getAttr(workshop.second.attr) > 0)
          return true;
      return false;
    case MinionActivity::SLEEP:
      return !c->getBody().isImmuneTo(LastingEffect::SLEEP, factory) && !c->isAffected(LastingEffect::POISON);
    case MinionActivity::EAT:
      return !c->getBody().isImmuneTo(LastingEffect::SATIATED, factory) && !col->hasTrait(c, MinionTrait::PRISONER);
    case MinionActivity::COPULATE:
      return c->isAffected(BuffId("COPULATION_SKILL"));
    case MinionActivity::RITUAL:
      return c->getBody().canPerformRituals(factory) && !col->hasTrait(c, MinionTrait::WORKER);
    case MinionActivity::GUARDING1:
    case MinionActivity::GUARDING2:
    case MinionActivity::GUARDING3:
      return col->hasTrait(c, MinionTrait::FIGHTER);
    case MinionActivity::CROPS:
      return c->isAffected(BuffId("CROPS_SKILL"));
    case MinionActivity::SPIDER:
      return c->isAffected(BuffId("SPIDER_SKILL"));
    case MinionActivity::EXPLORE_CAVES:
      return c->isAffected(BuffId("EXPLORE_CAVES_SKILL"));
    case MinionActivity::EXPLORE:
      return c->isAffected(BuffId("EXPLORE_SKILL"));
    case MinionActivity::EXPLORE_NOCTURNAL:
      return c->isAffected(BuffId("EXPLORE_NOCTURNAL_SKILL"));
    case MinionActivity::PHYLACTERY:
      return !c->getPhylactery();
    case MinionActivity::CONSTRUCTION:
    case MinionActivity::WORKING:
    case MinionActivity::HAULING:
      return c->getBody().canPickUpItems() && col->hasTrait(c, MinionTrait::WORKER);
    case MinionActivity::DIGGING:
      return c->getAttr(AttrType("DIGGING")) > 0 && col->hasTrait(c, MinionTrait::WORKER);
    case MinionActivity::MINION_ABUSE:
      return col->hasTrait(c, MinionTrait::LEADER) && col == game->getPlayerCollective();
    case MinionActivity::DISTILLATION:
      return c->isAffected(BuffId("DISTILLATION_SKILL"));
    case MinionActivity::PREACHING:
      return c->isAffected(BuffId("PREACHING_SKILL"));
    case MinionActivity::MASS: {
      auto& leaders = col->getLeaders();
      return !col->hasTrait(c, MinionTrait::PRISONER) && c->getBody().isHumanoid() &&
          !c->isAffected(BuffId("PREACHING_SKILL")) &&
          !leaders.empty() && col->getCurrentActivity(leaders[0]).activity == MinionActivity::PREACHING;
    }
    case MinionActivity::PRAYER:
      return !col->hasTrait(c, MinionTrait::PRISONER) &&
          !c->getAttributes().isTrainingMaxedOut(AttrType("DIVINITY"));
    case MinionActivity::HEARING_CONFESSION:
      return c->isAffected(BuffId("PREACHING_SKILL"));
    case MinionActivity::CONFESSION:
      return c->isAffected(BuffId("SINNED"));
  }
}

void MinionActivityMap::toggleLock(MinionActivity task) {
  if (canLock(task))
    locked.toggle(task);
}

bool MinionActivityMap::isLocked(const Collective* col, const Creature* c, MinionActivity task) const {
  return locked.contains(task) ^ isAutoLocked(col, c, task);
}

SERIALIZE_DEF(MinionActivityMap, locked);

#include "pretty_archive.h"
template<>
void MinionActivityMap::serialize(PrettyInputArchive&, unsigned) {
}
