#include "stdafx.h"
#include "furniture_click.h"
#include "furniture_factory.h"
#include "furniture_type.h"
#include "furniture.h"
#include "position.h"
#include "tribe.h"

void FurnitureClick::handle(FurnitureClickType type, Position pos, const Furniture* furniture) {
  switch (type) {
    case FurnitureClickType::LOCK:
      pos.replaceFurniture(furniture, FurnitureFactory::get(FurnitureType::LOCKED_DOOR, furniture->getTribe()));
      break;
    case FurnitureClickType::UNLOCK:
      pos.replaceFurniture(furniture, FurnitureFactory::get(FurnitureType::DOOR, furniture->getTribe()));
    break;
  }
}
