#include "stdafx.h"
#include "bed_type.h"

const char* getName(BedType type) {
  switch (type) {
    case BedType::PRISON: return "prison tile";
    case BedType::COFFIN: return "coffin";
    case BedType::BED: return "bed";
    case BedType::CAGE: return "cage";
    case BedType::STABLE: return "stable";
  }
}
