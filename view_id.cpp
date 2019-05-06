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
#ifdef RELEASE
  USER_CHECK(!!ret) << "ViewId not found: " << text;
#else
  CHECK(!!ret) << "ViewId not found: " << text;
#endif
  return *ret;
}

void ViewId::setViewIdGeneration(bool b) {
  viewIdGeneration = b;
}

ViewId::ViewId(const char* s, Color color) : id(getId(s)), color(color) {}

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
  return combineHash(id, color);
}

const char* ViewId::data() const {
  return allIds[id].data();
}

const Color& ViewId::getColor() const {
  return color;
}

ViewId::InternalId ViewId::getInternalId() const {
  return id;
}

std::ostream& operator <<(std::ostream& d, ViewId id) {
  return d << id.data();
}

template <class Archive>
void ViewId::serialize(Archive& ar1, const unsigned int) {
  if (Archive::is_loading::value) {
    string s;
    ar1(s, color);
    id = getId(s.data());
  } else {
    string s = data();
    ar1(s, color);
  }
}

SERIALIZABLE(ViewId)

SERIALIZATION_CONSTRUCTOR_IMPL(ViewId)

#include "pretty_archive.h"
template<>
void ViewId::serialize(PrettyInputArchive& ar, unsigned) {
  string text;
  ar >> NAMED(text);
  Color colorInfo = Color::WHITE;
  ar >> OPTION(colorInfo);
  ar >> endInput();
  id = getId(text.data());
  if (colorInfo != Color::WHITE)
    color = colorInfo;
}

#include "text_serialization.h"
template void ViewId::serialize(TextInputArchive&, unsigned);
template void ViewId::serialize(TextOutputArchive&, unsigned);
