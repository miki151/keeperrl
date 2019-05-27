#pragma once

#include "furniture_type.h"
#include "furniture_list_id.h"

struct FurnitureParams;
class TribeId;

class FurnitureList {
  public:
  FurnitureList(FurnitureType);
  FurnitureList(vector<pair<FurnitureType, double>>, vector<FurnitureType> unique = {});
  FurnitureParams getRandom(RandomGen&, TribeId);
  int numUnique() const;

  SERIALIZATION_DECL(FurnitureList)

  private:
  vector<pair<FurnitureType, double>> SERIAL(elems);
  vector<FurnitureType> SERIAL(unique);
};
