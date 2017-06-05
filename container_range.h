#pragma once

#include "stdafx.h"

#include "container_helpers.h"

template <typename T>
void eraseIterators(vector<T>& container, vector<typename vector<T>::iterator> iterators) {
  sort(iterators.begin(), iterators.end(),
       [&](const typename vector<T>::iterator& t1, const typename vector<T>::iterator& t2) {
    return t1 - container.begin() > t2 - container.begin();
  });
  for (auto& iterator : iterators)
    container.removeIndex(iterator - container.begin());
}

template <typename T, typename U>
void eraseIterators(map<T, U>& container, vector<typename map<T, U>::iterator> iterators) {
  for (auto& iterator : iterators)
    container.erase(iterator);
}

template <typename Container>
class ContainerRange {
  public:
  ContainerRange(Container& c) : container(c) {}

  typedef typename Container::value_type Value;

  class Accessor {
    public:

    Accessor(ContainerRange& r, int i, typename Container::iterator it) : range(r), ind(i), iterator(it) {}

    Value& operator * () const {
      return *iterator;
    }

    Value* operator -> () const {
      return &(*iterator);
    }

    int index() const {
      return ind;
    }

    void markToErase() const {
      range.toErase.push_back(iterator);
    }

    private:
    ContainerRange& range;
    int ind;
    typename Container::iterator iterator;
  };

  class Iter {
    public:
    Iter(ContainerRange& r, int i, typename Container::iterator iter) : range(r), index(i), iterator(iter) {}

    Accessor operator* () const {
      return Accessor(range, index, iterator);
    }

    bool operator != (const Iter& other) const {
      return iterator != other.iterator;
    }

    const Iter& operator++ () {
      ++iterator;
      ++index;
      return *this;
    }

    private:
    ContainerRange& range;
    int index;
    typename Container::iterator iterator;
  };

  Iter begin() {
    return Iter(*this, 0, container.begin());
  }

  Iter end() {
    return Iter(*this, container.size(), container.end());
  }

  ~ContainerRange() {
    eraseIterators(container, toErase);
  }

  private:
  friend Accessor;
  Container& container;
  vector<typename Container::iterator> toErase;
};

template <typename Container>
class ConstContainerRange {
  public:
  ConstContainerRange(const Container& c) : container(c) {}

  typedef typename Container::value_type Value;

  class Accessor {
    public:

    Accessor(ConstContainerRange& r, int i, typename Container::const_iterator it) : range(r), ind(i), iterator(it) {}

    const Value& operator * () const {
      return *iterator;
    }

    const Value* operator -> () const {
      return &(*iterator);
    }

    int index() const {
      return ind;
    }

    private:
    ConstContainerRange& range;
    int ind;
    typename Container::const_iterator iterator;
  };

  class Iter {
    public:
    Iter(ConstContainerRange& r, int i, typename Container::const_iterator iter) : range(r), index(i), iterator(iter) {}

    Accessor operator* () const {
      return Accessor(range, index, iterator);
    }

    bool operator != (const Iter& other) const {
      return iterator != other.iterator;
    }

    const Iter& operator++ () {
      ++iterator;
      ++index;
      return *this;
    }

    private:
    ConstContainerRange& range;
    int index;
    typename Container::const_iterator iterator;
  };

  Iter begin() {
    return Iter(*this, 0, container.begin());
  }

  Iter end() {
    return Iter(*this, container.size(), container.end());
  }

  private:
  friend Accessor;
  const Container& container;
};

template <typename Container>
class TempContainerRange {
  public:
  TempContainerRange(Container&& c) : container(std::move(c)) {}

  typedef typename Container::value_type Value;

  class Accessor {
    public:

    Accessor(TempContainerRange& r, int i, typename Container::iterator it) : range(r), ind(i), iterator(it) {}

    const Value& operator * () const {
      return *iterator;
    }

    const Value* operator -> () const {
      return &(*iterator);
    }

    int index() const {
      return ind;
    }

    private:
    TempContainerRange& range;
    int ind;
    typename Container::iterator iterator;
  };

  class Iter {
    public:
    Iter(TempContainerRange& r, int i, typename Container::iterator iter) : range(r), index(i), iterator(iter) {}

    Accessor operator* () const {
      return Accessor(range, index, iterator);
    }

    bool operator != (const Iter& other) const {
      return iterator != other.iterator;
    }

    const Iter& operator++ () {
      ++iterator;
      ++index;
      return *this;
    }

    private:
    TempContainerRange& range;
    int index;
    typename Container::iterator iterator;
  };

  Iter begin() {
    return Iter(*this, 0, container.begin());
  }

  Iter end() {
    return Iter(*this, container.size(), container.end());
  }

  private:
  friend Accessor;
  Container container;
};

template <typename Container>
ContainerRange<Container> Iter(Container& c) {
  return ContainerRange<Container>(c);
}

template <typename Container>
ConstContainerRange<Container> Iter(const Container& c) {
  return ConstContainerRange<Container>(c);
}

template <typename Container>
TempContainerRange<Container> Iter(Container&& c,
    typename std::enable_if<std::is_rvalue_reference<Container&&>::value >::type* = 0,
    typename std::enable_if<!std::is_const<Container>::value>::type* = 0) {
  return TempContainerRange<Container>(std::move(c));
}
