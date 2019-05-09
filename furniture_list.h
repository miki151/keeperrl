#pragma once

#include "furniture_type.h"

struct FurnitureParams;
class TribeId;

class FurnitureList {
  public:
  FurnitureList(FurnitureType);
  FurnitureList(vector<pair<FurnitureType, double>>, vector<FurnitureType> unique = {});
  FurnitureParams getRandom(RandomGen&, TribeId);
  int numUnique() const;

  private:
  vector<pair<FurnitureType, double>> elems;
  vector<FurnitureType> unique;
};

using FurnitureListId = string;
