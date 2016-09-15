#pragma once

#include "util.h"
#include "square_array.h"
#include "square_factory.h"
#include "square.h"
#include "square_type.h"
#include "read_write_array.h"

struct GenerateSquare {
  PSquare operator()(SquareType t) {
    return SquareFactory::get(t);
  }
};

class SquareArray : public ReadWriteArray<Square, SquareType, GenerateSquare> {
  public:
  using ReadWriteArray::ReadWriteArray;
};
