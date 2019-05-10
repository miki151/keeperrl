#pragma once

#include "furniture_factory.h"
#include "creature_factory.h"

struct ContentFactory {
  ContentFactory(NameGenerator, const GameConfig*);
  CreatureFactory SERIAL(creatures);
  FurnitureFactory SERIAL(furniture);
  void merge(ContentFactory);
  SERIALIZATION_DECL(ContentFactory)
};
