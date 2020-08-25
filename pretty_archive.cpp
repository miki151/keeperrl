#include "stdafx.h"
#include "pretty_archive.h"

void PrettyInputArchive::throwException(const StreamPosStack& positions, const string& message) {
  string allPos;
  for (auto& pos : positions)
    if (pos.line > -1) {
      string f;
      if (pos.filename > -1)
        f = filenames[pos.filename] + ": "_s;
      allPos += f + "line: "_s + toString(pos.line) + " column: " + toString<int>(pos.column) + ":\n";
    }
  throw PrettyException{allPos + message};
}

static bool contains(const vector<StreamChar>& a, const string& substring, int index) {
  if (a.size() - index < substring.size())
    return false;
  for (auto i = index; i < index + substring.size(); ++i)
    if (a[i].c != substring[i - index])
      return false;
  return true;
}

static void eatWhitespace(const vector<StreamChar>& s, int& index) {
  while (index < s.size() && isspace(s[index].c))
    ++index;
}

void eatArgument(const vector<StreamChar>& s, int& index);

static vector<StreamChar> subStream(const vector<StreamChar>& s, int index, int count) {
  vector<StreamChar> ret;
  ret.reserve(count);
  for (int i = index; i < index + count && i < s.size(); ++i)
    ret.push_back(s[i]);
  return ret;
}

static vector<vector<StreamChar>> parseArgs(const vector<StreamChar>& s, int& index) {
  vector<vector<StreamChar>> ret;
  eatWhitespace(s, index);
  if (s[index].c == '(') {
    ++index;
    while (1) {
      eatWhitespace(s, index);
      optional<int> beginArg;
      if (s[index].c != ')')
        beginArg = index;
      eatArgument(s, index);
      if (beginArg) {
        eatWhitespace(s, *beginArg);
        ret.push_back(subStream(s, *beginArg, index - *beginArg));
        while (isspace(ret.back().back().c))
          ret.back().pop_back();
      }
      if (s[index].c == ',')
        ++index;
      eatWhitespace(s, index);
      if (s[index].c == ')') {
        ++index;
        break;
      }
    }
  }
  return ret;
}

string getString(const vector<StreamChar>& s) {
  string ret;
  ret.reserve(s.size());
  for (auto& elem : s)
    ret += elem.c;
  return ret;
}

void eatArgument(const vector<StreamChar>& s, int& index) {
  while (1) {
    eatWhitespace(s, index);
    if (s[index].c == ')')
      return;
    if (s[index].c == '(')
      parseArgs(s, index);
    ++index;
    eatWhitespace(s, index);
    if (s[index].c == ',') {
      return;
    }
  }
}

optional<string> scanWord(const vector<StreamChar>& s, int& index) {
  string ret;
  while (index < s.size() && isspace(s[index].c))
    ++index;
  while (index < s.size() && isalnum(s[index].c))
    ret += s[index++].c;
  if (ret.empty())
    return none;
  return ret;
}

static optional<string> peekWord(const vector<StreamChar>& s, int index) {
  return scanWord(s, index);
}

static void replaceInStream(vector<StreamChar>& s, int index, int length, const vector<StreamChar>& content) {
  s.erase(index, length);
  s.insert(index, content);
}

pair<PrettyInputArchive::DefsMap, vector<StreamChar>> PrettyInputArchive::parseDefs(const vector<StreamChar>& content) {
  vector<StreamChar> ret;
  bool inQuote = false;
  optional<pair<pair<string, int>, DefInfo>> currentDef;
  DefsMap defs;
  for (int i = 0; i < content.size(); ++i) {
    if (content[i].c == '"' && (i == 0 || content[i - 1].c != '\\'))
      inQuote = !inQuote;
    if (!inQuote && contains(content, "Def", i)) {
      if (!!currentDef)
        throwException(content[i].pos, "Definition inside another definition is not allowed");
      i += strlen("Def");
      if (auto name = scanWord(content, i)) {
        auto args = parseArgs(content, i);
        currentDef = make_pair(make_pair(*name, args.size()),
            DefInfo{ i, 0, args.transform([](auto& arg) { return getString(arg); } ) });
        if (defs.count({*name, args.size()}))
          throwException(content[i].pos, *name + " defined more than once");
      } else
        throwException(content[i].pos, "Definition name expected");
      while (i < content.size() && peekWord(content, i) != "End"_s)
        ++i;
      if (i >= content.size())
        throwException(content[currentDef->second.begin].pos, "Definition lacks an End token");
      currentDef->second.end = i;
      defs.insert(*currentDef);
      currentDef = none;
      scanWord(content, i);
    } else if (i < content.size())
      ret.push_back(content[i]);
  }
  return make_pair(std::move(defs), std::move(ret));
}

