#include "view_id.h"


ViewId::ViewId(string s) : id(std::move(s)) {}

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
  return int(std::hash<string>()(id));
}

const char*ViewId::data() const {
  return id.data();
}

std::ostream& operator <<(std::ostream& d, ViewId id) {
  return d << id.data();
}

SERIALIZE_DEF(ViewId, id)
SERIALIZATION_CONSTRUCTOR_IMPL(ViewId)

#include "pretty_archive.h"
template void ViewId::serialize(PrettyInputArchive&, unsigned);

#include "text_serialization.h"
template void ViewId::serialize(TextInputArchive&, unsigned);
template void ViewId::serialize(TextOutputArchive&, unsigned);
