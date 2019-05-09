#include "furniture_list.h"
#include "furniture_factory.h"

FurnitureList::FurnitureList(FurnitureType t) : FurnitureList({{t, 1}}) {
}

FurnitureList::FurnitureList(vector<pair<FurnitureType, double>> elems, vector<FurnitureType> unique)
  : elems(std::move(elems)), unique(std::move(unique)) {
}

FurnitureParams FurnitureList::getRandom(RandomGen& random, TribeId tribe) {
  if (!unique.empty()) {
    FurnitureType f = unique.back();
    unique.pop_back();
    return {f, tribe};
  } else
    return {random.choose(elems), tribe};
}

int FurnitureList::numUnique() const {
  return unique.size();
}
