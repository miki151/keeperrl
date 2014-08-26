#ifndef _ENUM_VARIANT_H
#define _ENUM_VARIANT_H

#include "serialization.h"

template<typename U, typename Id, Id...value>
struct TypeAssign {
  TypeAssign(const U& u, Id id) {
    assert(hasValue(id));
  }

  template <typename V>
  TypeAssign(const V& u, Id id) {
    assert(!hasValue(id));
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
    boost::apply_visitor(CheckVisitor(id), values);
  }

  EnumVariant(const EnumVariant& other) = default;

  template<typename U>
  EnumVariant(Id i, U u) : id(i), values(u) {
    boost::apply_visitor(CheckVisitor(id), values);
  }

  Id getId() const {
    return id;
  }

  template<typename U>
  const U& get() const {
    return boost::get<U>(values);
  }

  bool operator == (const EnumVariant& other) const {
    return id == other.id && values == other.values;
  }

  bool operator != (const EnumVariant& other) {
    return !(*this == other);
  }

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version) {
    ar & id & values;
  }

  private:
  template<typename T>
  struct CheckId : public Assigns... {
    CheckId(Id id, const T& u) : Assigns(u, id)... {
    }
  };

  struct CheckVisitor : public boost::static_visitor<> {
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

#define TYPES(...) boost::variant<EmptyThing, __VA_ARGS__>
#define ASSIGN(T, ...)\
TypeAssign<T, decltype(__VA_ARGS__), __VA_ARGS__>



#endif