static void append(StreamPosStack& to, const StreamPosStack& from) {
  for (int i : All(to))
    if (to[i].line == -1)
      for (int j : All(from)) {
        to[i + j] = from[j];
        CHECK(i + j < to.size());
        if (from[j].line == -1 || j + i >= to.size() - 1)
          return;
      }
}

vector<StreamChar> PrettyInputArchive::preprocess(const vector<StreamChar>& content) {
  bool inQuote = false;
  auto parseRes = parseDefs(content);
  auto& ret = parseRes.second;
  auto& defs = parseRes.first;
  for (int i = 0; i < ret.size(); ++i) {
    if (ret[i].c == '"' && (i == 0 || ret[i - 1].c != '\\'))
      inQuote = !inQuote;
    if (!inQuote) {
      auto beginCall = i;
      if (auto name = scanWord(ret, i)) {
        int argsPos = i;
        auto args = parseArgs(ret, argsPos);
        if (auto def = getReferenceMaybe(defs, make_pair(*name, args.size()))) {
          if (args.size() != def->args.size())
            throwException(ret[argsPos].pos, "Wrong number of arguments to macro " + *name);
          auto body = subStream(content, def->begin, def->end - def->begin);
          for (auto& elem : body)
            append(elem.pos, ret[i].pos);
          for (int argNum : All(args)) {
            bool bodyInQuote = false;
            for (int bodyIndex = 0; bodyIndex < body.size(); ++bodyIndex) {
              if (body[bodyIndex].c == '"' && (bodyIndex == 0 || body[bodyIndex - 1].c != '\\'))
                bodyInQuote = !bodyInQuote;
              eatWhitespace(body, bodyIndex);
              auto beginOccurrence = bodyIndex;
              if (!bodyInQuote && scanWord(body, bodyIndex) == def->args[argNum]) {
                replaceInStream(body, beginOccurrence, bodyIndex - beginOccurrence, args[argNum]);
              }
            }
          }
          replaceInStream(ret, beginCall, argsPos - beginCall, body);
          i = beginCall;
        }
      }
    }
  }
  /*if (!defs.empty())
    std::cout << "Replaced\n" << getString(ret) << "\nEnd replaced\n";*/
  return ret;
}

vector<StreamChar> removeFormatting(string contents, signed char filename) {
  vector<StreamChar> ret;
  auto addChar = [&ret] (StreamPos pos, char c) {
    ret.push_back(StreamChar{{std::move(pos)}, c});
  };
  StreamPos cur {filename, 1, 1};
  bool inQuote = false;
  for (unsigned i = 0; i < contents.size(); ++i) {
    bool addSpace = false;
    if (contents[i] == '"' && (i == 0 || contents[i - 1] != '\\')) {
      inQuote = !inQuote;
      if (inQuote) {
        addChar(cur, ' ');
      } else
        addSpace = true;
    }
    if (contents[i] == '#' && !inQuote) {
      while (contents[i] != '\n' && i < contents.size())
        ++i;
    }
    else if (isOneOf(contents[i], '{', '}', ',', ')', '(') && !inQuote) {
      addChar(cur, ' ');
      addChar(cur, contents[i]);
      addChar(cur, ' ');
    } else {
      addChar(cur, contents[i]);
    }
    if (addSpace)
      addChar(cur, ' ');
    addSpace = false;
    if (contents[i] == '\n') {
      ++cur.line;
      cur.column = 1;
    } else
      ++cur.column;
  }
  return ret;
}

PrettyInputArchive::PrettyInputArchive(const vector<string>& inputs, const vector<string>& filenames, KeyVerifier* v)
  : keyVerifier(v ? *v : dummyKeyVerifier), filenames(filenames) {
  vector<StreamChar> allInput;
  if (!filenames.empty()) {
    allInput.push_back(StreamChar{{}, '{'});
    allInput.push_back(StreamChar{{}, '\n'});
  }
  for (int i = 0; i < inputs.size(); ++i)
    allInput.append(removeFormatting(inputs[i], i < filenames.size() ? i : -1));
  if (!filenames.empty()) {
    allInput.push_back(StreamChar{{}, '\n'});
    allInput.push_back(StreamChar{{}, '}'});
  }
  auto res = preprocess(allInput);
  streamPos = res.transform([](auto& elem) { return elem.pos; });
  is.str(getString(res));
}

static auto getOpenBracket(BracketType type) {
  switch (type) {
    case BracketType::CURLY: return "{";
    case BracketType::ROUND: return "(";
  }
}

static auto getCloseBracket(BracketType type) {
  switch (type) {
    case BracketType::CURLY: return "}";
    case BracketType::ROUND: return ")";
  }
}

void PrettyInputArchive::openBracket(BracketType type) {
  eat(getOpenBracket(type));
}

void PrettyInputArchive::closeBracket(BracketType type) {
  eat(getCloseBracket(type));
}

