#include "stdafx.h"
#include "content_factory.h"
#include "name_generator.h"

SERIALIZE_DEF(ContentFactory, creatures, furniture)

SERIALIZATION_CONSTRUCTOR_IMPL(ContentFactory)

ContentFactory::ContentFactory(NameGenerator nameGenerator, const GameConfig* config)
    : creatures(std::move(nameGenerator), config), furniture(config) {
}

void ContentFactory::merge(ContentFactory f) {
  creatures.merge(std::move(f.creatures));
  furniture.merge(std::move(f.furniture));
}
