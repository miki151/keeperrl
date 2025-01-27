#pragma once

#include "util.h"
#include "content_id.h"

class TStringId : public ContentId<TStringId> {
  public:
  using ContentId::ContentId;
};

class TString;

struct TSentence {
  TSentence(const char* id) : TSentence(TStringId(id)) {}
  TSentence(const char* id, TString param);
  TSentence(const char* id, TString param1, TString param2);
  TSentence(const char* id, vector<TString> params);
  TSentence(TStringId id) : id(id) {}
  TSentence(TStringId id, TString param);
  TSentence(TStringId id, TString param1, TString param2);
  TSentence(TStringId id, vector<TString> params);
  TStringId id;
  vector<TString> params;
  bool optionalParams = false;

  int getHash() const;

  bool operator == (const TSentence&) const;
  bool operator != (const TSentence&) const;
  bool operator < (const TSentence&) const;

  TSentence();
  template <typename Archive>
  void serialize(Archive&, unsigned int);
};

class TString {
  public:
  TString(string);
  TString(TSentence);
  TString(TStringId);
  TString();

  template <typename Archive>
  void serialize(Archive&);

  bool empty() const;

  int getHash() const;

  const char* data() const;

  TString& operator = (TSentence);
  TString& operator = (TStringId);
  TString& operator = (string);

  bool operator == (const TString&) const;
  bool operator != (const TString&) const;
  bool operator < (const TString&) const;

  variant<TSentence, string> text;
};

TString combineWithCommas(vector<TString>);
TString combineWithAnd(vector<TString>);
TString combineWithSpace(vector<TString>);
TString combineWithNewLine(vector<TString>);
TString combineWithOr(vector<TString>);
TString combineSentences(TString, TString);
TString combineSentences(vector<TString>);
TString toText(int num);