#pragma once

#include "stdafx.h"
#include "util.h"
#include "pretty_archive.h"
#include "layout_canvas.h"

using Token = string;

struct TilePredicate;

namespace TilePredicates {

struct On {
  Token SERIAL(token);
  SERIALIZE_ALL(roundBracket(), NAMED(token))
};

struct Not {
  HeapAllocated<TilePredicate> SERIAL(predicate);
  SERIALIZE_ALL(predicate)
};

struct True {
  SERIALIZE_EMPTY()
};

struct And {
  vector<TilePredicate> SERIAL(predicates);
  SERIALIZE_ALL(withRoundBrackets(predicates))
};

struct Or {
  vector<TilePredicate> SERIAL(predicates);
  SERIALIZE_ALL(withRoundBrackets(predicates))
};

struct Chance {
  double SERIAL(value);
  SERIALIZE_ALL(roundBracket(), NAMED(value))
};

struct Area {
  int SERIAL(radius);
  HeapAllocated<TilePredicate> SERIAL(predicate);
  int SERIAL(minCount) = 1;
  SERIALIZE_ALL(roundBracket(), NAMED(radius), NAMED(predicate), OPTION(minCount))
};

struct XMod {
  int SERIAL(div);
  int SERIAL(mod);
  SERIALIZE_ALL(roundBracket(), NAMED(div), NAMED(mod))
};

struct YMod {
  int SERIAL(div);
  int SERIAL(mod);
  SERIALIZE_ALL(roundBracket(), NAMED(div), NAMED(mod))
};

#define VARIANT_TYPES_LIST\
  X(On, 0)\
  X(Not, 1)\
  X(True, 2)\
  X(And, 3)\
  X(Or, 4)\
  X(Chance, 5)\
  X(Area, 6)\
  X(XMod, 7)\
  X(YMod, 8)

#define VARIANT_NAME PredicateImpl

#include "gen_variant.h"
#include "gen_variant_serialize.h"
inline
#include "gen_variant_serialize_pretty.h"

#undef VARIANT_TYPES_LIST
#undef VARIANT_NAME

}

struct TilePredicate : TilePredicates::PredicateImpl {
  using PredicateImpl::PredicateImpl;
  bool apply(LayoutCanvas::Map*, Vec2, RandomGen&) const;
};
