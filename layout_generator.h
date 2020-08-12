#pragma once

#include "stdafx.h"
#include "util.h"
#include "pretty_archive.h"
#include "tile_predicate.h"

struct LayoutGenerator;
struct LayoutCanvas;

using Token = string;

RICH_ENUM(MarginType, TOP, BOTTOM, LEFT, RIGHT);
RICH_ENUM(PlacementPos, MIDDLE);

namespace LayoutGenerators {

struct None {
  SERIALIZE_EMPTY()
};

struct Set {
  vector<Token> SERIAL(tokens);
  SERIALIZE_ALL(tokens)
};

struct Reset {
  vector<Token> SERIAL(tokens);
  SERIALIZE_ALL(tokens)
};

struct SetMaybe {
  TilePredicate SERIAL(predicate);
  vector<Token> SERIAL(tokens);
  SERIALIZE_ALL(predicate, tokens)
};

struct Remove {
  vector<Token> SERIAL(tokens);
  SERIALIZE_ALL(tokens)
};

struct Margin {
  MarginType SERIAL(type);
  int SERIAL(width);
  HeapAllocated<LayoutGenerator> SERIAL(border);
  HeapAllocated<LayoutGenerator> SERIAL(inside);
  SERIALIZE_ALL(type, width, border, inside)
};

struct Margins {
  int SERIAL(width);
  HeapAllocated<LayoutGenerator> SERIAL(border);
  HeapAllocated<LayoutGenerator> SERIAL(inside);
  SERIALIZE_ALL(width, border, inside)
};

struct HRatio {
  double SERIAL(r);
  HeapAllocated<LayoutGenerator> SERIAL(left);
  HeapAllocated<LayoutGenerator> SERIAL(right);
  SERIALIZE_ALL(r, left, right)
};

struct VRatio {
  double SERIAL(r);
  HeapAllocated<LayoutGenerator> SERIAL(top);
  HeapAllocated<LayoutGenerator> SERIAL(bottom);
  SERIALIZE_ALL(r, top, bottom)
};

struct Place {
  struct Elem {
    optional<Vec2> SERIAL(size);
    optional<Vec2> SERIAL(minSize);
    optional<Vec2> SERIAL(maxSize);
    HeapAllocated<LayoutGenerator> SERIAL(generator);
    Range SERIAL(count) = Range(1, 2);
    TilePredicate SERIAL(predicate) = TilePredicates::True{};
    optional<PlacementPos> SERIAL(position);
    int SERIAL(minSpacing) = 0;
    SERIALIZE_ALL(NAMED(size), NAMED(generator), OPTION(count), OPTION(predicate), OPTION(position), NAMED(minSize), NAMED(maxSize), OPTION(minSpacing))
  };
  vector<Elem> SERIAL(generators);
  SERIALIZE_ALL(generators)
};

struct NoiseMap {
  struct Elem {
    double SERIAL(lower);
    double SERIAL(upper);
    HeapAllocated<LayoutGenerator> SERIAL(generator);
    SERIALIZE_ALL(lower, upper, generator)
  };
  vector<Elem> SERIAL(generators);
  SERIALIZE_ALL(generators)
};

struct Chain {
  vector<LayoutGenerator> SERIAL(generators);
  SERIALIZE_ALL(generators)
};

struct Choose {
  vector<LayoutGenerator> SERIAL(generators);
  SERIALIZE_ALL(generators)
};

struct Call {
  string SERIAL(name);
  SERIALIZE_ALL(name)
};

struct Connect {
  struct Elem {
    optional<double> SERIAL(cost);
    TilePredicate SERIAL(predicate);
    HeapAllocated<LayoutGenerator> SERIAL(generator);
    SERIALIZE_ALL(cost, predicate, generator)
  };
  TilePredicate SERIAL(toConnect);
  vector<Elem> SERIAL(elems);
  SERIALIZE_ALL(toConnect, elems)
};

#define VARIANT_TYPES_LIST\
  X(None, 0)\
  X(Set, 1)\
  X(Reset, 2)\
  X(SetMaybe, 3)\
  X(Remove, 4)\
  X(Margin, 5)\
  X(Margins, 6)\
  X(HRatio, 7)\
  X(VRatio, 8)\
  X(Place, 9)\
  X(NoiseMap, 10)\
  X(Chain, 11)\
  X(Connect, 12)\
  X(Choose, 13)

#define VARIANT_NAME GeneratorImpl

#include "gen_variant.h"
#include "gen_variant_serialize.h"
inline
#include "gen_variant_serialize_pretty.h"

#undef VARIANT_TYPES_LIST
#undef VARIANT_NAME

}

struct LayoutGenerator : LayoutGenerators::GeneratorImpl {
  using GeneratorImpl::GeneratorImpl;
  [[nodiscard]] bool make(LayoutCanvas, RandomGen&) const;
};
