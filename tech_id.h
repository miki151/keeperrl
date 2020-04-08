#pragma once

#include "content_id.h"

const string getName(TechId);
const string getTechIdName(TechId);

class TechId : public ContentId<TechId> {
  public:
  using ContentId::ContentId;
};
