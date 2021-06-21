#pragma once

#include "util.h"
#include "effect.h"
#include "pretty_archive.h"

class Position;
class Furniture;

namespace FurnitureTickTypes {

struct Bed : EmptyStruct<struct BedTag> {};
struct Pigsty : EmptyStruct<struct PigstyTag> {};
struct Portal : EmptyStruct<struct PortalTag> {};
struct MeteorShower : EmptyStruct<struct MeteorShowerTag> {};
struct Pit : EmptyStruct<struct PitTag> {};
struct SetFurnitureOnFire : EmptyStruct<struct SetFurnitureOnFireTag> {};

struct Trap {
  int maxDistance;
  array<Effect, 4> SERIAL(effects); // {south, east, north, west}
  SERIALIZE_ALL(maxDistance, effects)
};

#define VARIANT_TYPES_LIST\
  X(Bed, 0)\
  X(Pigsty, 1)\
  X(Portal, 2)\
  X(MeteorShower, 3)\
  X(Pit, 4)\
  X(SetFurnitureOnFire, 5)\
  X(Trap, 6)\
  X(Effect, 7)

#define VARIANT_NAME FurnitureTickType

#include "gen_variant.h"
#include "gen_variant_serialize.h"
inline
#include "gen_variant_serialize_pretty.h"

#undef VARIANT_TYPES_LIST
#undef VARIANT_NAME

}

class FurnitureTickType : public FurnitureTickTypes::FurnitureTickType {
  public:
  using FurnitureTickTypes::FurnitureTickType::FurnitureTickType;
  void handle(Position, Furniture*);
};
