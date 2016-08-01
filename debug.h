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

#ifndef _DEBUG_H
#define _DEBUG_H

#include <string>

#define DEBUG

#ifdef DEBUG

#define FAIL Debug(FATAL, __FILE__, __LINE__)
#define CHECK(exp) if (!(exp)) Debug(FATAL, __FILE__, __LINE__) << ": " << #exp << " is false. "
//#define CHECKEQ(exp, exp2) if ((exp) != (exp2)) Debug(FATAL) << __FILE__ << ":" << __LINE__ << ": " << #exp << " = " << #exp2 << " is false. " << exp << " " << exp2
#define TRY(exp, msg) do { try { exp; } catch (...) { Debug(FATAL) << __FILE__ << ":" << __LINE__ << ": " << #exp << " failed. " << msg; exp; } } while(0)

#else

#define CHECK(exp) exp
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

#ifdef RELEASE
#define NO_RELEASE(exp)
#else
#define NO_RELEASE(exp) exp
#endif

enum DebugType { INFO, FATAL };

class NoDebug {
  public:
  NoDebug& operator <<(const string& msg) { return *this;}
  NoDebug& operator <<(const int msg) {return *this;}
  NoDebug& operator <<(const long long msg) {return *this;}
  NoDebug& operator <<(const size_t msg) {return *this;}
  NoDebug& operator <<(const char msg) {return *this;}
  NoDebug& operator <<(const double msg) {return *this;}
  template<class T>
  NoDebug& operator<<(const vector<T>& container) {return *this;}
  template<class T>
  NoDebug& operator<<(const vector<vector<T> >& container) {return *this;}
};

class Debug {
  public:
  Debug(DebugType t = INFO, const string& msg = "", int line = 0);
  static void init(bool log);
  static void setErrorCallback(function<void(const string&)>);
  Debug& operator <<(const string& msg);
  Debug& operator <<(const int msg);
  Debug& operator <<(const long long msg);
  Debug& operator <<(const size_t msg);
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
T* notNullCheck(T* e, const char* file, int line, const char* exp) {
  if (e == nullptr) Debug(FATAL) << file << ": " << line << ": " << exp << " is null";
  return e;
}

template <class T>
unique_ptr<T> notNullCheck(unique_ptr<T> e, const char* file, int line, const char* exp) {
  if (e.get() == nullptr) Debug(FATAL) << file << ": " << line << ": " << exp << " is null";
  return e;
}

template <class T, class V>
const T& rangeCheck(const T& e, const V& lower, const V& upper, const string& msg) {
  if (e < lower || e > upper) Debug(FATAL) << msg << " (" << e << " not in range " << lower << "," << upper << ")";
  return e;
}

#define NOTNULL(e) notNullCheck(e, __FILE__, __LINE__, #e)
#define CHECKEQ(e, v) valueCheck(e, v, string(__FILE__) + ":" + toString(__LINE__) + ": " + #e + " != " + #v + " ")
#define CHECKEQ2(e, v, msg) valueCheck(e, v, string(__FILE__) + ":" + toString(__LINE__) + ": " + #e + " != " + #v + " " + msg)
#define CHECK_RANGE(e, lower, upper, msg) rangeCheck(e, lower, upper, string(__FILE__) + ":" + toString(__LINE__) + ": " + msg)

#endif
