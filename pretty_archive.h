#pragma once

#include "extern/iomanip.h"
#include "util.h"

struct PrettyException {
  string text;
};

class PrettyOutputArchive : public cereal::OutputArchive<PrettyOutputArchive> {
  public:
    PrettyOutputArchive(std::ostream& stream)
          : OutputArchive<PrettyOutputArchive>(this), os(stream) {}

    using base = cereal::OutputArchive<PrettyOutputArchive>;

    template<class... Types>
    PrettyOutputArchive& operator() (Types&&...args) {
      base::operator()(std::forward<Types>(args)...);
      return *this;
    }

    ~PrettyOutputArchive() CEREAL_NOEXCEPT = default;

    std::ostream& os;
};

struct StreamPos {
  int line;
  int column;
};

static pair<string, vector<StreamPos>> removeFormatting(string contents) {
  string ret;
  vector<StreamPos> pos;
  StreamPos cur {1, 1};
  bool inQuote = false;
  for (int i = 0; i < contents.size(); ++i) {
    if (contents[i] == '"')
      inQuote = !inQuote;
    if (contents[i] == '#' && !inQuote) {
      while (contents[i] != '\n' && i < contents.size())
        ++i;
    }
    else if (isOneOf(contents[i], '{', '}', ',') && !inQuote) {
      ret += " " + string(1, contents[i]) + " ";
      pos.append({cur, cur, cur});
    } else {
      ret += contents[i];
      pos.push_back(cur);
    }
    if (contents[i] == '\n') {
      ++cur.line;
      cur.column = 1;
    } else
      ++cur.column;
  }
  return {ret, pos};
}

class PrettyInputArchive : public cereal::InputArchive<PrettyInputArchive> {
  public:
    PrettyInputArchive(const string& input, optional<string> filename)
          : InputArchive<PrettyInputArchive>(this), filename(filename) {
      auto p = removeFormatting(input);
      is.str(p.first);
      streamPos = p.second;
    }

    ~PrettyInputArchive() CEREAL_NOEXCEPT = default;

    string eat(const char* expected = nullptr) {
      string s;
      is >> s;
      if (expected != nullptr && s != expected)
        error("Expected \""_s + expected + "\", got \"" + s + "\"");
      return s;
    }

    struct NodeData {
      deque<pair<string, function<void()>>> loaders;
    };

    NodeData& getNode() {
      return nodeData.back();
    }

    void error(const string& s) {
      int n = (int) is.tellg();
      auto pos = streamPos[max(0, min<int>(n, streamPos.size() - 1))];
      string msg;
      if (filename)
        msg = *filename + ": ";
      throw PrettyException{msg + "line: " + toString(pos.line) + " column: " + toString(pos.column) + ": " + s};
    }

    bool eatMaybe(const string& s) {
      if (peek() == s) {
        eat();
        return true;
      } else
        return false;
    }

    string peek(int cnt = 1) {
      string s;
      auto bookmark = is.tellg();
      for (int i : Range(cnt))
        is >> s;
      is.seekg(bookmark);
      return s;
    }

    template <typename T>
    PrettyInputArchive& readText(T&& elem) {
      auto b = bookmark();
      is >> std::forward<T>(elem);
      if (!is) {
        is.clear();
        seek(b);
        error("Error reading value of type: "_s + typeid(T).name());
      }
      return *this;
    }

    long bookmark() {
      return is.tellg();
    }

    void seek(long p) {
      is.seekg(p);
    }

    vector<NodeData> nodeData;

    private:
    std::istringstream is;
    vector<StreamPos> streamPos;
    optional<string> filename;
};

namespace cereal {
  namespace variant_detail {
    template<int N, class Variant, class ... Args>
    typename std::enable_if<N == Variant::num_types>::type
    load_variant(PrettyInputArchive & ar, const string& target, Variant & /*variant*/) {
      ar.error("Element \"" + target + "\" not part of type " + Variant::getVariantName());
    }

    template<int N, class Variant, class H, class ... T>
    typename std::enable_if<N < Variant::num_types, void>::type
    load_variant(PrettyInputArchive & ar1, const string& target, Variant & variant) {
      if (variant.getName(N) == target) {
        H value;
        ar1(value);
        variant = Variant(value);
      } else
        load_variant<N+1, Variant, T...>(ar1, target, variant);
    }
  }

  //! Saving for boost::variant
  template <typename VariantType1, const char* Str(bool), typename... VariantTypes> inline
  void CEREAL_SAVE_FUNCTION_NAME(PrettyOutputArchive& ar1, NamedVariant<Str, VariantType1, VariantTypes...> const & v ) {
    v.visit([&](const auto& elem) {
        ar1.os << v.getName(v.index());
        ar1.os << ' ';
        ar1(elem);
    });
  }

