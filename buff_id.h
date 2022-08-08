#pragma once

#include "util.h"
#include "content_id.h"

class BuffId : public ContentId<BuffId> {
  public:
  using ContentId::ContentId;
};
