#pragma once

#include "util.h"
#include "enum_variant.h"
#include "unique_entity.h"
#include "t_string.h"

using CreatureUniqueId = UniqueEntity<Creature>::Id;

#define VARIANT_TYPES_LIST\
  X(CreatureUniqueId, 0)\
  X(TeamId, 1)\
  X(TString, 2)

#define VARIANT_NAME DragContent

#include "gen_variant.h"

#undef VARIANT_TYPES_LIST
#undef VARIANT_NAME


class DragContainer {
  public:
  void put(DragContent content, SGuiElem, Vec2 origin);
  optional<DragContent> pop();
  GuiElem* getGui();
  const optional<DragContent>& getElement() const;
  bool hasElement();
  Vec2 getOrigin();

  private:
  optional<DragContent> content;
  SGuiElem gui;
  Vec2 origin;
};

