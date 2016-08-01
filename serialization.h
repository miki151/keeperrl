/* Copyright (C) 2013-2014 Michal Brzozowski (rusolis@poczta.fm)

   This file is part of KeeperRL.

   KeeperRL is free software; you can redistribute it and/or modify it under the terms of the
   GNU General Public License as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   KeeperRL is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
   even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along with this program.
   If not, see http://www.gnu.org/licenses/ . */

#ifndef _SERIALIZATION
#define _SERIALIZATION

#ifdef TEXT_SERIALIZATION
typedef text_iarchive InputArchive;
typedef text_oarchive OutputArchive;
typedef portable_iarchive InputArchive2;
#else
typedef portable_iarchive InputArchive;
typedef portable_oarchive OutputArchive;
typedef text_iarchive InputArchive2;
#endif

#define SUBCLASS(X) boost::serialization::make_nvp("Base", boost::serialization::base_object<X>(*this))

#define SERIALIZABLE(T) \
   template void T::serialize(InputArchive&, unsigned); \
   template void T::serialize(OutputArchive&, unsigned); \
   template void T::serialize(InputArchive2&, unsigned);

#define SERIALIZABLE_TMPL(T, ...) \
   template class T<__VA_ARGS__>;\
   template void T<__VA_ARGS__>::serialize(InputArchive&, unsigned); \
   template void T<__VA_ARGS__>::serialize(OutputArchive&, unsigned); \
   template void T<__VA_ARGS__>::serialize(InputArchive2&, unsigned);

#define REGISTER_TYPES(M) \
   template void M(InputArchive&, int); \
   template void M(OutputArchive&, int); \
   template void M(InputArchive2&, int);

#define REGISTER_TYPE(T, A)\
(T).register_type(static_cast<A*>(nullptr))

#define SERIAL(X) X
#define SVAR(X) boost::serialization::make_nvp(#X, X)

#define SERIALIZATION_DECL(A) \
  friend boost::serialization::access; \
  A(); \
  template <class Archive> \
  void serialize(Archive& ar, const unsigned int version);

#define SERIALIZATION_CONSTRUCTOR_IMPL(A) \
  A::A() {}

#define SERIALIZATION_CONSTRUCTOR_IMPL2(A, B) \
  A::B() {}

#define SERIALIZATION_CONSTRUCTOR(A) \
  A() {}

#define SERIALIZE_DEF(CLASS, ...) \
template <class Archive> \
void CLASS::serialize(Archive& ar, const unsigned int version) { \
  serializeAll(ar, __VA_ARGS__);\
}\
SERIALIZABLE(CLASS);


template <typename Archive>
void serializeAll(Archive& ar) {
}

template <typename Archive, typename Arg1, typename... Args>
void serializeAll(Archive& ar, Arg1& arg1, Args&... args) {
  ar & boost::serialization::make_nvp("arg1", arg1);
  serializeAll(ar, args...);
}

#define SERIALIZE_SUBCLASS(SUB) \
  template <class Archive> \
  void serialize(Archive& ar, const unsigned int version) { \
    ar & SUBCLASS(SUB);\
  }

#define SERIALIZE_ALL2(SUB, ...) \
  template <class Archive> \
  void serialize(Archive& ar, const unsigned int version) { \
    ar & SUBCLASS(SUB);\
    serializeAll(ar, __VA_ARGS__); \
  }

#define SERIALIZE_ALL(...) \
  template <class Archive> \
  void serialize(Archive& ar, const unsigned int version) { \
    serializeAll(ar, __VA_ARGS__); \
  }

class Serialization {
  public:
  template <class Archive>
  static void registerTypes(Archive& ar, int version);
};

template <class T, class U>
class StreamCombiner {
  public:
  template <typename ...Args>
  StreamCombiner(Args... args) : stream(args...), archive(stream) {
 //   CHECK(stream.good()) << "File not found: " << filename;
  }

  U& getArchive() {
    return archive;
  }

