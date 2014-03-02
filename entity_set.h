#ifndef _ENTITY_SET_H
#define _ENTITY_SET_H

#include "unique_entity.h"
#include "item.h"

class EntitySet {
  public:
  EntitySet();
  template <class Container>
  EntitySet(const Container&);
  EntitySet& operator = (const EntitySet&) = default;
  void insert(const UniqueEntity*);
  void erase(const UniqueEntity*);
  bool contains(const UniqueEntity*) const;
  void insert(UniqueId);
  void erase(UniqueId);
  bool contains(UniqueId) const;
  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version);
  ItemPredicate containsPredicate() const;

  typedef set<UniqueId>::const_iterator Iter;

  Iter begin() const;
  Iter end() const;

  private:
  set<UniqueId> elems;
};

#endif
