#pragma once

#include "util.h"

enum class ContentIdGenerationStage;

class PrettyInputArchive;

template <typename T>
class PrimaryId;

template <typename T>
class ContentId {
  public:
  explicit ContentId(const char*);
  using InternalId = short;
  explicit ContentId(InternalId);
  bool operator == (const ContentId&) const;
  bool operator == (const char*) const;
  bool operator != (const ContentId&) const;
  bool operator < (const ContentId&) const;
  operator PrimaryId<T>() const;
  int getHash() const;
  const char* data() const;
  InternalId getInternalId() const;
  SERIALIZATION_DECL(ContentId)

  private:
  friend PrimaryId<T>;
  InternalId id;
  static vector<string>& getAllIds();
  static int getId(const char* text);
};

void setInitializedStatics();

template <typename T>
class PrimaryId {
  public:
  PrimaryId(typename ContentId<T>::InternalId);
  bool operator == (const PrimaryId<T>&) const;
  bool operator != (const PrimaryId<T>&) const;
  bool operator < (const PrimaryId<T>&) const;
  int getHash() const;
  operator T() const;
  const char* data() const;
  SERIALIZATION_DECL(PrimaryId)

  private:
  typename ContentId<T>::InternalId id;
};

template <typename Key, typename Value>
map<Key, Value> convertKeys(const map<PrimaryId<Key>, Value>& m) {
  map<Key, Value> ret;
  for (auto& elem : m)
    ret.insert(make_pair(Key(elem.first), std::move(elem.second)));
  return ret;
}

template <typename Key, typename Value>
vector<pair<Key, Value>> convertKeys(const vector<pair<PrimaryId<Key>, Value>>& v) {
  return v.transform([](auto& elem) { return make_pair(Key(std::move(elem.first)), std::move(elem.second)); });
}

template <typename Key, typename Value>
HashMap<Key, Value> convertKeysHash(const map<PrimaryId<Key>, Value>& m) {
  HashMap<Key, Value> ret;
  for (auto& elem : m)
    ret.insert(make_pair(Key(elem.first), std::move(elem.second)));
  return ret;
}

template <typename T>
std::ostream& operator <<(std::ostream&, ContentId<T>);