  //! Loading for boost::variant
  template <typename VariantType1, const char* Str(bool), typename... VariantTypes> inline
  void CEREAL_LOAD_FUNCTION_NAME( PrettyInputArchive & ar, NamedVariant<Str, VariantType1, VariantTypes...> & v )
  {
    string name;
    ar.readText(name);
    variant_detail::load_variant<0, NamedVariant<Str, VariantType1, VariantTypes...>, VariantType1, VariantTypes...>(
        ar, name, v);
  }
} // namespace cereal

namespace cereal {

  //! Saving for enum types
  template <class T> inline
  typename std::enable_if<std::is_enum<T>::value,void>::type
  CEREAL_SAVE_FUNCTION_NAME( PrettyOutputArchive & ar, T const & t)
  {
    ar.os << EnumInfo<T>::getString(t) << " ";
  }

  //! Loading for enum types
  template <class T> inline
  typename std::enable_if<std::is_enum<T>::value, void>::type
  CEREAL_LOAD_FUNCTION_NAME( PrettyInputArchive & ar, T & t)
  {
    string s;
    ar.readText(s);
    if (auto res = EnumInfo<T>::fromStringSafe(s))
      t = *res;
    else
      ar.error("Error reading "_s + EnumInfo<T>::getName() + " value \"" + s + "\"");
  }

  template<class T>
  struct specialize<typename std::enable_if<std::is_enum<T>::value, PrettyInputArchive>::type, T, cereal::specialization::non_member_load_save> {};

  template<class T>
  struct specialize<typename std::enable_if<std::is_enum<T>::value, PrettyOutputArchive>::type, T, cereal::specialization::non_member_load_save> {};

}

template<class T> inline
typename std::enable_if<std::is_arithmetic<T>::value && !std::is_enum<T>::value, void>::type
CEREAL_SAVE_FUNCTION_NAME(PrettyOutputArchive& ar, T const& t) {
  ar.os << t << " ";
}

template<class T> inline
typename std::enable_if<std::is_arithmetic<T>::value && !std::is_enum<T>::value, void>::type
CEREAL_LOAD_FUNCTION_NAME(PrettyInputArchive& ar, T& t) {
  ar.readText(t);
}

inline void CEREAL_SAVE_FUNCTION_NAME(PrettyOutputArchive& ar, std::string const& t) {
  ar.os << std::quoted(t) << " ";
}

inline void CEREAL_LOAD_FUNCTION_NAME(PrettyInputArchive& ar, std::string& t) {
  auto bookmark = ar.bookmark();
  string tmp;
  ar.readText(tmp);
  if (tmp[0] != '\"')
    ar.error("Expected quoted string, got: " + tmp);
  ar.seek(bookmark);
  ar.readText(std::quoted(t));
}

inline void CEREAL_LOAD_FUNCTION_NAME(PrettyInputArchive& ar, char& c) {
  string s;
  ar.readText(std::quoted(s));
  if (s[0] == '0')
    c = '\0';
  else
    c = s.at(0);
}

inline void CEREAL_SAVE_FUNCTION_NAME(PrettyOutputArchive& ar, char c) {
  string s {c};
  ar.os << std::quoted(s);
}

inline void CEREAL_LOAD_FUNCTION_NAME(PrettyInputArchive& ar, bool& c) {
  string s;
  ar.readText(s);
  if (s == "false")
    c = false;
  else if (s == "true")
    c = true;
  else
    ar.error("Unrecognized bool value: \"" + s + "\"");
}

inline void CEREAL_SAVE_FUNCTION_NAME(PrettyOutputArchive& ar, bool c) {
  ar.os << (c ? "true" : "fasle");
}

typedef StreamCombiner<ostringstream, PrettyOutputArchive> PrettyOutput;
//typedef StreamCombiner<istringstream, PrettyInputArchive> PrettyInput;

using PrettyInput = PrettyInputArchive;

template <typename T>
inline void CEREAL_LOAD_FUNCTION_NAME(PrettyInputArchive& ar1, vector<T>& v) {
  v.clear();
  string s;
  ar1.readText(s);
  if (s != "{")
    ar1.error("Expected list of items surrounded by { and }");
  while (1) {
    if (ar1.peek() == "}")
      break;
    T t;
    ar1(t);
    v.push_back(t);
  }
  ar1.eat("}");
}

template <typename T, typename U>
inline void CEREAL_LOAD_FUNCTION_NAME(PrettyInputArchive& ar1, map<T, U>& m) {
  vector<pair<T, U>> v;
  ar1(v);
  for (auto& elem : v)
    m.insert(elem);
}


template <typename T>
inline void CEREAL_SAVE_FUNCTION_NAME(PrettyOutputArchive& ar1, vector<T> const& v) {
  ar1.os << "{";
  bool first = true;
  for (auto& elem : v) {
    if (!first)
      ar1.os << ",";
    ar1 << v;
    first = false;
  }
  ar1.os << "}";
}

