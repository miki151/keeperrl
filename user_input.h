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

#ifndef _USER_INPUT_H
#define _USER_INPUT_H

#include "util.h"

struct UserInput {
  enum Type {
// common
    IDLE,
    DRAW_LEVEL_MAP,
    EXIT,
// real-time actions
    BUILD,
    WORKSHOP,
    LIBRARY,
    DEITIES,
    RECT_SELECTION,
    POSSESS,
    BUTTON_RELEASE,
    CREATURE_BUTTON,
    GATHER_TEAM,
    EDIT_TEAM,
    CANCEL_TEAM,
    MARKET,
    TECHNOLOGY,
// turn-based actions
    MOVE,
    MOVE_TO,
    TRAVEL,
    FIRE,
    PICK_UP,
    EXT_PICK_UP,
    DROP,
    EXT_DROP,
    SHOW_INVENTORY,
    APPLY_ITEM,
    EQUIPMENT,
    THROW,
    THROW_DIR,
    SHOW_HISTORY,
    HIDE,
    PAY_DEBT,
    CHAT,
    WAIT,
    UNPOSSESS,
    CAST_SPELL,
  } type;

  struct BuildInfo {
    int building;
  };

  UserInput();
  UserInput(Type);
  UserInput(Type, Vec2, BuildInfo);
  UserInput(Type, int);
  UserInput(Type, Vec2);

  Vec2 getPosition();
  int getNum();
  BuildInfo getBuildInfo();

  private:

  Vec2 position;
  int num;
  BuildInfo buildInfo;
};

std::ostream& operator << (std::ostream&, UserInput);
std::istream& operator >> (std::istream&, UserInput&);

#endif
