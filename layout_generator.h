#pragma once

#include "stdafx.h"
#include "util.h"
#include "pretty_archive.h"
#include "tile_predicate.h"

struct LayoutGenerator;
struct LayoutCanvas;

using Token = string;

namespace LayoutGenerators {
  enum class MarginType;
  enum class PlacementPos;
}

RICH_ENUM(LayoutGenerators::MarginType, TOP, BOTTOM, LEFT, RIGHT);
RICH_ENUM(LayoutGenerators::PlacementPos, MIDDLE, MIDDLE_V, MIDDLE_H, LEFT_CENTER, RIGHT_CENTER, TOP_CENTER, BOTTOM_CENTER,
    TOP_RIGHT, TOP_LEFT, BOTTOM_RIGHT, BOTTOM_LEFT, LEFT_STRETCHED, RIGHT_STRETCHED, BOTTOM_STRETCHED, TOP_STRETCHED);

namespace LayoutGenerators {

struct None {
  SERIALIZE_EMPTY()
};

struct Set {
  vector<Token> SERIAL(tokens);
  SERIALIZE_ALL(withRoundBrackets(tokens))
};

struct SetFront {
  Token SERIAL(token);
  SERIALIZE_ALL(roundBracket(), NAMED(token))
};

struct Reset {
  vector<Token> SERIAL(tokens);
  SERIALIZE_ALL(withRoundBrackets(tokens))
};

struct Filter {
  TilePredicate SERIAL(predicate);
  HeapAllocated<LayoutGenerator> SERIAL(generator);
  heap_optional<LayoutGenerator> SERIAL(alt);
  SERIALIZE_ALL(roundBracket(), NAMED(predicate), NAMED(generator), NAMED(alt))
};

struct Remove {
  vector<Token> SERIAL(tokens);
  SERIALIZE_ALL(withRoundBrackets(tokens))
};

struct MarginImpl {
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

struct SplitH {
  double SERIAL(r);
  HeapAllocated<LayoutGenerator> SERIAL(left);
  HeapAllocated<LayoutGenerator> SERIAL(right);
  SERIALIZE_ALL(roundBracket(), NAMED(r), NAMED(left), NAMED(right))
};

struct SplitV {
  double SERIAL(r);
  HeapAllocated<LayoutGenerator> SERIAL(top);
  HeapAllocated<LayoutGenerator> SERIAL(bottom);
  SERIALIZE_ALL(roundBracket(), NAMED(r), NAMED(top), NAMED(bottom))
};

struct Position {
  optional<Vec2> SERIAL(size);
  optional<Vec2> SERIAL(minSize);
  optional<Vec2> SERIAL(maxSize);
  HeapAllocated<LayoutGenerator> SERIAL(generator);
  PlacementPos SERIAL(position);
  SERIALIZE_ALL(roundBracket(), NAMED(position), NAMED(size), NAMED(generator), NAMED(minSize), NAMED(maxSize))
};

struct Place {
  struct Elem {
    optional<Vec2> SERIAL(size);
    optional<Vec2> SERIAL(minSize);
    optional<Vec2> SERIAL(maxSize);
    HeapAllocated<LayoutGenerator> SERIAL(generator);
    Range SERIAL(count) = Range(1, 2);
    TilePredicate SERIAL(predicate) = TilePredicates::True{};
    int SERIAL(minSpacing) = 0;
    SERIALIZE_ALL(roundBracket(), NAMED(size), NAMED(generator), OPTION(count), OPTION(predicate), NAMED(minSize), NAMED(maxSize), OPTION(minSpacing))
  };
  vector<Elem> SERIAL(generators);
  optional<int> placedCount;
  template <class Archive>
  void serialize(Archive& ar1, const unsigned int version) {
    // ar(generators)
    if (version == 0)
      ar1(withRoundBrackets(generators));
    else
      ar1(withRoundBrackets(generators), placedCount);
  }
  void serialize(PrettyInputArchive&, const unsigned int version);
};

struct NoiseMap {
  struct Elem {
    double SERIAL(lower);
    double SERIAL(upper);
    HeapAllocated<LayoutGenerator> SERIAL(generator);
    SERIALIZE_ALL(roundBracket(), NAMED(lower), NAMED(upper), NAMED(generator))
  };
  double SERIAL(exponent);
  vector<Elem> SERIAL(generators);
  SERIALIZE_ALL(roundBracket(), NAMED(exponent), NAMED(generators))
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
    void serialize(PrettyInputArchive&, const unsigned int version);
  };
  vector<Elem> SERIAL(generators);
  SERIALIZE_ALL(withRoundBrackets(generators))
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
  SERIALIZE_ALL(roundBracket(), NAMED(toConnect), NAMED(elems))
  void serialize(PrettyInputArchive&, const unsigned int version);
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

struct Fail {
  SERIALIZE_EMPTY()
};

#define VARIANT_TYPES_LIST\
  X(None, 0)\
  X(Set, 1)\
  X(SetFront, 2)\
  X(Reset, 3)\
  X(Remove, 4)\
  X(Filter, 5)\
  X(MarginImpl, 6)\
  X(Margins, 7)\
  X(SplitH, 8)\
  X(SplitV, 9)\
  X(Position, 10)\
  X(Place, 11)\
  X(NoiseMap, 12)\
  X(Chain, 13)\
  X(Connect, 14)\
  X(Choose, 15)\
  X(Repeat, 16)\
  X(FloodFill, 17)\
  X(Fail, 18)

#define VARIANT_NAME GeneratorImpl

#include "gen_variant.h"
#include "gen_variant_serialize.h"
#define DEFAULT_ELEM "Chain"
inline
#include "gen_variant_serialize_pretty.h"
#undef DEFAULT_ELEM
#undef VARIANT_TYPES_LIST
#undef VARIANT_NAME

}

CEREAL_CLASS_VERSION(LayoutGenerators::Place, 1)

struct LayoutGenerator : LayoutGenerators::GeneratorImpl {
  using GeneratorImpl::GeneratorImpl;
  [[nodiscard]] bool make(LayoutCanvas, RandomGen&) const;
};
