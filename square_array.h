#ifndef _SQUARE_ARRAY_H
#define _SQUARE_ARRAY_H

#include "util.h"
#include "square_type.h"

class SquareArray {
  public:
  SquareArray(Rectangle bounds);
  Rectangle getBounds() const;
  Square* getSquare(Vec2);
  PSquare extractSquare(Vec2);
  const Square* getReadonly(Vec2) const;
  void putSquare(Vec2, SquareType);
  void putSquare(Vec2, PSquare);
  int getNumModified() const;

  SERIALIZATION_DECL(SquareArray);

  private:
  Table<PSquare> SERIAL(modified);
  Table<Square*> SERIAL(readonly);
  Table<optional<SquareType>> SERIAL(types);
  unordered_map<SquareType, PSquare> SERIAL(readonlyMap);
  int SERIAL(numModified) = 0;
};

#endif
