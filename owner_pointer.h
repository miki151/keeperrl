#pragma once

#include "stdafx.h"
#include "util.h"

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

  void clear() {
    elem.reset();
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

  weak_ptr<T> getWeakPointer() const {
    return weak_ptr<T>(elem);
  }

  SERIALIZE_ALL(elem)

  private:

  template <typename U, typename... Args>
  friend OwnerPointer<U> makeOwner(Args... a);

  template <typename U, typename Subclass, typename... Args>
  friend OwnerPointer<U> makeOwner(Args... a);

  explicit OwnerPointer(shared_ptr<T> t) : elem(t) {}

  shared_ptr<T> SERIAL(elem);
};

template <typename T, typename... Args>
OwnerPointer<T> makeOwner(Args... a) {
  return OwnerPointer<T>(std::make_shared<T>(a...));
}

template <typename T, typename Subclass, typename... Args>
OwnerPointer<T> makeOwner(Args... a) {
  return OwnerPointer<T>(std::make_shared<Subclass>(a...));
}

template<class T>
vector<weak_ptr<T>> getWeakPointers(vector<OwnerPointer<T>>& v) {
  vector<weak_ptr<T>> ret;
  ret.resize(v.size());
  for (auto& el : v)
    ret.push_back(el.get());
  return ret;
}
