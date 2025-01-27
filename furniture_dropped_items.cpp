#include "stdafx.h"
#include "furniture_dropped_items.h"
#include "position.h"
#include "item.h"
#include "player_message.h"
#include "furniture.h"
#include "game.h"

SERIALIZE_DEF(FurnitureDroppedItems, dropData)
SERIALIZATION_CONSTRUCTOR_IMPL(FurnitureDroppedItems)

FurnitureDroppedItems::FurnitureDroppedItems(FurnitureDroppedItems::DropData d) : dropData(d) {

}

vector<PItem> FurnitureDroppedItems::handle(Position pos, const Furniture* f, vector<PItem> items) const {
  for (auto& stack : Item::stackItems(pos.getGame()->getContentFactory(), getWeakPointers(items))) {
    (stack.size() == 1 ? dropData.verbSingle : dropData.verbPlural).text.visit(
      [&](const string& s) {
      },
      [&](TSentence s) {
        s.params.push_back(stack[0]->getPluralTheName(stack.size()));
        s.params.push_back(f->getName());
        pos.globalMessage(std::move(s));
      }
    );
    if (dropData.unseenMessage)
      pos.unseenMessage(*dropData.unseenMessage);
  }
  return vector<PItem>();
}

#include "pretty_archive.h"
template void FurnitureDroppedItems::serialize(PrettyInputArchive&, unsigned);
