#include "stdafx.h"
#include "furniture.h"
#include "player_message.h"

Furniture::Furniture(const string& n, const ViewObject& o, Type t) : Renderable(o), name(n), type(t) {}

SERIALIZATION_CONSTRUCTOR_IMPL(Furniture);

template<typename Archive>
void Furniture::serialize(Archive& ar, const unsigned) {
  ar & SUBCLASS(Renderable);
  serializeAll(ar, name, type);
}

SERIALIZABLE(Furniture);

void Furniture::apply(Position, Creature*) {
}

const string& Furniture::getName() const {
  return name;
}

bool Furniture::isWalkable() const {
  return type == NON_BLOCKING;
}

double Furniture::getApplyTime() const {
  return 1;
}
  
void Furniture::onDestroy(Position pos) {
  pos.globalMessage("The " + name + " is destroyed.");
}
