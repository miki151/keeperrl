#pragma once

#include "stdafx.h"
#include "content_id.h"

class StorageId : public ContentId<StorageId> {
  public:
  using ContentId::ContentId;
};
