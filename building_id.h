#pragma once

#include "content_id.h"

class BuildingId : public ContentId<BuildingId> {
  public:
  using ContentId::ContentId;
};
