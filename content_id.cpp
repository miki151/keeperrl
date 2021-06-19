#include "stdafx.h"
#include <cassert>
#include "content_id.h"
#include "furniture_type.h"
#include "furniture_list_id.h"
#include "item_list_id.h"
#include "enemy_id.h"
#include "spell_id.h"
#include "tech_id.h"
#include "creature_id.h"
#include "spell_school_id.h"
#include "custom_item_id.h"
#include "building_id.h"
#include "name_generator_id.h"
#include "map_layout_id.h"
#include "random_layout_id.h"
#include "layout_mapping_id.h"
#include "biome_id.h"
#include "workshop_type.h"
#include "resource_id.h"
#include "storage_id.h"

static const char* staticsInitialized = nullptr;

void setInitializedStatics() {
  staticsInitialized = "initialized";
}


template<typename T>
vector<string>& ContentId<T>::getAllIds() {
  static vector<string> ret;
  assert(staticsInitialized && !strcmp(staticsInitialized, "initialized"));
  return ret;
}

template <typename T>
int ContentId<T>::getId(const char* text) {
  static unordered_map<string, int> ids;
  static int generatedId = 0;
  if (!ids.count(text)) {
    ids[text] = generatedId;
    getAllIds().push_back(text);
    ++generatedId;
  }
  auto ret = getReferenceMaybe(ids, text);
  return *ret;
}

template <typename T>
ContentId<T>::ContentId(const char* s) : id(getId(s)) {}

template <typename T>
ContentId<T>::ContentId(InternalId id) : id(id) {}

template <typename T>
bool ContentId<T>::operator == (const ContentId& o) const {
  return id == o.id;
}

template<typename T>
bool ContentId<T>::operator ==(const char* s) const {
  return !strcmp(data(), s);
}

template <typename T>
bool ContentId<T>::operator !=(const ContentId& o) const {
  return !(*this == o);
}

template <typename T>
bool ContentId<T>::operator <(const ContentId& o) const {
  return id < o.id;
}

template<typename T>
ContentId<T>::operator PrimaryId<T>() const {
  return PrimaryId<T>(id);
}

template <typename T>
int ContentId<T>::getHash() const {
  return id;
}

template <typename T>
const char* ContentId<T>::data() const {
  return getAllIds()[id].data();
}

template <typename T>
typename ContentId<T>::InternalId ContentId<T>::getInternalId() const {
  return id;
}

template <typename T>
std::ostream& operator <<(std::ostream& d, ContentId<T> id) {
  return d << id.data();
}

template <typename T>
template <class Archive>
void ContentId<T>::serialize(Archive& ar1, const unsigned int) {
  if (Archive::is_loading::value) {
    string s;
    ar1(s);
    id = getId(s.data());
  } else {
    string s = data();
    ar1(s);
  }
}

template <typename T>
template <class Archive>
void PrimaryId<T>::serialize(Archive& ar1, const unsigned int) {
  if (Archive::is_loading::value) {
    string s;
    ar1(s);
    id = ContentId<T>::getId(s.data());
  } else {
    string s = data();
    ar1(s);
  }
}

template<typename T>
PrimaryId<T>::PrimaryId(typename ContentId<T>::InternalId id) : id(id) {
}

template<typename T>
bool PrimaryId<T>::operator ==(const PrimaryId<T>& o) const {
  return id == o.id;
}

template<typename T>
bool PrimaryId<T>::operator !=(const PrimaryId<T>& o) const {
  return id != o.id;
}

template<typename T>
bool PrimaryId<T>::operator <(const PrimaryId<T>& o) const {
  return id < o.id;
}

template<typename T>
int PrimaryId<T>::getHash() const {
  return id;
}

template<typename T>
const char* PrimaryId<T>::data() const {
  return ContentId<T>::getAllIds()[id].data();
}

template<typename T>
PrimaryId<T>::operator T() const {
  return T(id);
}

template <typename T>
SERIALIZATION_CONSTRUCTOR_IMPL2(ContentId<T>, ContentId)

template <typename T>
SERIALIZATION_CONSTRUCTOR_IMPL2(PrimaryId<T>, PrimaryId)

#include "pretty_archive.h"

#define PRETTY_SPEC(T)\
template<> template<>\
void ContentId<T>::serialize(PrettyInputArchive& ar1, const unsigned int) {\
  string s;\
  ar1(s);\
  id = getId(s.data());\
  ar1.keyVerifier.verifyContentId<T>(s);\
} \
template<> template<>\
void PrimaryId<T>::serialize(PrettyInputArchive& ar1, const unsigned int) {\
  ContentId<T> cid;\
  ar1(cid);\
  if (!ar1.inheritingKey) \
    ar1.keyVerifier.addKey<T>(cid.data());\
  *this = cid;\
}

#include "text_serialization.h"

#define INST(T) \
SERIALIZABLE_TMPL(ContentId, T) \
SERIALIZABLE_TMPL(PrimaryId, T) \
template void ContentId<T>::serialize(TextInputArchive&, unsigned); \
template void ContentId<T>::serialize(TextOutputArchive&, unsigned); \
template void PrimaryId<T>::serialize(TextInputArchive&, unsigned); \
template void PrimaryId<T>::serialize(TextOutputArchive&, unsigned); \
template std::ostream& operator <<(std::ostream& d, ContentId<T> id); \
PRETTY_SPEC(T)

INST(ViewId)
INST(FurnitureType)
INST(ItemListId)
INST(EnemyId)
INST(FurnitureListId)
INST(SpellId)
INST(TechId)
INST(CreatureId)
INST(SpellSchoolId)
INST(CustomItemId)
INST(BuildingId)
INST(NameGeneratorId)
INST(MapLayoutId)
INST(RandomLayoutId)
INST(LayoutMappingId)
INST(BiomeId)
INST(CollectiveResourceId)
INST(WorkshopType)
INST(StorageId)
#undef INST
