#include "stdafx.h"
#include "square_array.h"
#include "square_factory.h"
#include "square.h"
#include "square_type.h"
#include "creature.h"

SquareArray::SquareArray(Rectangle bounds) : modified(bounds), readonly(bounds, nullptr), types(bounds) {
}

SERIALIZATION_CONSTRUCTOR_IMPL(SquareArray);

SERIALIZE_DEF(SquareArray, modified, readonly, types, readonlyMap, numModified); 

Rectangle SquareArray::getBounds() const {
  return modified.getBounds();
}

Square* SquareArray::getSquare(Vec2 pos) {
  if (!modified[pos]) {
    modified[pos] = SquareFactory::get(*types[pos]);
    readonly[pos] = modified[pos].get();
    ++numModified;
  }
  return modified[pos].get();
}

PSquare SquareArray::extractSquare(Vec2 pos) {
  getSquare(pos);
  types[pos].reset();
  readonly[pos] = nullptr;
  return std::move(modified[pos]);
}

const Square* SquareArray::getReadonly(Vec2 pos) const {
  return readonly[pos];
}

void SquareArray::putSquare(Vec2 pos, SquareType type) {
  if (!readonlyMap.count(type))
    readonlyMap.insert(make_pair(type, SquareFactory::get(type)));
  readonly[pos] = readonlyMap.at(type).get();
  modified[pos].reset();
  types[pos] = type;
}

void SquareArray::putSquare(Vec2 pos, PSquare s) {
  modified[pos] = std::move(s);
  readonly[pos] = modified[pos].get();
}

int SquareArray::getNumModified() const {
  return numModified;
}

