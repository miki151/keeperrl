#pragma once

#include "stdafx.h"
#include "furniture_type.h"
#include "furniture_layer.h"

namespace LayoutActions {
using Place = FurnitureType;
using Flag = SquareAttrib;
enum class StairDirection;
struct Stairs {
  StairDirection SERIAL(dir);
  FurnitureType SERIAL(type);
  SERIALIZE_ALL(dir, type)
};
struct LayoutAction;
using Chain = vector<LayoutAction>;
using ClearFurniture = EmptyStruct<struct ClearFurniturTag>;
using ClearLayer = FurnitureLayer;
using InsideFurniture = EmptyStruct<struct InsideFurnitureTag>;
using OutsideFurniture = EmptyStruct<struct OutsideFurnitureTag>;
struct Stockpile {
  int SERIAL(index);
  SERIALIZE_ALL(index)
};
#define VARIANT_TYPES_LIST\
  X(Place, 0)\
  X(Flag, 1)\
  X(Stairs, 2)\
  X(Chain, 3)\
  X(ClearFurniture, 4)\
  X(Stockpile, 5)\
  X(ClearLayer, 6)

#define VARIANT_NAME LayoutAction

#include "gen_variant.h"
#include "gen_variant_serialize.h"
inline
#include "gen_variant_serialize_pretty.h"

#undef VARIANT_TYPES_LIST
#undef VARIANT_NAME

}

using LayoutActions::LayoutAction;

struct LayoutMapping {
  map<string, LayoutAction> SERIAL(actions);
  SERIALIZE_ALL(actions)
};

RICH_ENUM(
    LayoutActions::StairDirection,
    UP, DOWN
);
