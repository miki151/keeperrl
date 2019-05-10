#include "stdafx.h"
#include "content_id.h"

template <typename T>
bool ContentId<T>::contentIdGeneration = false;

template <typename T>
vector<string> ContentId<T>::allIds;

template <typename T>
int ContentId<T>::getId(const char* text) {
  static unordered_map<string, int> ids;
  static int generatedId = 0;
  if (contentIdGeneration && !ids.count(text)) {
    ids[text] = generatedId;
    allIds.push_back(text);
    ++generatedId;
  }
  auto ret = getReferenceMaybe(ids, text);
#ifdef RELEASE
  USER_CHECK(!!ret) << "ViewId not found: " << text;
#else
  CHECK(!!ret) << "ViewId not found: " << text;
#endif
  return *ret;
}

template <typename T>
void ContentId<T>::setContentIdGeneration(bool b) {
  contentIdGeneration = b;
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

SERIALIZABLE_TMPL(ContentId, ViewId)

template <typename T>
SERIALIZATION_CONSTRUCTOR_IMPL2(ContentId<T>, ContentId)

#include "pretty_archive.h"

template ContentId<ViewId>::ContentId();
template void ContentId<ViewId>::serialize(PrettyInputArchive&, unsigned);

#include "text_serialization.h"
template void ContentId<ViewId>::serialize(TextInputArchive&, unsigned);
template void ContentId<ViewId>::serialize(TextOutputArchive&, unsigned);
