#pragma once

#include "stdafx.h"

#include "container_helpers.h"

template <typename Container>
class ContainerRange {
  public:
  ContainerRange(Container& c) : container(c) {}

  typedef typename Container::value_type Value;

  class Accessor {
    public:

    Accessor(ContainerRange& r, int i) : range(r), ind(i) {}

    Value& operator * () const {
      return range.container[ind];
    }

    Value* operator -> () const {
      return &range.container[ind];
    }

    int index() const {
      return ind;
    }

    void markToErase() const {
      range.toErase.push_back(ind);
    }

    private:
    ContainerRange& range;
    int ind;
  };

  class Iter {
    public:
    Iter(ContainerRange& r, int i) : range(r), ind(i) {}

    Accessor operator* () const {
      return Accessor(range, ind);
    }

    bool operator != (const Iter& other) const {
      return ind != other.ind;
    }

    const Iter& operator++ () {
      ++ind;
      return *this;
    }

    private:
    ContainerRange& range;
    int ind;
  };

  Iter begin() {
    return Iter(*this, 0);
  }

  Iter end() {
    return Iter(*this, container.size());
  }

  ~ContainerRange() {
    sort(toErase.begin(), toErase.end());
    for (int i = toErase.size() - 1; i >= 0; --i)
      removeIndex(container, toErase[i]);
  }

  private:
  friend Accessor;
  Container& container;
  vector<int> toErase;
};

template <typename Container>
class ConstContainerRange {
  public:
  ConstContainerRange(const Container& c) : container(c) {}

  typedef typename Container::value_type Value;

  class Accessor {
    public:

    Accessor(ConstContainerRange& r, int i) : range(r), ind(i) {}

    const Value& operator * () const {
      return range.container[ind];
    }

    const Value* operator -> () const {
      return &range.container[ind];
    }

    int index() const {
      return ind;
    }

    private:
    ConstContainerRange& range;
    int ind;
  };

  class Iter {
    public:
    Iter(ConstContainerRange& r, int i) : range(r), ind(i) {}

    Accessor operator* () const {
      return Accessor(range, ind);
    }

    bool operator != (const Iter& other) const {
      return ind != other.ind;
    }

    const Iter& operator++ () {
      ++ind;
      return *this;
    }

    private:
    ConstContainerRange& range;
    int ind;
  };

  Iter begin() {
    return Iter(*this, 0);
  }

  Iter end() {
    return Iter(*this, container.size());
  }

  private:
  friend Accessor;
  const Container& container;
};

template <typename Container>
ContainerRange<Container> Iter(Container& c) {
  return ContainerRange<Container>(c);
}

template <typename Container>
ConstContainerRange<Container> Iter(const Container& c) {
  return ConstContainerRange<Container>(c);
}
