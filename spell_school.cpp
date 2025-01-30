#include "spell_school.h"
#include "pretty_archive.h"

SERIALIZE_DEF(SpellSchool, expType, spells, name)
SERIALIZATION_CONSTRUCTOR_IMPL(SpellSchool)

void SpellSchool::serialize(PrettyInputArchive& ar1, const unsigned int) { \
  auto bookmark = ar1.bookmark();
  auto s1 = ar1.eat();
  auto s2 = ar1.eat();
  ar1.seek(bookmark);
  if (s2[0] == '{')
    ar1(expType, spells);
  else
    ar1(name, expType, spells);
}
