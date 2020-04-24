#pragma once

#include "util.h"
#include "color.h"
#include "content_id.h"

class ViewId : public ContentId<ViewId> {
  public:
  explicit ViewId(InternalId);
  explicit ViewId(const char*, Color color = Color::WHITE);
  bool operator == (const ViewId&) const;
  bool operator != (const ViewId&) const;
  bool operator < (const ViewId&) const;
  int getHash() const;
  const Color& getColor() const;
  ViewId withColor(Color) const;
  SERIALIZATION_DECL(ViewId)

  private:
  Color color = Color::WHITE;
};

using ViewIdList = vector<ViewId>;

std::ostream& operator <<(std::ostream&, ViewId);
