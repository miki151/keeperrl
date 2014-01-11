#include "stdafx.h"

vector<ActionId> dirActions {ActionId::MOVE, ActionId::MOVE_TO, ActionId::TRAVEL, ActionId::FIRE,
    ActionId::THROW_DIR};

Action::Action() {
}

Action::Action(ActionId i) : id(i) {
  CHECK(!contains(dirActions, id)) << "Id " << (int)id << " needs direction";
}

ActionId Action::getId() {
  return id;
}

Action::Action(ActionId i, Vec2 dir) : id(i), direction(dir) {
  CHECK(contains(dirActions, id)) << "Id " << (int)id << " doesn't have direction";
}

Vec2 Action::getDirection() {
  CHECK(contains(dirActions, id)) << "Id " << (int)id << " doesn't have direction";
  return direction;
}

std::ostream& operator << (std::ostream& o, Action a) {
  o << (int) a.getId();
  if (contains(dirActions, a.getId()))
    o << " " << a.getDirection();
  return o;
}

std::istream& operator >> (std::istream& in, Action& a) {
  int id;
  in >> id;
  if (contains(dirActions, (ActionId) id)) {
    Vec2 v;
    in >> v;
    a = Action((ActionId) id, v);
  } else
    a = Action((ActionId) id);
  return in;
}

