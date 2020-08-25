#pragma once

#include "stdafx.h"
#include "util.h"
#include "pretty_archive.h"
#include "tile_predicate.h"

struct LayoutGenerator;
struct LayoutCanvas;

using Token = string;

RICH_ENUM(MarginType, TOP, BOTTOM, LEFT, RIGHT);
RICH_ENUM(PlacementPos, MIDDLE, MIDDLE_V, MIDDLE_H, LEFT_CENTER, RIGHT_CENTER, TOP_CENTER, BOTTOM_CENTER);

namespace LayoutGenerators {

struct None {
  SERIALIZE_EMPTY()
};

struct Set {
  vector<Token> SERIAL(tokens);
  SERIALIZE_ALL(tokens)
};

struct SetFront {
  Token SERIAL(token);
  SERIALIZE_ALL(token)
};

struct Reset {
  vector<Token> SERIAL(tokens);
  SERIALIZE_ALL(tokens)
};

struct Filter {
  TilePredicate SERIAL(predicate);
  HeapAllocated<LayoutGenerator> SERIAL(generator);
  heap_optional<LayoutGenerator> SERIAL(alt);
  SERIALIZE_ALL(roundBracket(), NAMED(predicate), NAMED(generator), NAMED(alt))
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
  SERIALIZE_ALL(roundBracket(), NAMED(type), NAMED(width), NAMED(border), NAMED(inside))
};

struct Margins {
  int SERIAL(width);
  HeapAllocated<LayoutGenerator> SERIAL(border);
  HeapAllocated<LayoutGenerator> SERIAL(inside);
  SERIALIZE_ALL(roundBracket(), NAMED(width), NAMED(border), NAMED(inside))
};

struct HRatio {
  double SERIAL(r);
  HeapAllocated<LayoutGenerator> SERIAL(left);
  HeapAllocated<LayoutGenerator> SERIAL(right);
  SERIALIZE_ALL(roundBracket(), NAMED(r), NAMED(left), NAMED(right))
};

struct VRatio {
  double SERIAL(r);
  HeapAllocated<LayoutGenerator> SERIAL(top);
  HeapAllocated<LayoutGenerator> SERIAL(bottom);
  SERIALIZE_ALL(roundBracket(), NAMED(r), NAMED(top), NAMED(bottom))
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
  struct Elem {
    optional<double> SERIAL(chance);
    HeapAllocated<LayoutGenerator> SERIAL(generator);
    SERIALIZE_ALL(chance, generator)
    void serialize(PrettyInputArchive& ar, const unsigned int version);
  };

  vector<Elem> SERIAL(generators);
  SERIALIZE_ALL(generators)
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

struct Repeat {
  Range SERIAL(count);
  HeapAllocated<LayoutGenerator> SERIAL(generator);
  SERIALIZE_ALL(roundBracket(), NAMED(count), NAMED(generator))
};

struct FloodFill {
  TilePredicate SERIAL(predicate);
  HeapAllocated<LayoutGenerator> SERIAL(generator);
  SERIALIZE_ALL(roundBracket(), NAMED(predicate), NAMED(generator))
};

#define VARIANT_TYPES_LIST\
  X(None, 0)\
  X(Set, 1)\
  X(SetFront, 2)\
  X(Reset, 3)\
  X(Remove, 4)\
  X(Filter, 5)\
  X(Margin, 6)\
  X(Margins, 7)\
  X(HRatio, 8)\
  X(VRatio, 9)\
  X(Place, 10)\
  X(NoiseMap, 11)\
  X(Chain, 12)\
  X(Connect, 13)\
  X(Choose, 14)\
  X(Repeat, 15)\
  X(FloodFill, 16)

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
