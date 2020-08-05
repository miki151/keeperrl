#pragma once

#include "content_id.h"

class LayoutMappingId : public ContentId<LayoutMappingId> {
  public:
  using ContentId::ContentId;
};
