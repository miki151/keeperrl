#pragma once

#include "util.h"
#include "content_id.h"

class BodyMaterialId : public ContentId<BodyMaterialId> {
  public:
  using ContentId::ContentId;
};
