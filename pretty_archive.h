#pragma once

#include "extern/iomanip.h"
#include "util.h"

class PrettyOutputArchive : public cereal::OutputArchive<PrettyOutputArchive> {
  public:
    PrettyOutputArchive(std::ostream& stream)
          : OutputArchive<PrettyOutputArchive>(this), os(stream) {}

    using base = cereal::OutputArchive<PrettyOutputArchive>;

    template<class... Types>
    PrettyOutputArchive& operator() (Types&&...args) {
      base::operator()(std::forward<Types>(args)...);
      return *this;
    }

    ~PrettyOutputArchive() CEREAL_NOEXCEPT = default;

    std::ostream& os;
};

class PrettyInputArchive : public cereal::InputArchive<PrettyInputArchive> {
  public:
    //! Construct, loading from the provided stream
    PrettyInputArchive(std::istream& stream)
          : InputArchive<PrettyInputArchive>(this), is(stream) {
      // This makes the istream ignore { and } characters
      class my_ctype : public std::ctype<char> {
        mask my_table[table_size];
        public:
         my_ctype(size_t refs = 0) : std::ctype<char>(&my_table[0], false, refs) {
           std::copy_n(classic_table(), table_size, my_table);
           my_table[(int)'{'] = (mask)space;
           my_table[(int)'}'] = (mask)space;
         }
      };
      std::locale x(std::locale::classic(), new my_ctype);
      is.imbue(x);
    }

    ~PrettyInputArchive() CEREAL_NOEXCEPT = default;

    std::istream& is;
};

namespace cereal {
  namespace variant_detail {
    template<int N, class Variant, class ... Args>
    typename std::enable_if<N == Variant::num_types>::type
    load_variant(PrettyInputArchive & /*ar*/, const string& /*target*/, Variant & /*variant*/) {
      throw ::cereal::Exception("Error traversing variant during load");
    }

    template<int N, class Variant, class H, class ... T>
    typename std::enable_if<N < Variant::num_types, void>::type
    load_variant(PrettyInputArchive & ar1, const string& target, Variant & variant) {
      if (variant.getName(N) == target) {
        H value;
        ar1(value);
        variant = Variant(value);
      } else
        load_variant<N+1, Variant, T...>(ar1, target, variant);
    }
  }

  //! Saving for boost::variant
  template <typename VariantType1, const char* Str(), typename... VariantTypes> inline
  void CEREAL_SAVE_FUNCTION_NAME(PrettyOutputArchive& ar1, NamedVariant<Str, VariantType1, VariantTypes...> const & v ) {
    v.visit([&](const auto& elem) {
        using ThisType = typename variant_helpers::bare_type<decltype(elem)>::type;
        ar1.os << v.getName(v.index());
        if (!std::is_empty<ThisType>::value)
          ar1.os << "{";
        ar1.os << ' ';
        ar1(elem);
        if (!std::is_empty<ThisType>::value)
          ar1.os << " } ";
    });
  }

  //! Loading for boost::variant
  template <typename VariantType1, const char* Str(), typename... VariantTypes> inline
  void CEREAL_LOAD_FUNCTION_NAME( PrettyInputArchive & ar, NamedVariant<Str, VariantType1, VariantTypes...> & v )
  {
    string name;
    ar.is >> name;
    variant_detail::load_variant<0, NamedVariant<Str, VariantType1, VariantTypes...>, VariantType1, VariantTypes...>(
        ar, name, v);
  }
} // namespace cereal

namespace cereal {

  //! Saving for enum types
  template <class T> inline
  typename std::enable_if<std::is_enum<T>::value,void>::type
  CEREAL_SAVE_FUNCTION_NAME( PrettyOutputArchive & ar, T const & t)
  {
    ar.os << EnumInfo<T>::getString(t) << " ";
  }

  //! Loading for enum types
  template <class T> inline
  typename std::enable_if<std::is_enum<T>::value, void>::type
  CEREAL_LOAD_FUNCTION_NAME( PrettyInputArchive & ar, T & t)
  {
    string s;
    ar.is >> s;
    t = EnumInfo<T>::fromStringWithException(s);
  }

  template<class T>
  struct specialize<typename std::enable_if<std::is_enum<T>::value, PrettyInputArchive>::type, T, cereal::specialization::non_member_load_save> {};

  template<class T>
  struct specialize<typename std::enable_if<std::is_enum<T>::value, PrettyOutputArchive>::type, T, cereal::specialization::non_member_load_save> {};

}

template<class T> inline
typename std::enable_if<std::is_arithmetic<T>::value && !std::is_enum<T>::value, void>::type
CEREAL_SAVE_FUNCTION_NAME(PrettyOutputArchive& ar, T const& t) {
  ar.os << t << " ";
}

template<class T> inline
typename std::enable_if<std::is_arithmetic<T>::value && !std::is_enum<T>::value, void>::type
CEREAL_LOAD_FUNCTION_NAME(PrettyInputArchive& ar, T& t) {
  ar.is >> t;
}

inline void CEREAL_SAVE_FUNCTION_NAME(PrettyOutputArchive& ar, std::string const& t) {
  ar.os << std::quoted(t) << " ";
}

inline void CEREAL_LOAD_FUNCTION_NAME(PrettyInputArchive& ar, std::string& t) {
  ar.is >> std::quoted(t);
}

inline void CEREAL_LOAD_FUNCTION_NAME(PrettyInputArchive& ar, char& c) {
  string s;
  ar.is >> std::quoted(s);
  c = s.at(0);
}

inline void CEREAL_SAVE_FUNCTION_NAME(PrettyOutputArchive& ar, char c) {
  string s {c};
  ar.os << std::quoted(s);
}

//! Serializing NVP types to binary
template <class Archive, class T> inline
CEREAL_ARCHIVE_RESTRICT(PrettyInputArchive, PrettyOutputArchive)
CEREAL_SERIALIZE_FUNCTION_NAME(Archive& ar1, cereal::NameValuePair<T>& t) {
  if (strcmp(t.name, "cereal_class_version"))
    ar1(t.value);
}
//! Serializing SizeTags to binary
template <class Archive, class T> inline
CEREAL_ARCHIVE_RESTRICT(PrettyInputArchive, PrettyOutputArchive)
CEREAL_SERIALIZE_FUNCTION_NAME(Archive& ar1, cereal::SizeTag<T> & t) {
  ar1(t.size);
}

// register archives for polymorphic support
// Commented out because it causes linker errors in item_type.cpp
//CEREAL_REGISTER_ARCHIVE(PrettyOutputArchive)
//CEREAL_REGISTER_ARCHIVE(PrettyInputArchive)

// tie input and output archives together
CEREAL_SETUP_ARCHIVE_TRAITS(PrettyInputArchive, PrettyOutputArchive)


typedef StreamCombiner<ostringstream, PrettyOutputArchive> PrettyOutput;
typedef StreamCombiner<istringstream, PrettyInputArchive> PrettyInput;
