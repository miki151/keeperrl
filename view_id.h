#pragma once

#include "util.h"

class ViewId {
  public:
  explicit ViewId(const char*);
  bool operator == (const ViewId&) const;
  bool operator != (const ViewId&) const;
  bool operator < (const ViewId&) const;
  int getHash() const;
  const char* data() const;
  SERIALIZATION_DECL(ViewId)

  static void setViewIdGeneration(bool);

  private:
  int id;
};

std::ostream& operator <<(std::ostream&, ViewId);
