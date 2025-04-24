#pragma once

#include "util.h"
#include "content_id.h"
#include "game_time.h"

class TStringId : public ContentId<TStringId> {
  public:
  using ContentId::ContentId;
  bool operator < (const TStringId&) const;
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
  TStringId SERIAL(id);
  vector<TString> SERIAL(params);
  bool SERIAL(optionalParams) = false;

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
  explicit TString(string);
  explicit TString(int);
  explicit TString(TimeInterval);
  explicit TString(GlobalTime);
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

  static void enableExportingStrings(ostream*);

  private:
  static ostream* exportStrings;
};

ostream& operator << (ostream&, const TString&);

TString combineWithCommas(vector<TString>);
TString combineWithAnd(vector<TString>);
TString combineWithSpace(vector<TString>);
TString combineWithNoSpace(TString, TString);
TString combineWithNewLine(vector<TString>);
TString combineWithOr(vector<TString>);
TString combineSentences(TString, TString);
TString combineSentences(vector<TString>);
TString toText(int num);
TString capitalFirst(TString);
TString makePlural(TString);
TString setSubjectGender(TString sentence, TString subject);
TString toPercentage(double);
TString toStringWithSign(int);
