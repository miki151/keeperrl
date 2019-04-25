#include "stdafx.h"
#include "util.h"
#include "view_id.h"

static bool viewIdGeneration = false;
static vector<string> allIds;

static int getId(const char* text) {
  static unordered_map<string, int> ids;
  static int generatedId = 0;
  if (viewIdGeneration && !ids.count(text)) {
    ids[text] = generatedId;
    allIds.push_back(text);
    ++generatedId;
  }
  auto ret = getReferenceMaybe(ids, text);
  CHECK(!!ret) << "ViewId not found: " << text;
  return *ret;
}

void ViewId::setViewIdGeneration(bool b) {
  viewIdGeneration = b;
}

ViewId::ViewId(const char* s) : id(getId(s)) {}

bool ViewId::operator == (const ViewId& o) const {
  return id == o.id;
}

bool ViewId::operator !=(const ViewId& o) const {
  return !(*this == o);
}

bool ViewId::operator <(const ViewId& o) const {
  return id < o.id;
}

int ViewId::getHash() const {
  return id;
}

const char* ViewId::data() const {
  return allIds[id].data();
}

std::ostream& operator <<(std::ostream& d, ViewId id) {
  return d << id.data();
}

template <class Archive>
void ViewId::serialize(Archive& ar1, const unsigned int) {
  if (Archive::is_loading::value) {
    string s;
    ar1(s);
    id = getId(s.data());
  } else {
    string s = data();
    ar1(s);
  }
}

SERIALIZABLE(ViewId)

SERIALIZATION_CONSTRUCTOR_IMPL(ViewId)

#include "pretty_archive.h"
template<>
void ViewId::serialize(PrettyInputArchive& ar, unsigned) {
  string text;
  ar >> text;
  id = getId(text.data());
}

#include "text_serialization.h"
template void ViewId::serialize(TextInputArchive&, unsigned);
template void ViewId::serialize(TextOutputArchive&, unsigned);
