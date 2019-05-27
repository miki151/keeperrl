#pragma once

#include "content_id.h"

class EnemyId : public ContentId<EnemyId> {
  public:
  using ContentId::ContentId;
};
