#pragma once

#include "util.h"
#include "content_id.h"

class FurnitureType : public ContentId<FurnitureType> {
  public:
  using ContentId::ContentId;
};
