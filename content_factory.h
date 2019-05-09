#pragma once

#include "furniture_factory.h"
#include "creature_factory.h"

struct ContentFactory {
  CreatureFactory SERIAL(creatures);
  FurnitureFactory SERIAL(furniture);
  void merge(ContentFactory);
  SERIALIZE_ALL(creatures, furniture)
};
