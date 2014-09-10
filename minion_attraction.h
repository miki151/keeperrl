#ifndef _MINION_ATTRACTION_H
#define _MINION_ATTRACTION_H

#include "enum_variant.h"
#include "square_type.h"
#include "util.h"

enum class ItemClass;

enum class AttractionId {
  SQUARE,
  ITEM_CLASS,
};

typedef EnumVariant<AttractionId, TYPES(SquareType, ItemClass),
    ASSIGN(SquareType, AttractionId::SQUARE),
    ASSIGN(ItemClass, AttractionId::ITEM_CLASS)> MinionAttraction;

namespace std {
  template <> struct hash<MinionAttraction> {
    size_t operator()(const MinionAttraction& t) const {
      return (size_t)t.getId();
    }
  };
}

#endif
