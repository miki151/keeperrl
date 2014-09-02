#ifndef _MINION_ATTRACTION_H
#define _MINION_ATTRACTION_H

#include "enum_variant.h"
#include "square_type.h"
#include "util.h"

enum class AttractionId {
  SQUARE,
};

typedef EnumVariant<AttractionId, TYPES(SquareType),
    ASSIGN(SquareType, AttractionId::SQUARE)> MinionAttraction;

namespace std {
  template <> struct hash<MinionAttraction> {
    size_t operator()(const MinionAttraction& t) const {
      return (size_t)t.getId();
    }
  };
}

#endif
