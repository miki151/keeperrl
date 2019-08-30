#include "furniture_list.h"
#include "furniture_factory.h"

SERIALIZE_DEF(FurnitureList, NAMED(elems), OPTION(unique))
SERIALIZATION_CONSTRUCTOR_IMPL(FurnitureList)

FurnitureList::FurnitureList(FurnitureType t) : FurnitureList({{t, 1}}) {
}

FurnitureList::FurnitureList(vector<pair<FurnitureType, double>> elems, vector<FurnitureType> unique)
  : elems(std::move(elems)), unique(std::move(unique)) {
}

optional<FurnitureParams> FurnitureList::getRandom(RandomGen& random, TribeId tribe) {
  if (!unique.empty()) {
    FurnitureType f = unique.back();
    unique.pop_back();
    return FurnitureParams{f, tribe};
  } else
  if (!elems.empty())
    return FurnitureParams{random.choose(elems), tribe};
  else
    return none;
}

int FurnitureList::numUnique() const {
  return unique.size();
}

#include "pretty_archive.h"
template
void FurnitureList::serialize(PrettyInputArchive& ar1, unsigned);
