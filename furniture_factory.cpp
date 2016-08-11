#include "stdafx.h"
#include "furniture_factory.h"
#include "furniture.h"
#include "furniture_type.h"
#include "view_id.h"
#include "view_layer.h"
#include "view_object.h"

static Furniture* get(FurnitureType type) {
  switch (type) {
    case FurnitureType::TRAINING_DUMMY:
      return new Furniture("Training dummy", ViewObject(ViewId::TRAINING_ROOM, ViewLayer::FLOOR),
          Furniture::BLOCKING);
    case FurnitureType::WORKSHOP:
      return new Furniture("Workshop", ViewObject(ViewId::WORKSHOP, ViewLayer::FLOOR),
          Furniture::BLOCKING);
    case FurnitureType::FORGE:
      return new Furniture("Forge", ViewObject(ViewId::FORGE, ViewLayer::FLOOR),
          Furniture::BLOCKING);
    case FurnitureType::LABORATORY:
      return new Furniture("Laboratory", ViewObject(ViewId::LABORATORY, ViewLayer::FLOOR),
          Furniture::BLOCKING);
    case FurnitureType::JEWELER:
      return new Furniture("Jeweler", ViewObject(ViewId::JEWELER, ViewLayer::FLOOR),
          Furniture::BLOCKING);
    case FurnitureType::BOOK_SHELF:
      return new Furniture("Book shelf", ViewObject(ViewId::LIBRARY, ViewLayer::FLOOR),
          Furniture::BLOCKING);
    case FurnitureType::THRONE:
      return new Furniture("Throne", ViewObject(ViewId::THRONE, ViewLayer::FLOOR),
          Furniture::NON_BLOCKING);
    case FurnitureType::IMPALED_HEAD:
      return new Furniture("Impaled head", ViewObject(ViewId::IMPALED_HEAD, ViewLayer::FLOOR),
          Furniture::NON_BLOCKING);
    case FurnitureType::BEAST_CAGE:
      return new Furniture("Beast cage", ViewObject(ViewId::BEAST_CAGE, ViewLayer::FLOOR),
          Furniture::NON_BLOCKING);
    case FurnitureType::BED:
      return new Furniture("Bed", ViewObject(ViewId::BED, ViewLayer::FLOOR),
          Furniture::NON_BLOCKING);
    case FurnitureType::GRAVE:
      return new Furniture("Grave", ViewObject(ViewId::GRAVE, ViewLayer::FLOOR),
          Furniture::NON_BLOCKING);
    case FurnitureType::DEMON_SHRINE:
      return new Furniture("Demon shrine", ViewObject(ViewId::RITUAL_ROOM, ViewLayer::FLOOR),
          Furniture::BLOCKING);
    case FurnitureType::STOCKPILE_RES:
      return new Furniture("Resource stockpile", ViewObject(ViewId::STOCKPILE1, ViewLayer::FLOOR),
          Furniture::NON_BLOCKING);
    case FurnitureType::STOCKPILE_EQUIP:
      return new Furniture("Equipment stockpile", ViewObject(ViewId::STOCKPILE2, ViewLayer::FLOOR),
          Furniture::NON_BLOCKING);
    case FurnitureType::TREASURE_CHEST:
      return new Furniture("Treasure chest", ViewObject(ViewId::TREASURE_CHEST, ViewLayer::FLOOR),
          Furniture::NON_BLOCKING);
  }
}

PFurniture FurnitureFactory::get(FurnitureType type) {
  return PFurniture(::get(type));
}
