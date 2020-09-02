#pragma once

#include "stdafx.h"
#include "serialization.h"
#include "debug.h"
#include "my_containers.h"
#include "mem_usage_counter.h"

template <typename T>
class WeakPointer;

template <typename T>
class OwnerPointer {
  public:

  template <typename U>
  OwnerPointer(OwnerPointer<U>&& o) noexcept : elem(std::move(o.elem)) {
  }

  OwnerPointer() noexcept {}
  OwnerPointer(std::nullptr_t) noexcept {}

  shared_ptr<T> giveMeSharedPointer() {
    return elem;
  }

  explicit OwnerPointer(shared_ptr<T> t) noexcept;

  OwnerPointer<T>& operator = (OwnerPointer<T>&& o) noexcept {
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
  bool operator == (const OwnerPointer<U>& o) const noexcept {
    return elem == o.elem;
  }

  template <typename U>
  bool operator != (const OwnerPointer<U>& o) const noexcept {
    return !(*this == o);
  }

  T* get() const;

  explicit operator bool() const {
    return !!elem;
  }

  bool operator !() const {
    return !elem;
  }

  ~OwnerPointer() noexcept {

  }

/*  weak_ptr<T> getWeakPointer() const {
    return weak_ptr<T>(elem);
  }*/

  template <class Archive>
  void serialize(Archive& ar) {
    ar(elem);
  }

#ifdef MEM_USAGE_TEST
  void serialize(MemUsageArchive& ar1, const unsigned int) {
    if (!!elem) {
      ar1.addUsage(sizeof(T));
      ar1(*elem);
    }
  }
#endif

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

  shared_ptr<T> giveMeSharedPointer() {
    return elem.lock();
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

  T* get() const {
    return elem.lock().get();
  }

  int getHash() const {
    auto sp = elem.lock();
    return std::hash<decltype(sp)>()(sp);
  }

  SERIALIZE_ALL(elem)

  private:
  template<class U>
  friend std::ostream& operator<<(std::ostream&, const WeakPointer<U>&);

  template <typename>
  friend class OwnerPointer;
  template <typename>
  friend class WeakPointer;
  WeakPointer(const shared_ptr<T>& e) : elem(e) {}

  weak_ptr<T> SERIAL(elem);
};

template<typename T>
bool operator == (const T* o, const WeakPointer<T>& p) {
  return p.get() == o;
}


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

  template <class Archive>
  void serialize(Archive& ar, const unsigned int) {
    ar(weakPointer);
    CHECK(!!weakPointer);
  }

  private:
  template <typename>
  friend class OwnerPointer;
  WeakPointer<T> SERIAL(weakPointer);
};

template <typename T>
OwnerPointer<T>::OwnerPointer(shared_ptr<T> t) noexcept : elem(t) {
  elem->weakPointer = WeakPointer<T>(elem);
}

template <typename T>
T* OwnerPointer<T>::get() const {
  return elem.get();
}

template <typename T, typename... Args>
OwnerPointer<T> makeOwner(Args&&... args) {
  return OwnerPointer<T>(std::make_shared<T>(std::forward<Args>(args)...));
}

template<class T>
auto getWeakPointers(const vector<OwnerPointer<T>>& v) {
  vector<decltype(v[0].get())> ret;
  ret.reserve(v.size());
  for (auto& el : v)
    ret.push_back(el.get());
  return ret;
}
namespace cereal
{

template <typename Archive, typename T>
inline void CEREAL_LOAD_FUNCTION_NAME(Archive& ar1, T*& m) {
  WeakPointer<T> wptr;
  ar1(wptr);
  m = wptr.get();
}


template <typename Archive, typename T>
inline void CEREAL_SAVE_FUNCTION_NAME(Archive& ar1, T* m) {
  WeakPointer<T> wptr;
  if (m)
    wptr = m->getThis().template dynamicCast<T>();
  ar1(wptr);
}
}
#define DEF_UNIQUE_PTR(T) class T;\
  typedef unique_ptr<T> P##T

#define DEF_SHARED_PTR(T) class T;\
  typedef shared_ptr<T> S##T

#define DEF_OWNER_PTR(T) class T;\
  typedef OwnerPointer<T> P##T; \
  typedef T* W##T; \
  typedef T const* WConst##T

DEF_OWNER_PTR(Item);
DEF_UNIQUE_PTR(LevelMaker);
DEF_OWNER_PTR(Creature);
DEF_UNIQUE_PTR(Square);
DEF_UNIQUE_PTR(Furniture);
DEF_UNIQUE_PTR(MonsterAI);
DEF_UNIQUE_PTR(Behaviour);
DEF_OWNER_PTR(Task);
DEF_OWNER_PTR(TaskPredicate);
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
DEF_OWNER_PTR(PositionMatching);
DEF_SHARED_PTR(UnknownLocations);
DEF_SHARED_PTR(CreatureFactory);
