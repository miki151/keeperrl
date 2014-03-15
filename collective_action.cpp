#include "stdafx.h"

#include "collective_action.h"

CollectiveAction::CollectiveAction() {}

vector<CollectiveAction::Type> vectorIntTypes { CollectiveAction::BUILD, CollectiveAction::WORKSHOP,
  CollectiveAction::RECT_SELECTION };
vector<CollectiveAction::Type> vectorTypes { CollectiveAction::POSSESS };
vector<CollectiveAction::Type> intTypes { CollectiveAction::TECHNOLOGY, CollectiveAction::BUTTON_RELEASE,
    CollectiveAction::CREATURE_BUTTON, CollectiveAction::CREATURE_DESCRIPTION};

CollectiveAction::CollectiveAction(Type t, Vec2 p, int n) : type(t), pos(p), num(n) {
  CHECK(contains(vectorIntTypes, t));
}

CollectiveAction::CollectiveAction(Type t, Vec2 p) : type(t), pos(p) {
  CHECK(contains(vectorTypes, t));
}

CollectiveAction::CollectiveAction(Type t, int n) : type(t), num(n) {
  CHECK(contains(intTypes, t));
}

CollectiveAction::CollectiveAction(Type t) : type(t) {
  CHECK(!contains(vectorIntTypes, t) && !contains(intTypes, t) && !contains(vectorTypes, t));
}

CollectiveAction::Type CollectiveAction::getType() {
  return type;
}

Vec2 CollectiveAction::getPosition() {
  CHECK(contains(vectorTypes, type) || contains(vectorIntTypes, type));
  return pos;
}

int CollectiveAction::getNum() {
  CHECK(contains(intTypes, type) || contains(vectorIntTypes, type));
  return num;
}

std::ostream& operator << (std::ostream& o, CollectiveAction a) {
  o << (int) a.getType();
  if (contains(vectorIntTypes, a.getType()))
    o << " " << a.getPosition() << " " << a.getNum();
  if (contains(vectorTypes, a.getType()))
    o << " " << a.getPosition();
  if (contains(intTypes, a.getType()))
    o << " " << a.getNum();
  return o;
}

std::istream& operator >> (std::istream& in, CollectiveAction& a) {
  int _id;
  in >> _id;
  CollectiveAction::Type id = CollectiveAction::Type(_id);
  if (contains(vectorTypes, id)) {
    Vec2 v;
    in >> v;
    a = CollectiveAction(id, v);
  } else
  if (contains(vectorIntTypes, id)) {
    Vec2 v;
    int n;
    in >> v >> n;
    a = CollectiveAction(id, v, n);
  } else
  if (contains(intTypes, id)) {
    int n;
    in >> n;
    a = CollectiveAction(id, n);
  } else
    a = CollectiveAction(id);
  return in;
}

