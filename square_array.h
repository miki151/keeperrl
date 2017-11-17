#pragma once

#include "util.h"
#include "square_array.h"
#include "square.h"
#include "read_write_array.h"

using SquareParam = int;

struct GenerateSquare {
  PSquare operator()(SquareParam) {
    return makeOwner<Square>();
  }
};

class SquareArray : public ReadWriteArray<Square, SquareParam, GenerateSquare> {
  public:
  using ReadWriteArray::ReadWriteArray;
};
