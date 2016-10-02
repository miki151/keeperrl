#include "stdafx.h"
#include "furniture_array.h"

template <class Archive>
void FurnitureArray::serialize(Archive& ar, const unsigned int) {
  ar & SUBCLASS(ReadWriteArray);
  serializeAll(ar, construction);
}
SERIALIZABLE(FurnitureArray)
SERIALIZATION_CONSTRUCTOR_IMPL(FurnitureArray)

FurnitureArray::FurnitureArray(Rectangle bounds) : ReadWriteArray(bounds), construction(bounds) {

}
