#include "stdafx.h"
#include "bed_type.h"
#include "t_string.h"

TStringId getName(BedType type) {
  switch (type) {
    case BedType::PRISON: return TStringId("PRISON_TILE");
    case BedType::COFFIN: return TStringId("COFFIN");
    case BedType::BED: return TStringId("BED");
    case BedType::CAGE: return TStringId("CAGE");
    case BedType::STABLE: return TStringId("STABLE");
  }
}
