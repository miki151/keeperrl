#ifndef _INVENTORY_H
#define _INVENTORY_H

#include <functional>

#include "util.h"
#include "item.h"

class Inventory {
  public:
  void addItem(PItem);
  void addItems(vector<PItem>);
  PItem removeItem(Item* item);
  vector<PItem> removeItems(vector<Item*> items);
  vector<PItem> removeAllItems();

  vector<Item*> getItems() const;
  vector<Item*> getItems(function<bool (Item*)> predicate) const;

  bool hasItem(const Item*) const;
  int size() const;

  bool isEmpty() const;

  SERIALIZATION_DECL(Inventory);

  private:
  vector<PItem> items;
  vector<Item*> itemsCache;
};

#endif
