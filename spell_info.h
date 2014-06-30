#ifndef _SPELL_INFO_H
#define _SPELL_INFO_H

#include "enums.h"
#include "util.h"

struct SpellInfo {
  SpellId id;
  string name;
  EffectType type;
  double ready;
  int difficulty;

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version);
};



#endif
