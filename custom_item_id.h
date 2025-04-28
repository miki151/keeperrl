#pragma once

#include "content_id.h"

class ContentFactory;

class CustomItemId : public ContentId<CustomItemId> {
  public:
  using ContentId::ContentId;
  SItemAttributes getAttributes(const ContentFactory*) const;
};