template <typename T>
inline void CEREAL_LOAD_FUNCTION_NAME(PrettyInputArchive& ar1, optional<T>& v) {
  v.reset();
  if (ar1.eatMaybe("none"))
    return;
  T t;
  ar1(t);
  v = std::move(t);
}

template <typename T>
inline void CEREAL_SAVE_FUNCTION_NAME(PrettyOutputArchive& ar1, optional<T> const& v) {
  if (!v)
    ar1.os << "none";
  else
    ar1 << *v;
}

template <class T>
inline void CEREAL_SAVE_FUNCTION_NAME(PrettyOutputArchive& ar1, cereal::NameValuePair<T> const& t) {
  if (strcmp(t.name, "cereal_class_version"))
    ar1(t.value);
}

template <class T>
inline void setVersion1000(T&) {
}

template <>
inline void setVersion1000(unsigned int& a) {
  a = 1000;
}

template <class T>
inline void CEREAL_LOAD_FUNCTION_NAME(PrettyInputArchive& ar1, cereal::NameValuePair<T>& t) {
  if (strcmp(t.name, "cereal_class_version")) {
    auto& value = t.value;
    ar1.getNode().loaders.push_back(make_pair(t.name, [&ar1, &value]{ ar1(value); }));
  } else
    setVersion1000(t.value);
}

template <class T, class U> inline
void serialize(PrettyInputArchive& ar1, std::pair<T, U>& t) {
  ar1(t.first, t.second);
}

namespace pretty_tuple_detail {
    template <size_t Height>
    struct serialize {
      template <class Archive, class ... Types> inline
      static void apply(Archive& ar1, std::tuple<Types...>& tuple) {
        serialize<Height - 1>::template apply(ar1, tuple);
        ar1(std::get<Height - 1>(tuple));
      }
    };

    template <>
    struct serialize<0> {
      template <class Archive, class ... Types> inline
      static void apply( Archive &, std::tuple<Types...>& ) { }
    };
  }

template <class ... Types> inline
void serialize(PrettyInputArchive& ar, std::tuple<Types...>& tuple) {
  pretty_tuple_detail::serialize<std::tuple_size<std::tuple<Types...>>::value>::template apply( ar, tuple );
}

template <class T, cereal::traits::EnableIf<!std::is_arithmetic<T>::value> = cereal::traits::sfinae>
inline void prologue(PrettyInputArchive& ar1, T const & ) {
  ar1.nodeData.emplace_back();
}

template <class T, cereal::traits::EnableIf<!std::is_arithmetic<T>::value> = cereal::traits::sfinae>
inline void epilogue(PrettyInputArchive& ar1, T const &t ) {
  auto loaders = ar1.getNode().loaders;
  if (!loaders.empty()) {
    ar1.eat("{");
    bool keysAndValues = false;
    while (ar1.peek() != "}") {
      if (ar1.peek() == ",")
        ar1.eat();
      auto bookmark = ar1.bookmark();
      string name, equals;
      ar1.readText(name).readText(equals);
      if (equals != "=") {
        if (keysAndValues)
          ar1.error("Expected a \"key = value\" pair");
        ar1.seek(bookmark);
        for (auto& loader : loaders) {
          if (ar1.peek() == "}")
            break;
          loader.second();
        }
        break;
      } else
        keysAndValues = true;
      bool found = false;
      for (auto& loader : loaders)
        if (loader.first == name) {
          loader.second();
          found = true;
          break;
        }
      if (!found)
        ar1.error("No member named \"" + name + "\" in structure");
    }
    ar1.eat("}");
  }
  ar1.nodeData.pop_back();
}

// Ignore these inputs
template <class T> inline
void prologue(PrettyInputArchive&, cereal::NameValuePair<T> const & ) { }

template <class T> inline
void epilogue(PrettyInputArchive&, cereal::NameValuePair<T> const & ) { }

template <class T> inline
void prologue(PrettyInputArchive&, cereal::SizeTag<T> const & ) { }

template <class T> inline
void epilogue(PrettyInputArchive&, cereal::SizeTag<T> const & ) { }

//! Serializing SizeTags to binary
template <class Archive, class T> inline
CEREAL_ARCHIVE_RESTRICT(PrettyInputArchive, PrettyOutputArchive)
CEREAL_SERIALIZE_FUNCTION_NAME(Archive& ar1, cereal::SizeTag<T> & t) {
  ar1(t.size);
}

// register archives for polymorphic support
// Commented out because it causes linker errors in item_type.cpp
//CEREAL_REGISTER_ARCHIVE(PrettyOutputArchive)
//CEREAL_REGISTER_ARCHIVE(PrettyInputArchive)

// tie input and output archives together
CEREAL_SETUP_ARCHIVE_TRAITS(PrettyInputArchive, PrettyOutputArchive)
