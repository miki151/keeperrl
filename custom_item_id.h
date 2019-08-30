#pragma once

#include "content_id.h"

class ItemAttributes;
class ContentFactory;

class CustomItemId : public ContentId<CustomItemId> {
  public:
  using ContentId::ContentId;
  ItemAttributes getAttributes(const ContentFactory*) const;
};
