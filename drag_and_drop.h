#pragma once

#include "util.h"
#include "enum_variant.h"
#include "unique_entity.h"

enum class DragContentId {
  CREATURE,
  CREATURE_GROUP,
  TEAM
};

class DragContent : public EnumVariant<DragContentId, TYPES(UniqueEntity<Creature>::Id, TeamId),
    ASSIGN(UniqueEntity<Creature>::Id, DragContentId::CREATURE, DragContentId::CREATURE_GROUP),
    ASSIGN(TeamId, DragContentId::TEAM)> {
  using EnumVariant::EnumVariant;
};

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

