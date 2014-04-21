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

  UserInput();
  UserInput(Type);
  UserInput(Type, Vec2, int);
  UserInput(Type, int);
  UserInput(Type, Vec2);

  Vec2 getPosition();
  int getNum();

  private:

  Vec2 position;
  int num;
};

std::ostream& operator << (std::ostream&, UserInput);
std::istream& operator >> (std::istream&, UserInput&);

#endif
