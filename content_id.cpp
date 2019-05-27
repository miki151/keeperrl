#include "stdafx.h"
#include "content_id.h"

enum class ContentIdGenerationStage {
  BEFORE,
  DURING,
  AFTER
};

template <typename T>
ContentIdGenerationStage ContentId<T>::contentIdGeneration = ContentIdGenerationStage::BEFORE;

template <typename T>
vector<string> ContentId<T>::allIds;

template <typename T>
unordered_set<int> ContentId<T>::validIds;


template <typename T>
void ContentId<T>::checkId(InternalId id) {
#ifdef RELEASE
  USER_CHECK(validIds.count(id)) << "Content id not declared: " << allIds[id];
#else
  CHECK(validIds.count(id)) << "Content id not declared: " << allIds[id];
#endif
}

template <typename T>
int ContentId<T>::getId(const char* text) {
  static unordered_map<string, int> ids;
  static int generatedId = 0;
  if (!ids.count(text)) {
    ids[text] = generatedId;
    allIds.push_back(text);
    ++generatedId;
  }
  auto ret = getReferenceMaybe(ids, text);
  switch (contentIdGeneration) {
    case ContentIdGenerationStage::BEFORE:
      break;
    case ContentIdGenerationStage::DURING:
      validIds.insert(*ret);
      break;
    case ContentIdGenerationStage::AFTER:
      checkId(*ret);
      break;
  }
  return *ret;
}

template <typename T>
void ContentId<T>::startContentIdGeneration() {
  contentIdGeneration = ContentIdGenerationStage::DURING;
}

template<typename T>
void ContentId<T>::validateContentIds() {
  contentIdGeneration = ContentIdGenerationStage::AFTER;
  for (int i : All(allIds))
    checkId(i);
}

template <typename T>
ContentId<T>::ContentId(const char* s) : id(getId(s)) {}

template <typename T>
bool ContentId<T>::operator == (const ContentId& o) const {
  return id == o.id;
}

template <typename T>
bool ContentId<T>::operator !=(const ContentId& o) const {
  return !(*this == o);
}

template <typename T>
bool ContentId<T>::operator <(const ContentId& o) const {
  return id < o.id;
}

template <typename T>
int ContentId<T>::getHash() const {
  return id;
}

template <typename T>
const char* ContentId<T>::data() const {
  return allIds[id].data();
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
SERIALIZATION_CONSTRUCTOR_IMPL2(ContentId<T>, ContentId)

#include "pretty_archive.h"
#include "text_serialization.h"

#define INST(T) \
SERIALIZABLE_TMPL(ContentId, T) \
template void ContentId<T>::serialize(PrettyInputArchive&, unsigned); \
template void ContentId<T>::serialize(TextInputArchive&, unsigned); \
template void ContentId<T>::serialize(TextOutputArchive&, unsigned);

INST(ViewId)
INST(FurnitureType)
INST(ItemListId)
