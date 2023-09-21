#include "spell_school.h"
#include "pretty_archive.h"
#include "experience_type.h"

SERIALIZE_DEF(SpellSchool, expType, spells, name)
SERIALIZATION_CONSTRUCTOR_IMPL(SpellSchool)

void SpellSchool::serialize(PrettyInputArchive& ar1, const unsigned int) { \
  if (ar1.peek()[0] == '\"') {
    ar1(name, expType, spells);
  } else
    ar1(expType, spells);
}
