#pragma once

#include "content_id.h"

class ItemListId : public ContentId<ItemListId> {
  public:
  using ContentId::ContentId;
};
