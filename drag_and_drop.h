#ifndef _DRAG_AND_DROP_H
#define _DRAG_AND_DROP_H

#include "util.h"
#include "enum_variant.h"
#include "unique_entity.h"

enum class DragContentId {
  CREATURE
};

class DragContent : public EnumVariant<DragContentId, TYPES(UniqueEntity<Creature>::Id),
    ASSIGN(UniqueEntity<Creature>::Id, DragContentId::CREATURE)> {
  using EnumVariant::EnumVariant;
};

class DragContainer {
  public:
  void put(DragContent content, PGuiElem, Vec2 origin);
  optional<DragContent> pop();
  GuiElem* getGui();
  bool hasElement();
  Vec2 getOrigin();

  private:
  optional<DragContent> content;
  PGuiElem gui;
  Vec2 origin;
};

#endif
