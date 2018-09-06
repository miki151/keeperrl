#pragma once

#include "fx_name.h"
#include "color.h"

struct FurnitureFXInfo {
  FXName name;
  Color color;
};

optional<FurnitureFXInfo> destroyFXInfo(FurnitureType);
optional<FurnitureFXInfo> tryDestroyFXInfo(FurnitureType);
optional<FurnitureFXInfo> walkIntoFXInfo(FurnitureType);
optional<FurnitureFXInfo> walkOverFXInfo(FurnitureType);
