#pragma once

#include "content_id.h"

class CreatureId : public ContentId<CreatureId> {
  public:
  using ContentId::ContentId;
};
