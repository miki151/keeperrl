#pragma once

#ifdef EASY_PROFILER
#define BUILD_WITH_EASY_PROFILER

#include <easy/profiler.h>


#define PROFILE EASY_FUNCTION(__LINE__)
#define PROFILE_BLOCK(...) EASY_BLOCK(__VA_ARGS__)

#define ENABLE_PROFILER\
  profiler::startListen()
/*  EASY_PROFILER_ENABLE\
  DestructorFunction dumpProfileData([]{profiler::dumpBlocksToFile("test_profile.prof");})
*/
#else

#define PROFILE
#define PROFILE_BLOCK(...)
#define ENABLE_PROFILER

#endif
