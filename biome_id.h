#pragma once

#include "util.h"
#include "content_id.h"

class BiomeId : public ContentId<BiomeId> {
  public:
  using ContentId::ContentId;
};
