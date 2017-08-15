#pragma once

#include "variant.h"
#include <cereal/cereal.hpp>


namespace cereal {
  namespace variant_detail {
    template<int N, class Variant, class ... Args, class Archive>
    typename std::enable_if<N == Variant::num_types>::type
    load_variant(Archive & /*ar*/, int /*target*/, Variant & /*variant*/) {
      throw ::cereal::Exception("Error traversing variant during load");
    }

    template<int N, class Variant, class H, class ... T, class Archive>
    typename std::enable_if<N < Variant::num_types, void>::type
    load_variant(Archive & ar, int target, Variant & variant) {
      if (N == target) {
        H value;
        ar( CEREAL_NVP_("data", value) );
        variant = value;
      } else
        load_variant<N+1, Variant, T...>(ar, target, variant);
    }
  }

  //! Saving for boost::variant
  template <class Archive, typename VariantType1, typename... VariantTypes> inline
  void CEREAL_SAVE_FUNCTION_NAME(Archive& ar, variant<VariantType1, VariantTypes...> const & v ) {
    int32_t which = v.index();
    ar( CEREAL_NVP_("which", which) );
    v.visit([&](const auto& elem) { ar(elem); });
  }

  //! Loading for boost::variant
  template <class Archive, typename VariantType1, typename... VariantTypes> inline
  void CEREAL_LOAD_FUNCTION_NAME( Archive & ar, variant<VariantType1, VariantTypes...> & v )
  {
    int32_t which;
    ar( CEREAL_NVP_("which", which) );
    if (which >= variant<VariantType1, VariantTypes...>::num_types)
      throw Exception("Invalid 'which' selector when deserializing boost::variant");
    variant_detail::load_variant<0, variant<VariantType1, VariantTypes...>, VariantType1, VariantTypes...>(ar, which, v);
  }
} // namespace cereal
