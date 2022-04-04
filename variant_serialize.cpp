#include "stdafx.h"
#include "serialization.h"
#include "storage_info.h"
#include "position.h"
#include "collective.h"
#include "enum_variant.h"
#include "attack_trigger.h"
#include "creature_group.h"
#include "special_trait.h"
#include "build_info.h"
#include "item_type.h"
#include "immigrant_info.h"
#include "furniture_dropped_items.h"
#include "furniture_entry.h"
#include "effect.h"
#include "campaign.h"
#include "z_level_info.h"
#include "attack_behaviour.h"
#include "tech_id.h"
#include "enemy_info.h"
#include "effect_type.h"
#include "item_types.h"
#include "furniture_usage.h"
#include "furniture_tick.h"

namespace cereal {
  namespace variant_detail {
    template<int N, class Variant, class ... Args, class Archive>
    typename std::enable_if<N == Variant::num_types>::type
    load_variant(Archive & /*ar*/, int /*target*/, Variant & /*variant*/) {
      throw ::cereal::Exception("Error traversing variant during load");
    }

    template<int N, class Variant, class H, class ... T, class Archive>
    typename std::enable_if<N < Variant::num_types, void>::type
    load_variant(Archive & ar1, int target, Variant & variant) {
      if (N == target) {
        H value;
        ar1( CEREAL_NVP_("data", value) );
        variant = Variant(value);
      } else
        load_variant<N+1, Variant, T...>(ar1, target, variant);
    }
  }

  //! Saving for boost::variant
  template <class Archive, typename VariantType1, typename... VariantTypes> inline
  void save(Archive& ar1, variant<VariantType1, VariantTypes...> const & v ) {
    int32_t which = v.index();
    ar1( CEREAL_NVP_("which", which) );
    v.visit([&](const auto& elem) { ar1(elem); });
  }

  //! Loading for boost::variant
  template <class Archive, typename VariantType1, typename... VariantTypes> inline
  void load( Archive & ar1, variant<VariantType1, VariantTypes...> & v )
  {
    int32_t which;
    ar1( CEREAL_NVP_("which", which) );
    if (which >= variant<VariantType1, VariantTypes...>::num_types)
      throw Exception("Invalid 'which' selector when deserializing boost::variant");
    variant_detail::load_variant<0, variant<VariantType1, VariantTypes...>, VariantType1, VariantTypes...>(ar1, which, v);
  }

#ifdef MEM_USAGE_TEST
#define INST(...) \
  template void save(BinaryOutputArchive&, variant<__VA_ARGS__> const&); \
  template void load(BinaryInputArchive&, variant<__VA_ARGS__>&); \
  template void save(MemUsageArchive&, variant<__VA_ARGS__> const&);

#define INST2(V) \
  template void save(BinaryOutputArchive&, V const&); \
  template void load(BinaryInputArchive&, V&); \
  template void save(MemUsageArchive&, V const&);
#else
#define INST(...) \
  template void save(BinaryOutputArchive&, variant<__VA_ARGS__> const&); \
  template void load(BinaryInputArchive&, variant<__VA_ARGS__>&);
#define INST2(V) \
  template void save(BinaryOutputArchive&, V const&); \
  template void load(BinaryInputArchive&, V&);
#endif

  INST(StorageInfo, vector<Position>)
  INST(TechId, int)
  INST2(AttractionType_impl)
  INST2(FurnitureEntry::EntryData_impl)
  INST2(Campaign::SiteInfo::Dweller)
  INST2(ZLevelType_impl)
  INST2(MapLayoutTypes::LayoutType_impl)
  INST2(LevelConnection::EnemyLevelInfo_impl)
  INST2(FurnitureUsageType_impl)
  INST(LastingEffect, BuffId)
} // namespace cereal
#undef INST
#undef INST2
