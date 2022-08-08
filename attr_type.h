#pragma once

#include "util.h"
#include "content_id.h"

class AttrType : public ContentId<AttrType> {
  public:
  using ContentId::ContentId;
};