  T& getStream() {
    return stream;
  }

  private:
  T stream;
  U archive;
};


namespace boost { 
namespace serialization {


// why the f** are these things not implemented by default in boost?
//unordered_map
template<class Archive, class T, class U, class H>
inline void save(Archive& ar, const unordered_map<T, U, H>& t, unsigned int file_version){
  int count = t.size();
  ar << BOOST_SERIALIZATION_NVP(count);
  for (auto& elem : t)
    ar << boost::serialization::make_nvp("key", elem.first) << boost::serialization::make_nvp("value", elem.second);
}

template<class Archive, class T, class U, class H>
inline void load(Archive& ar, unordered_map<T, U, H>& t, unsigned int){
  int count;
  ar >> BOOST_SERIALIZATION_NVP(count);
  t.clear();
  t.reserve(count);
  while (count-- > 0) {
    pair<T, U> p;
    ar >> boost::serialization::make_nvp("key", p.first) >> boost::serialization::make_nvp("value", p.second);
    t.insert(std::move(p));
  }
}

template<class Archive, class T, class U, class H>
inline void serialize(Archive& ar, unordered_map<T, U, H>& t, unsigned int file_version){
  boost::serialization::split_free(ar, t, file_version);
}

//priority queue
template<class Archive, class T, class U, class V>
inline void save(Archive& ar, priority_queue<T, U, V> t, unsigned int file_version){
  vector<T> array;
  while (!t.empty()) {
    array.push_back(t.top());
    t.pop();
  }
  ar << BOOST_SERIALIZATION_NVP(array);
}

template<class Archive, class T, class U, class V>
inline void load(Archive& ar, priority_queue<T, U, V>& t, unsigned int){
  vector<T> array;
  ar >> BOOST_SERIALIZATION_NVP(array);
  for (T& elem : array) {
    t.push(std::move(elem));
  }
}

template<class Archive, class T, class U, class V>
inline void serialize(Archive& ar, priority_queue<T, U, V>& t, unsigned int file_version){
  boost::serialization::split_free(ar, t, file_version);
}

//queue
template<class Archive, class T>
inline void save(Archive& ar, queue<T> t, unsigned int file_version){
  int count = t.size();
  ar << BOOST_SERIALIZATION_NVP(count);
  while (!t.empty()) {
    ar << boost::serialization::make_nvp("elem", t.front());
    t.pop();
  }
}

template<class Archive, class T>
inline void load(Archive& ar, queue<T>& t, unsigned int){
  int count;
  ar >> BOOST_SERIALIZATION_NVP(count);
  while (count-- > 0) {
    T elem;
    ar >> boost::serialization::make_nvp("elem", elem);
    t.push(std::move(elem));
  }
}

template<class Archive, class T>
inline void serialize(Archive& ar, queue<T>& t, unsigned int file_version){
  boost::serialization::split_free(ar, t, file_version);
}

//stack
template<class Archive, class T>
inline void save(Archive& ar, stack<T> t, unsigned int file_version){
  int count = t.size();
  ar << BOOST_SERIALIZATION_NVP(count);
  while (!t.empty()) {
    ar << boost::serialization::make_nvp("elem", t.top());
    t.pop();
  }
}

template<class Archive, class T>
inline void load(Archive& ar, stack<T>& t, unsigned int){
  int count;
  ar >> BOOST_SERIALIZATION_NVP(count);
  stack<T> s;
  while (count-- > 0) {
    T elem;
    ar >> boost::serialization::make_nvp("elem", elem);
    s.push(std::move(elem));
  }
  while (!s.empty()) {
    t.push(std::move(s.top()));
    s.pop();
  }
}

template<class Archive, class T>
inline void serialize(Archive& ar, stack<T>& t, unsigned int file_version){
  boost::serialization::split_free(ar, t, file_version);
}

//unordered_set
template<class Archive, class T, class H>
inline void save(Archive& ar, const unordered_set<T, H>& t, unsigned int file_version){
  int count = t.size();
  ar << BOOST_SERIALIZATION_NVP(count);
  for (auto elem : t)
    ar << boost::serialization::make_nvp("item", elem);
}

template<class Archive, class T, class H>
inline void load(Archive& ar, unordered_set<T, H>& t, unsigned int){
  int count;
  ar >> BOOST_SERIALIZATION_NVP(count);
  t.clear();
  t.reserve(count);
  while (count-- > 0) {
    T elem;
    ar >> boost::serialization::make_nvp("item", elem);
    t.insert(elem);
  }
}

template<class Archive, class T, class H>
inline void serialize(Archive& ar, unordered_set<T, H>& t, unsigned int file_version){
  boost::serialization::split_free(ar, t, file_version);
}

// unique_ptr
template<class Archive, class T>
inline void save(Archive & ar, const std::unique_ptr< T > &t, unsigned int file_version){
  const T * const ptr = t.get();
  ar << BOOST_SERIALIZATION_NVP(ptr);
}

template<class Archive, class T>
inline void load(Archive & ar, std::unique_ptr< T > &t, unsigned int file_version){
  T *ptr;
  ar >> BOOST_SERIALIZATION_NVP(ptr);
  t.reset(ptr);
}

template<class Archive, class T>
inline void serialize(Archive & ar, std::unique_ptr< T > &t, unsigned int file_version){
  boost::serialization::split_free(ar, t, file_version);
}

// vector
template<class Archive, class T, class Allocator>
inline void save(Archive & ar, const std::vector<T, Allocator> &t, unsigned int){
  int count = t.size();
  ar << BOOST_SERIALIZATION_NVP(count);
  for(auto it=t.begin(), end=t.end(); it!=end; ++it)
    ar << boost::serialization::make_nvp("item", (*it));
}

template<class Archive, class T, class Allocator>
inline void load(Archive & ar, std::vector<T, Allocator> &t, unsigned int){
  int count;
  ar >> BOOST_SERIALIZATION_NVP(count);
  t.clear();
  t.reserve(count);
  while( count-- > 0 ) {
    T i;
    ar >> boost::serialization::make_nvp("item", i);
    t.push_back(std::move(i));
  }
}

template<class Archive, class T, class Allocator>
inline void serialize(Archive & ar, std::vector<T, Allocator> & t, unsigned int file_version){
  boost::serialization::split_free(ar, t, file_version);
}

#ifdef DEBUG_STL
// stl debug dummies

template<class Archive, class T>
inline void serialize(Archive & ar, __gnu_debug::vector< T > &t, unsigned int file_version) {
}

/*template<class Archive>
inline void serialize(Archive & ar, __gnu_debug::string &t, unsigned int file_version) {
}*/

template<class Archive, class T, class U>
inline void serialize(Archive & ar, __gnu_debug::map< T, U > &t, unsigned int file_version) {
}

template<class Archive, class T>
inline void serialize(Archive & ar, __gnu_debug::set< T > &t, unsigned int file_version) {
}

template<class Archive, class T>
inline void serialize(Archive & ar, __gnu_debug::deque< T > &t, unsigned int file_version) {
}

#endif

//tuple
template<int N>
struct Serialize {
    template<class Archive, typename... Args>
    static void serialize(Archive & ar, std::tuple<Args...> & t, const unsigned int version) {
        ar & boost::serialization::make_nvp("elem", std::get<N-1>(t));
        Serialize<N-1>::serialize(ar, t, version);
    }
};

template<>
struct Serialize<0> {
    template<class Archive, typename... Args>
    static void serialize(Archive & ar, std::tuple<Args...> & t, const unsigned int version) {
    }
};

template<class Archive, typename... Args>
void serialize(Archive & ar, std::tuple<Args...> & t, const unsigned int version) {
    Serialize<sizeof...(Args)>::serialize(ar, t, version);
}

}} // namespace boost::serialization

#endif
