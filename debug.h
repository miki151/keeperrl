#ifndef _DEBUG_H
#define _DEBUG_H

#include <string>

#define DEBUG

#ifdef DEBUG

#define CHECK(exp) if (!(exp)) Debug(FATAL) << __FILE__ << ":" << __LINE__ << ": " << #exp << " is false. "
//#define CHECKEQ(exp, exp2) if ((exp) != (exp2)) Debug(FATAL) << __FILE__ << ":" << __LINE__ << ": " << #exp << " = " << #exp2 << " is false. " << exp << " " << exp2
#define TRY(exp, msg) do { try { exp; } catch (...) { Debug(FATAL) << __FILE__ << ":" << __LINE__ << ": " << #exp << " failed. " << msg; exp; } } while(0)

#else

#define CHECK(exp) if (!(exp)) NoDebug() << 1
#define TRY(exp, msg) exp
#endif

#ifndef WINDOWS

#define MEASURE(exp, text) do { \
  timeval time1; \
  gettimeofday(&time1, nullptr); \
  suseconds_t m1 = time1.tv_usec + time1.tv_sec * 1000000; \
  exp; \
  gettimeofday(&time1, nullptr); \
  suseconds_t m2 = time1.tv_usec + time1.tv_sec * 1000000; \
  Debug() << text << " " << int(m2 - m1);} while(0);

#else

#define MEASURE(exp, text) exp;

#endif

enum DebugType { INFO, FATAL };

class NoDebug {
  public:
  NoDebug& operator <<(const string& msg) { return *this;}
  NoDebug& operator <<(const int msg) {return *this;}
  NoDebug& operator <<(const char msg) {return *this;}
  NoDebug& operator <<(const double msg) {return *this;}
  template<class T>
  NoDebug& operator<<(const vector<T>& container) {return *this;}
  template<class T>
  NoDebug& operator<<(const vector<vector<T> >& container) {return *this;}
};

class Debug {
  public:
  Debug(DebugType t = INFO);
  static void init();
  Debug& operator <<(const string& msg);
  Debug& operator <<(const int msg);
  Debug& operator <<(const char msg);
  Debug& operator <<(const double msg);
  template<class T>
  Debug& operator<<(const vector<T>& container);
  template<class T>
  Debug& operator<<(const vector<vector<T> >& container);
  ~Debug();

  private:
  string out;
  DebugType type;
  void add(const string& a);
};

template <class T, class V>
const T& valueCheck(const T& e, const V& v, const string& msg) {
  if (e != v) Debug(FATAL) << msg << " (" << e << " != " << v << ")";
  return e;
}

template <class T>
T* notNullCheck(T* e, const string& msg) {
  if (e == nullptr) Debug(FATAL) << msg;
  return e;
}

template <class T>
unique_ptr<T> notNullCheck(unique_ptr<T> e, const string& msg) {
  if (e.get() == nullptr) Debug(FATAL) << msg;
  return e;
}

#define NOTNULL(e) notNullCheck(e, string(__FILE__) + ":" + convertToString(__LINE__) + ": " + #e + " is null.")
#define CHECKEQ(e, v) valueCheck(e, v, string(__FILE__) + ":" + convertToString(__LINE__) + ": " + #e + " != " + #v + " ")
#define CHECKEQ2(e, v, msg) valueCheck(e, v, string(__FILE__) + ":" + convertToString(__LINE__) + ": " + #e + " != " + #v + " " + msg)

#endif
