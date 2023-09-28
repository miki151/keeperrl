#pragma once

#include "content_id.h"

class AchievementId : public ContentId<AchievementId> {
  public:
  using ContentId::ContentId;
};
