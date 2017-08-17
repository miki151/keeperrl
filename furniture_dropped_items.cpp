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
        for (auto& stack : Item::stackItems(getWeakPointers(items))) {
          PlayerMessage message(stack[0]->getPluralTheNameAndVerb(stack.size(),
              info.verbSingle, info.verbPlural) + " in the " + f->getName());
          pos.globalMessage(message);
          if (info.unseenMessage)
            pos.unseenMessage(*info.unseenMessage);
        }
        return vector<PItem>();
      }
  );
}
