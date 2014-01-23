#include "stdafx.h"

#include "collective_action.h"

vector<CollectiveAction::Type> vectorTypes { CollectiveAction::GO_TO };
vector<CollectiveAction::Type> intTypes { CollectiveAction::ROOM_BUTTON };
vector<CollectiveAction::Type> creatureTypes { CollectiveAction::CREATURE_BUTTON,
    CollectiveAction::CREATURE_DESCRIPTION };

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
  CHECK(!contains(intTypes, t) && !contains(vectorTypes, t) && !contains(creatureTypes, t));
}

CollectiveAction::Type CollectiveAction::getType() {
  return type;
}

Vec2 CollectiveAction::getPosition() {
  CHECK(contains(vectorTypes, type));
  return pos;
}

int CollectiveAction::getNum() {
  CHECK(contains(intTypes, type));
  return num;
}

const Creature* CollectiveAction::getCreature() {
  CHECK(contains(creatureTypes, type));
  return creature;
}
