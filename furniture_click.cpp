#include "stdafx.h"
#include "furniture_click.h"
#include "furniture_factory.h"
#include "furniture_type.h"
#include "furniture.h"
#include "position.h"
#include "tribe.h"
#include "view_object.h"
#include "movement_set.h"
#include "game.h"
#include "collective.h"

void FurnitureClick::handle(FurnitureClickType type, Position pos, WConstFurniture furniture) {
  auto layer = furniture->getLayer();
  switch (type) {
    case FurnitureClickType::LOCK: {
      // Note: the original furniture object is destroyed after this line
      auto f = pos.modFurniture(layer);
      if (f->getMovementSet().hasTrait(MovementTrait::WALK)) {
        f->getViewObject()->setModifier(ViewObject::Modifier::LOCKED);
        f->setBlocking();
      } else {
        f->getViewObject()->setModifier(ViewObject::Modifier::LOCKED, false);
        f->setBlockingEnemies();
      }
      pos.updateConnectivity();
      break;
    }
    case FurnitureClickType::KEEPER_BOARD:
      pos.getGame()->handleMessageBoard(pos, pos.getGame()->getPlayerCollective()->getLeader());
      break;
  }
}

const char* FurnitureClick::getText(FurnitureClickType type, Position, WConstFurniture furniture) {
  switch (type) {
    case FurnitureClickType::LOCK: {
      if (furniture->getMovementSet().hasTrait(MovementTrait::WALK))
        return "Lock door";
      else
        return "Unlock door";
    }
    case FurnitureClickType::KEEPER_BOARD:
      return "Write on board";
  }
}
