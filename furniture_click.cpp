#include "stdafx.h"
#include "furniture_click.h"
#include "furniture_factory.h"
#include "furniture_type.h"
#include "furniture.h"
#include "position.h"
#include "tribe.h"
#include "view_object.h"
#include "movement_set.h"

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
  }
}
