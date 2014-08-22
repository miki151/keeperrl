#ifndef _ENUM_VARIANT_H
#define _ENUM_VARIANT_H

#include "serialization.h"

template<typename U, typename Id, Id...ids>
struct TypeAssign {
  TypeAssign() {}

  TypeAssign(Id id) {
    CHECK(!containsId(id));
  }

  template<class V>
  TypeAssign<V>(V v, Id id) {
    CHECK(!containsId(id));
  }

  TypeAssign(U u, Id id) {
    CHECK(containsId(id));
  }

  TypeAssign(const boost::any& a1, const boost::any& a2, function<void()> eqFun) {
    if (a1.type() == typeid(U) && a2.type() == typeid(U) && boost::any_cast<U>(a1) == boost::any_cast<U>(a2)) {
      eqFun();
    }
  }

  TypeAssign(boost::archive::binary_oarchive& ar, Id, boost::any& a) {
    if (a.type() == typeid(U))
      ar & (*boost::any_cast<U>(&a));
  }

  TypeAssign(boost::archive::binary_iarchive& ar, Id id, boost::any& a) {
    if (containsId(id)) {
      a = U();
      ar & (*boost::any_cast<U>(&a));
    }
  }

  static bool containsId(Id id) {
    return contains({ids...}, id);
  }
};

template<typename... Assigns>
struct AnyCompare : public Assigns... {
  AnyCompare(const boost::any& a1, const boost::any& a2, function<void()> eqFun) : Assigns(a1, a2, eqFun)... {}
};

template<typename Archive, typename Id, typename... Assigns>
struct AnySerialize : public Assigns... {
  AnySerialize(Archive& ar, Id id, boost::any& value) : Assigns(ar, id, value)... {}
};

template<typename Id, typename... Assigns>
class EnumVariant : public Assigns... {
  public:
  EnumVariant() {}
  EnumVariant(Id i) : Assigns(i)..., id(i) {
  }

  EnumVariant(const EnumVariant& other) = default;

  template<typename U>
  EnumVariant(Id i, U u) : Assigns(u, i)..., id(i), value(u) {
  }

  Id getId() const {
    return id;
  }

  template<typename U>
  U get() {
    return boost::any_cast<U>(value);
  }

  bool operator == (const EnumVariant& other) const {
    if (id != other.id)
      return false;
    if (value.empty() && other.value.empty())
      return true;
    bool yes = false;
    AnyCompare<Assigns...>(value, other.value, [&]() { yes = true;});
    return yes;
  }

  bool operator != (const EnumVariant& other) {
    return !(*this == other);
  }

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version) {
    ar & id;
    AnySerialize<Archive, Id, Assigns...>(ar, id, value);
  }

  private:
  Id id;
  boost::any value;
};

#define ASSIGN(T, ...)\
TypeAssign<T, decltype(__VA_ARGS__), __VA_ARGS__>



#endif
