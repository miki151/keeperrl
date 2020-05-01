#pragma once

#include "serialization.h"
#include "debug.h"

template<typename U, typename Id, Id...value>
struct TypeAssign {
  TypeAssign(const U& u, Id id) {
    CHECK(hasValue(id));
  }

  template <typename V>
  TypeAssign(const V& u, Id id) {
    CHECK(!hasValue(id));
  }

  bool hasValue(Id id) {
    for (Id i : {value...})
      if (i == id)
        return true;
    return false;
  }
};

struct EmptyThing {
  bool operator == (const EmptyThing& other) const {
    return true;
  }
  template <class Archive>
  void serialize(Archive& ar, const unsigned int version) {
  }
};

template<typename Id, typename Variant, typename... Assigns>
class EnumVariant {
  public:
  EnumVariant() {}
  EnumVariant(Id i) : id(i) {
    NO_RELEASE(values.visit(CheckVisitor(id)));
  }

  EnumVariant(const EnumVariant& other) = default;
  EnumVariant(EnumVariant&& other) = default;
  EnumVariant& operator = (const EnumVariant& other) = default;
  EnumVariant& operator = (EnumVariant&& other) = default;

  template<typename U>
  EnumVariant(Id i, U u) : id(i), values(u) {
    NO_RELEASE(values.visit(CheckVisitor(id)));
  }

  Id getId() const {
    return id;
  }

  template<typename U>
  const U& get() const {
    return *values.template getReferenceMaybe<U>();
  }

  bool operator == (const EnumVariant& other) const {
    return id == other.id && values == other.values;
  }

  bool operator != (const EnumVariant& other) const {
    return !(*this == other);
  }

  private:
  template<typename T>
  struct CheckId : public Assigns... {
    CheckId(Id id, const T& u) : Assigns(u, id)... {
    }
  };

  struct CheckVisitor {
    public:
    CheckVisitor(Id i) : id(i) {}
    template<typename T>
    void operator() (const T& t) const {
       CheckId<T> tmp(id, t);
    }
    Id id;
  };

  Id id;
  Variant values;
};

#define FIRST(first, ...) first
#define TYPES(...) variant<EmptyThing, __VA_ARGS__>
#define ASSIGN(T, ...)\
TypeAssign<T, decltype(FIRST(__VA_ARGS__)), __VA_ARGS__>



