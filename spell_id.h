#pragma once

#include "content_id.h"

class SpellId : public ContentId<SpellId> {
  public:
  using ContentId::ContentId;
};
