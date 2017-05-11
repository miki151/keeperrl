#include "stdafx.h"
#include "furniture_dropped_items.h"
#include "position.h"
#include "item.h"
#include "player_message.h"
#include "furniture.h"

SERIALIZE_DEF(FurnitureDroppedItems, dropData)
SERIALIZATION_CONSTRUCTOR_IMPL(FurnitureDroppedItems)

FurnitureDroppedItems::FurnitureDroppedItems(FurnitureDroppedItems::DropData d) : dropData(d) {

}

vector<PItem> FurnitureDroppedItems::handle(Position pos, WConstFurniture f, vector<PItem> items) const {
  return dropData.visit(
      [&](const Water& info) {
        for (auto& elem : Item::stackItems(getWeakPointers(items))) {
          PlayerMessage message(elem.second[0]->getPluralTheNameAndVerb(elem.second.size(),
              info.verbSingle, info.verbPlural) + " in the " + f->getName());
          if (info.unseenMessage)
            pos.globalMessage(message, *info.unseenMessage);
          else
            pos.globalMessage(message);
        }
        return vector<PItem>();
      }
  );
}
