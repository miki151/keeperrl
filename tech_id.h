#pragma once

#include "content_id.h"

class TechId : public ContentId<TechId> {
  public:
  using ContentId::ContentId;
};
