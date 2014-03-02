#include "stdafx.h"
#include "unique_entity.h"

static int idCounter = 1;

UniqueEntity::UniqueEntity() {
  id = ++idCounter;
}

UniqueId UniqueEntity::getUniqueId() const {
  return id;
}

template <class Archive> 
void UniqueEntity::serialize(Archive& ar, const unsigned int version) {
  ar & BOOST_SERIALIZATION_NVP(id);
  if (id > idCounter)
    idCounter = id;
}

SERIALIZABLE(UniqueEntity);
