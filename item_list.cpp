#include "stdafx.h"
#include "item_list.h"

ItemList& ItemList::operator = (const ItemList&) = default;

ItemList::ItemList(const ItemList&) = default;

ItemList::~ItemList() {}

SERIALIZE_DEF(ItemList, NAMED(items), OPTION(unique))
SERIALIZATION_CONSTRUCTOR_IMPL(ItemList)

struct ItemList::ItemInfo {
  ItemInfo(ItemType _id, double _weight) : id(_id), weight(_weight), count(1, 2) {}
  ItemInfo(ItemType _id, double _weight, Range c)
    : id(_id), weight(_weight), count(c) {}
  SERIALIZATION_CONSTRUCTOR(ItemInfo)
  ItemType SERIAL(id);
  double SERIAL(weight) = 1;
  Range SERIAL(count) = Range(1, 2);
  SERIALIZE_ALL(NAMED(id), OPTION(weight), OPTION(count))
};

ItemList& ItemList::setRandomPrefixes(double chance) {
  for (auto& item : items)
    item.id.setPrefixChance(chance);
  return *this;
}

vector<PItem> ItemList::random() {
  if (unique.size() > 0) {
    ItemType id = unique.back().first;
    int cnt = Random.get(unique.back().second);
    unique.pop_back();
    return id.get(cnt);
  }
  int index = Random.get(items.transform([](const auto& elem) { return elem.weight; }));
  return items[index].id.get(Random.get(items[index].count));
}

#include "pretty_archive.h"
template<> void ItemList::serialize(PrettyInputArchive& ar1, unsigned) {
  double prefixChance = 0;
  ar1(NAMED(items), OPTION(unique), OPTION(prefixChance));
  ar1 >> endInput();
  setRandomPrefixes(prefixChance);
}
