#pragma once

#include "util.h"
#include "square.h"


class SquareArray {
  public:
  SquareArray(Rectangle bounds) : modified(bounds), readOnly(make_unique<Square>()) {}

  Table<PSquare> SERIAL(modified);
  PSquare SERIAL(readOnly);
  int SERIAL(numModified) = 0;

  SERIALIZE_ALL(modified, readOnly, numModified)
  SERIALIZATION_CONSTRUCTOR(SquareArray)

  const Rectangle& getBounds() const {
    return modified.getBounds();
  }

  Square* getWritable(Vec2 pos) {
    if (!modified[pos]) {
      modified[pos] = make_unique<Square>();
      ++numModified;
    }
    return modified[pos].get();
  }

  const Square* getReadonly(Vec2 pos) const {
    if (modified[pos])
      return modified[pos].get();
    else
      return readOnly.get();
  }

  int getNumGenerated() const {
    return numModified;
  }

  int getNumTotal() const {
    return numModified;
  }

};
