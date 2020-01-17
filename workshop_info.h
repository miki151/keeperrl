#pragma once

#include "furniture_type.h"

struct WorkshopInfo {
  FurnitureType SERIAL(furniture);
  string SERIAL(taskName);
  SkillId SERIAL(skill);
  SERIALIZE_ALL(furniture, taskName, skill)
};
