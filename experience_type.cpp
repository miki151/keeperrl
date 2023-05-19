#include "stdafx.h"
#include "experience_type.h"
#include "attr_type.h"

const char* getName(ExperienceType type) {
  switch (type) {
    case ExperienceType::SPELL:
      return "Spell";
    case ExperienceType::MELEE:
      return "Melee";
    case ExperienceType::ARCHERY:
      return "Archery";
    case ExperienceType::FORGE:
      return "Forge";
  }
}

const static EnumMap<ExperienceType, string> lowerCaseName([](ExperienceType type) { return toLower(getName(type)); });

const char* getNameLowerCase(ExperienceType type) {
  return lowerCaseName[type].c_str();
}

const EnumMap<ExperienceType, unordered_map<AttrType, int, CustomHash<AttrType>>>& getAttrIncreases() {
  static const EnumMap<ExperienceType, unordered_map<AttrType, int, CustomHash<AttrType>>> attrIncreases {
    {ExperienceType::MELEE, {
        {AttrType("DAMAGE"), 1},
        {AttrType("DEFENSE"), 1}
    }},
    {ExperienceType::SPELL, {
        {AttrType("SPELL_DAMAGE"), 1},
        {AttrType("SPELL_SPEED"), 3}
    }},
    {ExperienceType::ARCHERY, {
        {AttrType("RANGED_DAMAGE"), 1}
    }},
    {ExperienceType::FORGE, {
        {AttrType("FORGE"), 1}
    }},
  };
  return attrIncreases;
}

optional<pair<ExperienceType, int>> getExperienceType(AttrType attr) {
  static auto ret = [] {
    unordered_map<AttrType, optional<pair<ExperienceType, int>>, CustomHash<AttrType>> ret;
    for (auto expType : ENUM_ALL(ExperienceType))
      for (auto attr : getAttrIncreases()[expType])
        ret[attr.first] = make_pair(expType, attr.second);
    return ret;
  }();
  return ret[attr];
}
