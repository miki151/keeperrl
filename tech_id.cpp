#include "tech_id.h"

const string getName(TechId id) {
  return id.data();
}

const string getTechIdName(TechId id) {
  return getName(id);
}
