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

#ifndef _STDAFX_H
#define _STDAFX_H

#include <iostream>
#include <fstream>
#include <algorithm>
#include <sstream>
#include <string>
#include <cstdlib>
#include <memory>
#include "ctype.h"
#include <cstring>
#include <time.h>
#include <cstdlib>
#include <sys/time.h>
#include <typeinfo>
#include <unordered_set>
#include <unordered_map>
#include <queue>
#include <random>
#include <stack>
#include <stdexcept>
#include <tuple>

// Use boost threads on OSX to use the main thread for rendering
// and set a large stack size for the model thread.
#ifdef OSX
#include <boost/thread.hpp>
#else
#include <thread>
#endif

#include <typeinfo>
#include <tuple>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include "extern/portable_iarchive.hpp"
#include "extern/portable_oarchive.hpp"


#define BOOST_ENABLE_ASSERT_HANDLER
#include <boost/assert.hpp>

#include <boost/serialization/export.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/deque.hpp>
#include <boost/serialization/set.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/array.hpp>
#include <boost/serialization/variant.hpp>
#include <boost/serialization/optional.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/variant.hpp>
#include <boost/any.hpp>
#include <boost/optional.hpp>
#include <boost/version.hpp>

#ifdef DEBUG_STL

#include <debug/vector>
//#include <debug/string>
#include <string>
#include <debug/deque>
#include <debug/set>
#include <debug/map>
using __gnu_debug::vector;
using __gnu_debug::map;
using __gnu_debug::set;
using __gnu_debug::deque;
//using __gnu_debug::string;
using std::string;

#else

#include <vector>
#include <string>
#include <deque>
#include <set>
#include <map>
using std::vector;
using std::map;
using std::set;
using std::deque;
using std::string;
using boost::none;

#endif


using std::queue;
using std::unique_ptr;
using std::default_random_engine;
using std::function;
using std::initializer_list;
using std::unordered_set;
using std::pair;
using std::tuple;
using std::out_of_range;
using std::unordered_map;
using std::min;
using std::max;
using std::ofstream;
using std::ifstream;
using std::endl;
using std::priority_queue;
using std::make_pair;
using std::stack;
using std::uniform_int_distribution;
using std::uniform_real_distribution;
using std::make_tuple;
using std::get;
using std::hash;
using std::array;
#ifdef OSX
using boost::thread;
inline thread::id currentThreadId() { return boost::this_thread::get_id(); }
#else
using std::thread;
inline thread::id currentThreadId() { return std::this_thread::get_id(); }
#endif
using std::atomic;
using std::swap;
using std::remove_if;
using boost::variant;
using boost::optional;
using boost::none_t;
using boost::replace_all;
using eos::portable_oarchive;
using eos::portable_iarchive;

#include "serialization.h"
#endif
