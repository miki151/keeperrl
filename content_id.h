#pragma once

#include "util.h"

enum class ContentIdGenerationStage;

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

  static void startContentIdGeneration();
  static void validateContentIds();

  private:
  InternalId id;
  static void checkId(InternalId);
  static ContentIdGenerationStage contentIdGeneration;
  static vector<string> allIds;
  static unordered_set<int> validIds;
  static int getId(const char* text);
};

template <typename T>
std::ostream& operator <<(std::ostream&, ContentId<T>);
