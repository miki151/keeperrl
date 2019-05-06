#pragma once

#include "util.h"
#include "color.h"

class ViewId {
  public:
  explicit ViewId(const char*, Color color = Color::WHITE);
  bool operator == (const ViewId&) const;
  bool operator != (const ViewId&) const;
  bool operator < (const ViewId&) const;
  int getHash() const;
  const char* data() const;
  const Color& getColor() const;
  using InternalId = int;
  InternalId getInternalId() const;
  SERIALIZATION_DECL(ViewId)

  static void setViewIdGeneration(bool);

  private:
  InternalId id;
  Color color = Color::WHITE;
};

std::ostream& operator <<(std::ostream&, ViewId);
