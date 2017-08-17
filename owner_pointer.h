#pragma once

#include "stdafx.h"
#include "serialization.h"
#include "debug.h"
#include "my_containers.h"

template <typename T>
class WeakPointer;

template <typename T>
class OwnerPointer {
  public:

  template <typename U>
  OwnerPointer(OwnerPointer<U>&& o) : elem(std::move(o.elem)) {
  }

  OwnerPointer() {}
  OwnerPointer(std::nullptr_t) {}

  shared_ptr<T> giveMeSharedPointer() {
    return elem;
  }

  explicit OwnerPointer(shared_ptr<T> t);

  OwnerPointer<T>& operator = (OwnerPointer<T>&& o) {
    elem = std::move(o.elem);
    return *this;
  }

  void clear() {
    elem.reset();
  }

  T* operator -> () const {
    return elem.get();
  }

  T& operator * () const {
    return *elem;
  }

  template <typename U>
  bool operator == (const OwnerPointer<U>& o) const {
    return elem == o.elem;
  }

  template <typename U>
  bool operator != (const OwnerPointer<U>& o) const {
    return !(*this == o);
  }

  WeakPointer<T> get() const;

  explicit operator bool() const {
    return !!elem;
  }

  bool operator !() const {
    return !elem;
  }

/*  weak_ptr<T> getWeakPointer() const {
    return weak_ptr<T>(elem);
  }*/

  SERIALIZE_ALL(elem)

  private:
  template <typename>
  friend class OwnerPointer;

  /*template <typename U, typename... Args>
  friend OwnerPointer<U> makeOwner(Args&&... a);*/


  shared_ptr<T> SERIAL(elem);
};

template <typename T>
class WeakPointer {
  public:

  template <typename U>
  WeakPointer(const WeakPointer<U>& o) : elem(o.elem) {
  }

  template <typename U>
  WeakPointer(WeakPointer<U>&& o) : elem(std::move(o.elem)) {
  }

  WeakPointer(T* t) : WeakPointer(t->getThis().template dynamicCast<T>()) {}

  WeakPointer() {}
  WeakPointer(std::nullptr_t) {}

  template <typename U>
  WeakPointer<T>& operator = (WeakPointer<U>&& o) {
    elem = std::move(o.elem);
    return *this;
  }

  template <typename U>
  WeakPointer<T>& operator = (const WeakPointer<U>& o) {
    elem = o.elem;
    return *this;
  }

  WeakPointer<T>& operator = (std::nullptr_t) {
    elem.reset();
    return *this;
  }

  template <typename U>
  WeakPointer<U> dynamicCast() {
    return WeakPointer<U>(std::dynamic_pointer_cast<U>(elem.lock()));
  }

  using NoConst = typename std::remove_const<T>::type;

  WeakPointer<NoConst> removeConst() {
    return WeakPointer<NoConst>(std::const_pointer_cast<NoConst>(elem.lock()));
  }

  void clear() {
    elem.reset();
  }

  T* operator -> () const {
    return elem.lock().get();
  }

  T& operator * () const {
    return *elem.lock().get();
  }

  explicit operator bool() const {
    return !!elem.lock();
  }

  bool operator !() const {
    return !elem.lock();
  }

  template <typename U>
  bool operator == (const WeakPointer<U>& o) const {
    return get() == o.get();
  }

  template <typename U>
  bool operator != (const WeakPointer<U>& o) const {
    return !(*this == o);
  }

  bool operator == (const T* o) const {
    return get() == o;
  }

  bool operator != (const T* o) const {
    return !(*this == o);
  }

  bool operator == (std::nullptr_t) const {
    return !elem.lock();
  }

  bool operator != (std::nullptr_t) const {
    return !!elem.lock();
  }

  SERIALIZE_ALL(elem)

  private:
  template<class U>
  friend std::ostream& operator<<(std::ostream&, const WeakPointer<U>&);

  T* get() const {
    return elem.lock().get();
  }

  template <typename>
  friend class OwnerPointer;
  template <typename>
  friend class WeakPointer;
  WeakPointer(const shared_ptr<T>& e) : elem(e) {}

  weak_ptr<T> SERIAL(elem);
};

template<class T>
std::ostream& operator<<(std::ostream& d, const WeakPointer<T>& p){
  d << "pointer(" << p.get() << ")";
  return d;
}

template <typename T>
class OwnedObject {
  public:
  WeakPointer<T> getThis() {
    CHECK(!!weakPointer);
    return weakPointer;
  }

  WeakPointer<const T> getThis() const {
    CHECK(!!weakPointer);
    return weakPointer;
  }

  SERIALIZE_ALL(weakPointer)

  private:
  template <typename>
  friend class OwnerPointer;
  WeakPointer<T> SERIAL(weakPointer);
};

template <typename T>
OwnerPointer<T>::OwnerPointer(shared_ptr<T> t) : elem(t) {
  elem->weakPointer = WeakPointer<T>(elem);
}

template <typename T>
WeakPointer<T> OwnerPointer<T>::get() const {
  return WeakPointer<T>(elem);
}

template <typename T, typename... Args>
OwnerPointer<T> makeOwner(Args&&... args) {
  return OwnerPointer<T>(std::make_shared<T>(std::forward<Args>(args)...));
}

template<class T>
vector<WeakPointer<T>> getWeakPointers(const vector<OwnerPointer<T>>& v) {
  vector<WeakPointer<T>> ret;
  ret.reserve(v.size());
  for (auto& el : v)
    ret.push_back(el.get());
  return ret;
}

#define DEF_UNIQUE_PTR(T) class T;\
  typedef unique_ptr<T> P##T;

#define DEF_SHARED_PTR(T) class T;\
  typedef shared_ptr<T> S##T;

#define DEF_OWNER_PTR(T) class T;\
  typedef OwnerPointer<T> P##T; \
  typedef WeakPointer<T> W##T; \
  typedef WeakPointer<const T> WConst##T;

DEF_OWNER_PTR(Item);
DEF_UNIQUE_PTR(LevelMaker);
DEF_OWNER_PTR(Creature);
DEF_OWNER_PTR(Square);
DEF_OWNER_PTR(Furniture);
DEF_UNIQUE_PTR(MonsterAI);
DEF_UNIQUE_PTR(Behaviour);
DEF_OWNER_PTR(Task);
DEF_OWNER_PTR(Controller);
DEF_OWNER_PTR(Level);
DEF_OWNER_PTR(VillageControl);
DEF_SHARED_PTR(GuiElem);
DEF_UNIQUE_PTR(Animation);
DEF_UNIQUE_PTR(ViewObject);
DEF_OWNER_PTR(Collective);
DEF_OWNER_PTR(CollectiveControl);
DEF_OWNER_PTR(PlayerControl);
DEF_OWNER_PTR(Model);
DEF_UNIQUE_PTR(Tribe);
DEF_SHARED_PTR(Tutorial);
DEF_OWNER_PTR(CreatureVision);
DEF_OWNER_PTR(Game);
DEF_SHARED_PTR(MapMemory);
DEF_SHARED_PTR(MessageBuffer);
DEF_SHARED_PTR(VisibilityMap);
DEF_OWNER_PTR(Immigration);
