#pragma once

#include "variant.h"
#include <cereal/cereal.hpp>


namespace cereal {

  //! Saving for boost::variant
  template <class Archive, typename VariantType1, typename... VariantTypes>
  void CEREAL_SAVE_FUNCTION_NAME(Archive& ar, variant<VariantType1, VariantTypes...> const & v );

  //! Loading for boost::variant
  template <class Archive, typename VariantType1, typename... VariantTypes>
  void CEREAL_LOAD_FUNCTION_NAME( Archive & ar, variant<VariantType1, VariantTypes...> & v );
} // namespace cereal
