#include "stdafx.h"
#include "pretty_archive.h"
#include "key_verifier.h"

string PrettyInputArchive::positionToString(const StreamPosStack& positions) {
  string allPos;
  for (auto& pos : positions)
    if (pos.line > -1) {
      string f;
      if (pos.filename > -1)
        f = filenames[pos.filename] + ": "_s;
      allPos += f + "line: "_s + toString(pos.line) + " column: " + toString<int>(pos.column) + ":\n";
    }
  return allPos;
}

void PrettyInputArchive::throwException(const StreamPosStack& positions, const string& message) {
  throw PrettyException{positionToString(positions) + message};
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
  if (index >= s.size())
    return ret;
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
      if (index >= s.size())
        return ret;
      if (s[index].c == ',')
        ++index;
      eatWhitespace(s, index);
      if (s[index].c == ')') {
        ++index;
        break;
      }
      if (index >= s.size())
        return ret;
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
    if (s[index].c == '\"' && s[index - 1].c != '\\')
      do {
        ++index;
      } while (index < s.size() - 1 && (s[index].c != '\"' || s[index - 1].c == '\\'));
    if (s[index].c == ')')
      return;
    if (s[index].c == '(')
      parseArgs(s, index);
    ++index;
    eatWhitespace(s, index);
    if (index >= s.size() || s[index].c == ',') {
      return;
    }
  }
}

optional<string> scanWord(const vector<StreamChar>& s, int& index) {
  string ret;
  int origIndex = index;
  while (index < s.size() && isspace(s[index].c))
    ++index;
  while (index < s.size() && (isalnum(s[index].c) || s[index].c == '_'))
    ret += s[index++].c;
  if (ret.empty()) {
    index = origIndex;
    return none;
  }
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
        auto beforeArgs = i;
        auto args = parseArgs(content, i);
        if (i >= content.size())
          throwException(content[beforeArgs].pos, "Couldn't parse macro arguments");
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
              const auto started = bodyIndex;
              if (body[bodyIndex].c == '"' && (bodyIndex == 0 || body[bodyIndex - 1].c != '\\'))
                bodyInQuote = !bodyInQuote;
              eatWhitespace(body, bodyIndex);
              const auto beginOccurrence = bodyIndex;
              if (!bodyInQuote && scanWord(body, bodyIndex) == def->args[argNum]) {
                replaceInStream(body, beginOccurrence, bodyIndex - beginOccurrence, args[argNum]);
                bodyIndex = beginOccurrence + args[argNum].size();
              }
              if (started != bodyIndex)
                --bodyIndex;
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
    else if (isOneOf(contents[i], '{', '}', ',', ')', '(', '+') && !inQuote) {
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

static KeyVerifier dummyKeyVerifier;

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

bool PrettyInputArchive::isOpenBracket(BracketType type) {
  return peek() == getOpenBracket(type);
}

bool PrettyInputArchive::isCloseBracket(BracketType type) {
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

StreamPosStack PrettyInputArchive::getCurrentPosition() {
  int n = (int) is.tellg();
  return streamPos.empty() ? StreamPosStack() : streamPos[max(0, min<int>(n, streamPos.size() - 1))];
}

void PrettyInputArchive::error(const string& s) {
  throwException(getCurrentPosition(), s);
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
    while (!ar1.isCloseBracket(bracket)) {
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
          if (ar1.isCloseBracket(bracket))
            break;
          processed.insert(loader.name);
          ar1.lastNamedField = loader.name;
          loader.load(true);
          ar1.eatMaybe(",");
        }
        break;
      } else
        keysAndValues = true;
      bool found = false;
      for (auto& loader : loaders)
        if (loader.name == name) {
          ar1.lastNamedField = loader.name;
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
  t.clear();
  while (true) {
    auto bookmark = ar.bookmark();
    string tmp = ar.peek();
    if (isdigit(tmp[0])) {
      int value;
      ar.is >> value;
      t += toString(value);
      if (!ar.eatMaybe("+"))
        break;
    } else {
      if (tmp[0] != '\"')
        ar.error("Expected quoted string, got: " + tmp);
      string next;
      ar.readText(std::quoted(next));
      t += next;
      if (!ar.eatMaybe("+"))
        break;
    }
  }
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

namespace {
struct Expression {
  PrettyInputArchive& ar;

  char pop() {
    char c = ' ';
    while (isspace(c))
      c = ar.is.get();
    return c;
  }

  char peek() {
    while (isspace(ar.is.peek()))
      ar.is.get();
    return ar.is.peek();
  }

  int factor() {
    if (isdigit(peek()) || peek() == '-') {
      int result;
      ar.is >> result;
      return result;
    } else
    if (peek() == '(') {
      pop();
      int result = expression();
      pop();
      return result;
    }
    ar.error("Error reading arithmetic expression: " + string(1, peek()));
    return 0;
  }

  int term() {
    int result = factor();
    while (isOneOf(peek(), '*', '/')) {
      if (pop() == '*')
        result *= factor();
      else
        result /= factor();
    }
    return result;
  }

  int expression() {
    int result = term();
    while (isOneOf(peek(), '+', '-')) {
      if (pop() == '+')
       result += term();
      else
        result -= term();
    }
    return result;
  }
};
}

void serialize(PrettyInputArchive& ar, int& c) {
  if (ar.isOpenBracket(BracketType::ROUND)) {
    ar.openBracket(BracketType::ROUND);
    c = Expression{ar}.expression();
    ar.closeBracket(BracketType::ROUND);
  } else
    ar.readText(c);
}
