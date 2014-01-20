#include "stdafx.h"

#include "inventory.h"

using namespace std;

void Inventory::addItem(PItem item) {
  itemsCache.push_back(item.get());
  items.push_back(move(item));
}

void Inventory::addItems(vector<PItem> v) {
  for (PItem& it : v)
    addItem(std::move(it));
}

PItem Inventory::removeItem(Item* itemRef) {
  int ind = -1;
  for (int i : All(items))
    if (items[i].get() == itemRef) {
      ind = i;
      break;
    }
  CHECK(ind > -1) << "Tried to remove unknown item.";
  PItem item = std::move(items[ind]);
  items.erase(items.begin() + ind);
  removeElement(itemsCache, itemRef);
  return item;
}

vector<PItem> Inventory::removeItems(vector<Item*> items) {
  vector<PItem> ret;
  for (Item* item : items)
    ret.push_back(removeItem(item));
  return ret;
}

vector<PItem> Inventory::removeAllItems() {
  itemsCache.clear();
  return move(items);
}

vector<Item*> Inventory::getItems(function<bool (Item*)> predicate) const {
  vector<Item*> ret;
  for (const PItem& item : items)
    if (predicate(item.get()))
      ret.push_back(item.get());
  return ret;
}

vector<Item*> Inventory::getItems() const {
  return itemsCache;
}

bool Inventory::hasItem(const Item* itemRef) const {
  for (const PItem& item : items) 
    if (item.get() == itemRef)
      return true;
  return false;
}

int Inventory::size() const {
  return items.size();
}

bool Inventory::isEmpty() const {
  return items.empty();
}