bool PrettyInputArchive::isClosedBracket(BracketType type) {
  return peek() == getCloseBracket(type);
}

string PrettyInputArchive::eat(const char* expected) {
  string s;
  is >> s;
  if (!is) {
    if (is.eof())
      error("Unexpected end of file");
    if (is.fail())
      error("Failure " + is.str());
    if (is.bad())
      error("IO Failure");
  }
  if (expected != nullptr && s != expected)
    error("Expected \""_s + expected + "\", got \"" + s + "\"");
  return s;
}

void PrettyInputArchive::error(const string& s) {
  int n = (int) is.tellg();
  auto pos = streamPos.empty() ? StreamPosStack() : streamPos[max(0, min<int>(n, streamPos.size() - 1))];
  throwException(pos, s);
}

bool PrettyInputArchive::eatMaybe(const string& s) {
  if (peek() == s) {
    eat();
    return true;
  } else
    return false;
}

string PrettyInputArchive::peek(int cnt) {
  string s;
  auto bookmark = is.tellg();
  for (int i : Range(cnt))
    is >> s;
  is.seekg(bookmark);
  return s;
}

long PrettyInputArchive::bookmark() {
  return is.tellg();
}

void PrettyInputArchive::seek(long p) {
  is.seekg(p);
}

void PrettyInputArchive::startNode() {
  nodeData.emplace_back();
  if (nextElemInherited)
    nodeData.back().inherited = true;
  nextElemInherited = false;
}

void PrettyInputArchive::endNode() {
  nodeData.pop_back();
}

void prettyEpilogue(PrettyInputArchive& ar1) {
  auto loaders = ar1.getNode().loaders;
  if (!loaders.empty()) {
    auto bracket = ar1.getNode().bracket;
    bool appending = ar1.eatMaybe("append") || ar1.getNode().inherited;
    ar1.openBracket(bracket);
    bool keysAndValues = false;
    set<string> processed;
    while (!ar1.isClosedBracket(bracket)) {
      if (ar1.peek() == ",")
        ar1.eat();
      auto bookmark = ar1.bookmark();
      string name, equals;
      ar1.readText(name);
      ar1.readText(equals);
      if (equals != "=") {
        if (keysAndValues)
          ar1.error("Expected a \"key = value\" pair");
        ar1.seek(bookmark);
        for (auto& loader : loaders) {
          if (ar1.isClosedBracket(bracket))
            break;
          processed.insert(loader.name);
          loader.load(true);
          ar1.eatMaybe(",");
        }
        break;
      } else
        keysAndValues = true;
      bool found = false;
      for (auto& loader : loaders)
        if (loader.name == name) {
          if (processed.count(name))
            ar1.error("Value defined twice: \"" + name + "\"");
          processed.insert(name);
          bool initialize = true;
          if (ar1.peek() == "append") {
            if (!appending)
              ar1.error("Can't append to value that wasn't inherited");
            initialize = false;
          }
          loader.load(initialize);
          found = true;
          break;
        }
      if (!found)
        ar1.error("No member named \"" + name + "\" in structure");
    }
    if (!appending)
      for (auto& loader : loaders)
        if (!processed.count(loader.name) && !loader.optional)
          ar1.error("Field \"" + loader.name + "\" not present");
    ar1.closeBracket(bracket);
    ar1.getNode().loaders.clear();
  }
}

void serialize(PrettyInputArchive& ar, string& t) {
  auto bookmark = ar.bookmark();
  string tmp;
  ar.readText(tmp);
  if (tmp[0] != '\"')
    ar.error("Expected quoted string, got: " + tmp);
  ar.seek(bookmark);
  ar.readText(std::quoted(t));
}

void serialize(PrettyInputArchive& ar, char& c) {
  string s;
  ar.readText(std::quoted(s));
  if (s[0] == '0')
    c = '\0';
  else
    c = s.at(0);
}

void serialize(PrettyInputArchive& ar, bool& c) {
  string s;
  ar.readText(s);
  if (s == "false")
    c = false;
  else if (s == "true")
    c = true;
  else
    ar.error("Unrecognized bool value: \"" + s + "\"");
}

void serialize(PrettyInputArchive& ar, PrettyFlag& c) {
  string s;
  ar.readText(s);
  if (s == "true")
    c.value = true;
  else
    ar.error("This value can only be set to \"true\" or not set at all");
}

void serialize(PrettyInputArchive& ar1, EndPrettyInput&) {
  prettyEpilogue(ar1);
}

void serialize(PrettyInputArchive& ar1, SetRoundBracket&) {
  ar1.nodeData[ar1.nodeData.size() - 2].bracket = BracketType::ROUND;
}

EndPrettyInput& endInput() {
  static EndPrettyInput ret;
  return ret;
}

SetRoundBracket& roundBracket() {
  static SetRoundBracket ret;
  return ret;
}
