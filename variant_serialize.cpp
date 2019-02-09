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

#define INST(...) \
  template void save(BinaryOutputArchive&, variant<__VA_ARGS__> const&); \
  template void load(BinaryInputArchive&, variant<__VA_ARGS__>&);

#define INST2(V) \
  template void save(BinaryOutputArchive&, V const&); \
  template void load(BinaryInputArchive&, V&);

  INST(StorageInfo, vector<Position>)
  INST(EmptyThing, int, RoomTriggerInfo)
  INST(EmptyThing, int, CreatureGroup)
  INST2(SpecialTrait_impl)
  INST2(BuildInfo::BuildType_impl)
  INST(string, int)
  INST2(ItemType::Type_impl)
  INST2(ImmigrantRequirement_impl)
  INST2(SpawnLocation_impl)
  INST2(AttractionType_impl)
  INST2(ItemPrefix_impl)
  INST2(FurnitureDroppedItems::DropData)
  INST2(FurnitureEntry::EntryData)
  INST2(Effect::EffectType_impl)
  INST2(Campaign::SiteInfo::Dweller)
  INST2(ZLevelType_impl)
} // namespace cereal
