#pragma once

#include "stdafx.h"
#include "util.h"

template <typename T>
class OwnerPointer;

template <typename T>
class WeakPointer {
  public:

  WeakPointer() {}

  WeakPointer(const OwnerPointer<T>& p) : elem(p.elem) {}
  WeakPointer(std::nullptr_t) {}
  WeakPointer(const WeakPointer<T>& o) : elem(o.elem) {}

  WeakPointer<T>& operator = (const WeakPointer<T>& o) {
    elem = o.elem;
    return *this;
  }

  T* operator -> () const {
    return elem.lock().get();
  }

  T& operator * () const {
    return *elem.lock();
  }

  T* get() const {
    return elem.lock().get();
  }

  operator bool() const {
    return !elem.expired();
  }

  bool operator !() const {
    return elem.expired();
  }

  private:
  weak_ptr<T> elem;
};

template <typename T>
class OwnerPointer {
  public:

  OwnerPointer(OwnerPointer<T>&& o) : elem(std::move(o.elem)) {
    o.elem.reset();
  }

  OwnerPointer() {}
  OwnerPointer(std::nullptr_t) {}

  OwnerPointer<T>& operator = (OwnerPointer<T>&& o) {
    elem = std::move(o.elem);
    return *this;
  }

  T* operator -> () const {
    return elem.get();
  }

  T& operator * () const {
    return *elem;
  }

  T* get() const {
    return elem.get();
  }

  operator bool() const {
    return !!elem;
  }

  bool operator !() const {
    return !elem;
  }

  SERIALIZE_ALL(elem)

  private:
  friend class WeakPointer<T>;

  template <typename U, typename... Args>
  friend OwnerPointer<U> makeOwner(Args... a);

  template <typename U, typename Subclass, typename... Args>
  friend OwnerPointer<U> makeOwner(Args... a);

  explicit OwnerPointer(shared_ptr<T> t) : elem(t) {}

  shared_ptr<T> SERIAL(elem);
};

template <typename T, typename... Args>
OwnerPointer<T> makeOwner(Args... a) {
  return OwnerPointer<T>(make_shared<T>(a...));
}

template <typename T, typename Subclass, typename... Args>
OwnerPointer<T> makeOwner(Args... a) {
  return OwnerPointer<T>(make_shared<Subclass>(a...));
}

template<class T>
vector<WeakPointer<T>> getWeakPointers(vector<OwnerPointer<T>>& v) {
  vector<WeakPointer<T>> ret;
  ret.resize(v.size());
  for (auto& el : v)
    ret.push_back(el.get());
  return ret;
}
