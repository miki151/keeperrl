#pragma once

#include "content_id.h"

class RandomLayoutId : public ContentId<RandomLayoutId> {
  public:
  using ContentId::ContentId;
};
