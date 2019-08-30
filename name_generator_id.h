#pragma once

#include "content_id.h"

class NameGeneratorId : public ContentId<NameGeneratorId> {
  public:
  using ContentId::ContentId;
};
