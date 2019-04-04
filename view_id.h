#pragma once

#include "util.h"

class ViewId {
  public:
  explicit ViewId(string);
  bool operator == (const ViewId&) const;
  bool operator != (const ViewId&) const;
  bool operator < (const ViewId&) const;
  int getHash() const;
  const char* data() const;
  SERIALIZATION_DECL(ViewId)

  private:
  string SERIAL(id);
};

std::ostream& operator <<(std::ostream&, ViewId);
