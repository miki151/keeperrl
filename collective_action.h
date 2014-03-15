#ifndef _COLLECTIVE_ACTION_H
#define _COLLECTIVE_ACTION_H

#include "util.h"

class CollectiveAction {
  public:
  enum Type { IDLE, BUILD, WORKSHOP, RECT_SELECTION, POSSESS, BUTTON_RELEASE, CREATURE_BUTTON,
      CREATURE_DESCRIPTION, GATHER_TEAM, CANCEL_TEAM, MARKET, TECHNOLOGY, DRAW_LEVEL_MAP, EXIT };

  CollectiveAction(Type, Vec2 pos, int);
  CollectiveAction(Type, Vec2 pos);
  CollectiveAction(Type, int);
  CollectiveAction(Type);
  CollectiveAction();

  Type getType();
  Vec2 getPosition();
  int getNum();

  private:
  Type type;
  Vec2 pos;
  int num;
};

std::ostream& operator << (std::ostream&, CollectiveAction);
std::istream& operator >> (std::istream&, CollectiveAction&);

#endif
