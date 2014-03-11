#include "stdafx.h"

#include "collective_action.h"

vector<CollectiveAction::Type> vectorIntTypes { CollectiveAction::BUILD, CollectiveAction::WORKSHOP,
  CollectiveAction::RECT_SELECTION };
vector<CollectiveAction::Type> vectorTypes { CollectiveAction::POSSESS };
vector<CollectiveAction::Type> intTypes { CollectiveAction::TECHNOLOGY, CollectiveAction::BUTTON_RELEASE };
vector<CollectiveAction::Type> creatureTypes { CollectiveAction::CREATURE_BUTTON,
    CollectiveAction::CREATURE_DESCRIPTION };

CollectiveAction::CollectiveAction(Type t, Vec2 p, int n) : type(t), pos(p), num(n) {
  CHECK(contains(vectorIntTypes, t));
}

CollectiveAction::CollectiveAction(Type t, Vec2 p) : type(t), pos(p) {
  CHECK(contains(vectorTypes, t));
}

CollectiveAction::CollectiveAction(Type t, int n) : type(t), num(n) {
  CHECK(contains(intTypes, t));
}

CollectiveAction::CollectiveAction(Type t, const Creature* c) : type(t), creature(c) {
  CHECK(contains(creatureTypes, t));
}

CollectiveAction::CollectiveAction(Type t) : type(t) {
  CHECK(!contains(vectorIntTypes, t) && !contains(intTypes, t) && !contains(vectorTypes, t)
      && !contains(creatureTypes, t));
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

const Creature* CollectiveAction::getCreature() {
  CHECK(contains(creatureTypes, type));
  return creature;
}
