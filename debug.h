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

#pragma once

#include <string>
#include <ostream>
#include <functional>

#define FATAL FatalLog.get() << "FATAL " << __FILE__ << ":" << __LINE__ << " "
#define INFO InfoLog.get() << __FILE__ << ":" <<  __LINE__ << " "
#define CHECK(exp) if (!(exp)) FATAL << ": " << #exp << " is false. "
//#define CHECKEQ(exp, exp2) if ((exp) != (exp2)) FATAL << __FILE__ << ":" << __LINE__ << ": " << #exp << " = " << #exp2 << " is false. " << exp << " " << exp2
//#define TRY(exp, msg) do { try { exp; } catch (...) { FATAL << __FILE__ << ":" << __LINE__ << ": " << #exp << " failed. " << msg; exp; } } while(0)

#ifndef WINDOWS

#define MEASURE(exp, text) do { \
  timeval time1; \
  gettimeofday(&time1, nullptr); \
  suseconds_t m1 = time1.tv_usec + time1.tv_sec * 1000000; \
  exp; \
  gettimeofday(&time1, nullptr); \
  suseconds_t m2 = time1.tv_usec + time1.tv_sec * 1000000; \
  INFO << text << " " << int(m2 - m1);} while(0);

#else

#define MEASURE(exp, text) exp;

#endif

#ifdef RELEASE
#define NO_RELEASE(exp)
#else
#define NO_RELEASE(exp) exp
#endif


template<class T>
std::ostream& operator<<(std::ostream& d, const vector<T>& container){
  d << "{";
  for (const T& elem : container) {
    d << elem << ",";
  }
  d << "}";
  return d;
}

inline std::ostream& operator<<(std::ostream& d, const milliseconds& millis) {
  return d << millis.count() << "ms";
}

template<class T>
std::ostream& operator<<(std::ostream& d, const vector<vector<T> >& container){
  d << "{";
  for (int i = 0; i < container[0].size(); ++i) {
    for (int j = 0; j < container[0].size(); ++i) {
      d << container[j][i] << ",";
    }
    d << '\n';
  }
  d << "}";
  return d;
}

class DebugOutput {
  public:
  static DebugOutput toStream(std::ostream&);
  static DebugOutput toString(function<void(const string&)> callback);
  static DebugOutput crash();

  typedef function<void()> LineEndFun;
  std::ostream& out;
  LineEndFun onLineEnd;

  private:
  DebugOutput(std::ostream& o, LineEndFun end) : out(o), onLineEnd(end) {}
};

class DebugLog {
  public:
  void addOutput(DebugOutput);

  class Logger {
    public:
    Logger(vector<DebugOutput>& s) : outputs(s) {}

    template <typename T>
    Logger& operator << (const T& t) {
      for (int i = outputs.size() - 1; i >= 0; --i)
        outputs[i].out << t;
      return *this;
    }
    ~Logger() {
      for (int i = outputs.size() - 1; i >= 0; --i)
        outputs[i].onLineEnd();
    }

    private:
    vector<DebugOutput>& outputs;
  };

  Logger get();

  private:
  vector<DebugOutput> outputs;
};

extern DebugLog InfoLog;
extern DebugLog FatalLog;

template <class T, class V>
const T& valueCheck(const T& e, const V& v, const string& msg) {
  if (e != v) FATAL << msg << " (" << e << " != " << v << ")";
  return e;
}

template <class T>
T* notNullCheck(T* e, const char* file, int line, const char* exp) {
  if (e == nullptr) FATAL << file << ": " << line << ": " << exp << " is null";
  return e;
}

template <class T>
unique_ptr<T> notNullCheck(unique_ptr<T> e, const char* file, int line, const char* exp) {
  if (e.get() == nullptr) FATAL << file << ": " << line << ": " << exp << " is null";
  return e;
}

#define NOTNULL(e) notNullCheck(e, __FILE__, __LINE__, #e)
#define CHECKEQ(e, v) valueCheck(e, v, string(__FILE__) + ":" + toString(__LINE__) + ": " + #e + " != " + #v + " ")
#define CHECKEQ2(e, v, msg) valueCheck(e, v, string(__FILE__) + ":" + toString(__LINE__) + ": " + #e + " != " + #v + " " + msg)

