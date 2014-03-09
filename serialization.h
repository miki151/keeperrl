#ifndef _SERIALIZATION
#define _SERIALIZATION

#define SUBCLASS(X) boost::serialization::make_nvp("Base", boost::serialization::base_object<X>(*this))

#define SERIALIZABLE(T) \
   template void T::serialize(boost::archive::binary_iarchive&, unsigned); \
   template void T::serialize(boost::archive::binary_oarchive&, unsigned);
 //  template void T::serialize(boost::archive::xml_iarchive&, unsigned);
 //  template void T::serialize(boost::archive::xml_oarchive&, unsigned);

#define REGISTER_TYPES(T) \
   template void T::registerTypes(boost::archive::binary_iarchive&); \
   template void T::registerTypes(boost::archive::binary_oarchive&);
 //  template void T::registerTypes(boost::archive::xml_iarchive&);
//   template void T::registerTypes(boost::archive::xml_oarchive&);

#define REGISTER_TYPE(T, A)\
  (T).register_type(static_cast<A*>(nullptr))

#define SERIALIZATION_DECL(A) \
  friend boost::serialization::access; \
  A() {} \
  template <class Archive> \
  void serialize(Archive& ar, const unsigned int version);

#define SERIALIZATION_CONSTRUCTOR(A) \
  A() {}

class Serialization {
  public:
  template <class Archive>
  static void registerTypes(Archive& ar);
};

class SerialChecker {
  public:

  void checkSerial();

  class Check {
    public:
    Check(SerialChecker& checker, const string& name);
    void tick();
    void tickOff();

    private:
    bool ticked = false;
    string name;
  };

  friend class Check;
  private:
  vector<Check*> checks;
};

#define SERIAL_CHECKER SerialChecker serialChecker
#define CHECK_SERIAL serialChecker.checkSerial()
#define SERIAL(X) X; SerialChecker::Check X##_Check = SerialChecker::Check(serialChecker,#X)
#define SERIAL2(X, Y) X = Y; SerialChecker::Check X##_Check = SerialChecker::Check(serialChecker,#X)

template <class T>
T& checkSerial(T& t, SerialChecker::Check& check) {
  check.tick();
  return t;
}

#define SVAR(X) checkSerial(X, X##_Check)


namespace boost { 
namespace serialization {


// why the f** are these things not implemented by default in boost?
//unordered_map
template<class Archive, class T, class U>
inline void save(Archive& ar, const unordered_map<T, U>& t, unsigned int file_version){
  int count = t.size();
  ar << BOOST_SERIALIZATION_NVP(count);
  for (auto& elem : t)
    ar << boost::serialization::make_nvp("key", elem.first) << boost::serialization::make_nvp("value", elem.second);
}

template<class Archive, class T, class U>
inline void load(Archive& ar, unordered_map<T, U>& t, unsigned int){
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

template<class Archive, class T, class U>
inline void serialize(Archive& ar, unordered_map<T, U>& t, unsigned int file_version){
  boost::serialization::split_free(ar, t, file_version);
}

//priority queue
template<class Archive, class T, class U, class V>
inline void save(Archive& ar, priority_queue<T, U, V> t, unsigned int file_version){
  int count = t.size();
  ar << BOOST_SERIALIZATION_NVP(count);
  T array[count];
  int cnt = 0;
  while (!t.empty()) {
    array[cnt++] = t.top();
    t.pop();
  }
  ar << boost::serialization::make_array(array, count);
}

template<class Archive, class T, class U, class V>
inline void load(Archive& ar, priority_queue<T, U, V>& t, unsigned int){
  int count;
  ar >> BOOST_SERIALIZATION_NVP(count);
  T array[count];
  ar >> boost::serialization::make_array(array, count);
  for (int i = 0; i < count; ++i) {
    t.push(std::move(array[i]));
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
template<class Archive, class T>
inline void save(Archive& ar, const unordered_set<T>& t, unsigned int file_version){
  int count = t.size();
  ar << BOOST_SERIALIZATION_NVP(count);
  for (auto elem : t)
    ar << boost::serialization::make_nvp("item", elem);
}

template<class Archive, class T>
inline void load(Archive& ar, unordered_set<T>& t, unsigned int){
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

template<class Archive, class T>
inline void serialize(Archive& ar, unordered_set<T>& t, unsigned int file_version){
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

//tuple
template<uint N>
struct Serialize
{
    template<class Archive, typename... Args>
    static void serialize(Archive & ar, std::tuple<Args...> & t, const unsigned int version)
    {
        ar & boost::serialization::make_nvp("elem", std::get<N-1>(t));
        Serialize<N-1>::serialize(ar, t, version);
    }
};

template<>
struct Serialize<0>
{
    template<class Archive, typename... Args>
    static void serialize(Archive & ar, std::tuple<Args...> & t, const unsigned int version)
    {
    }
};

template<class Archive, typename... Args>
void serialize(Archive & ar, std::tuple<Args...> & t, const unsigned int version)
{
    Serialize<sizeof...(Args)>::serialize(ar, t, version);
}

}} // namespace boost::serialization

#endif
