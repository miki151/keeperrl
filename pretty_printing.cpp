#include "stdafx.h"
#include "pretty_printing.h"
#include "pretty_archive.h"
#include "creature_factory.h"
#include "furniture_type.h"
#include "attack_type.h"
#include "effect.h"
#include "lasting_effect.h"
#include "attr_type.h"
#include "body.h"

optional<EffectType> PrettyPrinting::getEffect(const string& s) {
  try {
    PrettyInput input(s);
    EffectType type;
    input.getArchive() >> type;
    return type;
  } catch (...) {}
  return none;
}
