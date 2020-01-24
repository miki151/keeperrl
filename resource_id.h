#pragma once

#include "util.h"
#include "content_id.h"

class CollectiveResourceId : public ContentId<CollectiveResourceId> {
  public:
  using ContentId::ContentId;
};
