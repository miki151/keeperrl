#pragma once

#include "util.h"

template <typename T>
class ContentId {
  public:
  explicit ContentId(const char*);
  bool operator == (const ContentId&) const;
  bool operator != (const ContentId&) const;
  bool operator < (const ContentId&) const;
  int getHash() const;
  const char* data() const;
  using InternalId = int;
  InternalId getInternalId() const;
  SERIALIZATION_DECL(ContentId)

  static void setContentIdGeneration(bool);

  private:
  InternalId id;
  static bool contentIdGeneration;
  static vector<string> allIds;
  static int getId(const char* text);
};

template <typename T>
std::ostream& operator <<(std::ostream&, ContentId<T>);
