#include "stdafx.h"
#include "content_factory.h"

void ContentFactory::merge(ContentFactory f) {
  creatures.merge(std::move(f.creatures));
}
