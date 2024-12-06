#pragma once

#include "util.h"
#include "content_id.h"

class TStringId : public ContentId<TStringId> {
  public:
  using ContentId::ContentId;
};

class TString : public variant<TStringId, string> {
  public:
  using variant::variant;
  template <typename Archive>
  void serialize(Archive&);

  TString& operator = (TStringId);
  TString& operator = (string);
};