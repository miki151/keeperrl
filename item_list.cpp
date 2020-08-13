#include "stdafx.h"
#include "item_list.h"
#include "item_list_id.h"

ItemList& ItemList::operator = (const ItemList&) = default;

ItemList::ItemList(const ItemList&) = default;
ItemList::ItemList(ItemList&&) noexcept = default;

ItemList::~ItemList() {}

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

struct ItemList::MultiItemInfo {
  Range SERIAL(count) = Range(1, 2);
  vector<pair<ItemType, double>> SERIAL(items);
  SERIALIZE_ALL(count, items)
};

ItemList& ItemList::setRandomPrefixes(double chance) {
  for (auto& item : items)
    item.id.setPrefixChance(chance);
  return *this;
}

ItemList::ItemList(vector<ItemInfo> t) : items(std::move(t)) {}
ItemList::ItemList(vector<ItemType> t) : ItemList(t.transform([](const auto& t) { return ItemInfo(t, 1); })) {}

vector<ItemType> ItemList::getAllItems() const {
  vector<ItemType> ret;
  for (auto& item : items)
    ret.push_back(item.id);
  for (auto& elem : unique)
    ret.push_back(elem.first);
  for (auto& elem : multiItems)
    for (auto& item : elem.items)
      ret.push_back(item.first);
  return ret;
}

vector<PItem> ItemList::random(const ContentFactory* factory) & {
  while (!multiItems.empty() && multiItems.back().count == Range::singleElem(0))
    multiItems.pop_back();
  if (!multiItems.empty()) {
    auto& item = multiItems.back();
    if (item.count.getLength() > 1)
      item.count = Range::singleElem(Random.get(item.count));
    if (item.count.getStart() > 0) {
      item.count = Range::singleElem(item.count.getStart() - 1);
      vector<PItem> res;
      for (auto& id : item.items)
        if (Random.chance(id.second))
          res.push_back(id.first.get(factory));
      return res;
    }
  }
  if (unique.size() > 0) {
    ItemType id = unique.back().first;
    int cnt = Random.get(unique.back().second);
    unique.pop_back();
    return id.get(cnt, factory);
  }
  if (items.empty())
    return {};
  int index = Random.get(items.transform([](const auto& elem) { return elem.weight; }));
  return items[index].id.get(Random.get(items[index].count), factory);
}

SERIALIZE_DEF(ItemList, OPTION(items), OPTION(unique), OPTION(multiItems))
SERIALIZATION_CONSTRUCTOR_IMPL(ItemList)

#include "pretty_archive.h"
template<> void ItemList::serialize(PrettyInputArchive& ar1, unsigned) {
  double prefixChance = 0;
  ar1(OPTION(items), OPTION(unique), OPTION(multiItems), OPTION(prefixChance));
  ar1(endInput());
  setRandomPrefixes(prefixChance);
}
