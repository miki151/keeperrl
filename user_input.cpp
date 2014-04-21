#include "stdafx.h"
#include "user_input.h"

vector<UserInput::Type> vectorIntTypes {
  UserInput::BUILD,
  UserInput::WORKSHOP,
  UserInput::RECT_SELECTION };

vector<UserInput::Type> vectorTypes {
  UserInput::POSSESS,
  UserInput::MOVE, 
  UserInput::MOVE_TO, 
  UserInput::TRAVEL, 
  UserInput::FIRE,
  UserInput::THROW_DIR,
};

vector<UserInput::Type> intTypes {
  UserInput::TECHNOLOGY,
  UserInput::BUTTON_RELEASE,
  UserInput::CREATURE_BUTTON
};

UserInput::UserInput() {
}

UserInput::UserInput(Type t, Vec2 p, int n) : type(t), position(p), num(n) {
  CHECK(contains(vectorIntTypes, t));
}

UserInput::UserInput(Type t, Vec2 p) : type(t), position(p) {
  CHECK(contains(vectorTypes, t));
}

UserInput::UserInput(Type t, int n) : type(t), num(n) {
  CHECK(contains(intTypes, t));
}

UserInput::UserInput(Type t) : type(t) {
  CHECK(!contains(vectorIntTypes, t) && !contains(intTypes, t) && !contains(vectorTypes, t));
}

Vec2 UserInput::getPosition() {
  CHECK(contains(vectorTypes, type) || contains(vectorIntTypes, type));
  return position;
}

int UserInput::getNum() {
  CHECK(contains(vectorIntTypes, type) || contains(intTypes, type));
  return num;
}

std::ostream& operator << (std::ostream& o, UserInput a) {
  o << (int) a.type;
  if (contains(vectorIntTypes, a.type))
    o << " " << a.getPosition() << " " << a.getNum();
  if (contains(vectorTypes, a.type))
    o << " " << a.getPosition();
  if (contains(intTypes, a.type))
    o << " " << a.getNum();
  return o;
}

std::istream& operator >> (std::istream& in, UserInput& a) {
  int _id;
  in >> _id;
  UserInput::Type id = UserInput::Type(_id);
  if (contains(vectorTypes, id)) {
    Vec2 v;
    in >> v;
    a = UserInput(id, v);
  } else
  if (contains(vectorIntTypes, id)) {
    Vec2 v;
    int n;
    in >> v >> n;
    a = UserInput(id, v, n);
  } else
  if (contains(intTypes, id)) {
    int n;
    in >> n;
    a = UserInput(id, n);
  } else
    a = UserInput(id);
  return in;
}

