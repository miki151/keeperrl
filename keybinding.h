#pragma once

#include "util.h"
#include "content_id.h"

class Keybinding : public ContentId<Keybinding> {
  public:
  using ContentId::ContentId;
};
