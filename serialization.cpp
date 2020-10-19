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

#include "serialization.h"
#include "creature_factory.h"
#include "monster_ai.h"
#include "effect.h"
#include "skill.h"
#include "tribe.h"
#include "player.h"
#include "monster.h"
#include "square.h"
#include "village_control.h"
#include "task.h"
#include "player_control.h"
#include "item_factory.h"
#include "collective_control.h"
#include "collective.h"
#include "map_memory.h"
#include "visibility_map.h"
#include "view_index.h"
#include "village_behaviour.h"

REGISTER_TYPE(Player)
REGISTER_TYPE(Monster)
REGISTER_TYPE(PlayerControl)
REGISTER_TYPE(VillageControl)
REGISTER_TYPE(DoNothingController)

