cmake_minimum_required (VERSION 2.6)
project(keeper)

# set default cxxflags
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++1y -Wno-sign-compare -Wno-unused-variable -Wno-shift-count-overflow -ftemplate-depth=512")
# clang specific cflags
if(${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-tautological-constant-out-of-range-compare -Wno-mismatched-tags")
endif()

set(STDAFX_H "${CMAKE_CURRENT_SOURCE_DIR}/stdafx.h")
set(STDAFX_H_GCH "${CMAKE_CURRENT_BINARY_DIR}/stdagx.h.gch")

# set debug cxxflags
set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wimplicit-fallthrough -Wno-unused-function")
#if ((${CMAKE_BUILD_TYPE} MATCHES "Debug") AND (${CMAKE_CXX_COMPILER_ID} MATCHES "Clang"))
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -L ${CMAKE_CURRENT_SOURCE_DIR}/extern/steamworks/redistributable_bin/linux64/ -lsteam_api -Wl,-rpath=.")
  #endif()

# additional cmake modules directory
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

# find required libraries
find_package(SDL2 REQUIRED)
find_package(SDL2_image REQUIRED)
find_package(OpenGL REQUIRED)
find_package (Threads REQUIRED)
find_package(CURL REQUIRED)
find_package(ZLIB REQUIRED)
find_package(OpenAL REQUIRED)
find_package(Vorbis REQUIRED)
find_package(VorbisFile REQUIRED)

# generate version.h
file(GLOB GENERATE_VERSION_SH "gen_version.sh")
set(KEEPER_VERSION_H "${CMAKE_CURRENT_SOURCE_DIR}/version.h")
add_custom_command(
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/
    COMMAND ${GENERATE_VERSION_SH}
    DEPENDS ${GENERATE_VERSION_SH}
    OUTPUT ${KEEPER_VERSION_H}
    COMMENT "Generating version.h"
)


# rules to create `keeper` binary
file(GLOB SOURCES "*.cpp" "extern/*.cpp")
add_executable(keeper ${SOURCES} ${KEEPER_VERSION_H})
target_include_directories(keeper PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(keeper PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/extern)
target_include_directories(keeper PRIVATE ${SDL2_INCLUDE_DIRS})
target_link_libraries(keeper PRIVATE 
    ${SDL2_LIBRARIES}
    ${SDL2_IMAGE_LIBRARY}
    ${OPENGL_gl_LIBRARY}
    ${CMAKE_THREAD_LIBS_INIT}
    ${CURL_LIBRARIES}
    ${ZLIB_LIBRARIES}
    ${OPENAL_LIBRARY}
    ${VORBIS_LIBRARIES}
    ${VorbisFile_LIBRARIES}
    )

# set up definitions
if ((${CMAKE_BUILD_TYPE} MATCHES "Release"))
    target_compile_definitions(keeper PRIVATE RELEASE=1)
else()
# generate stdafx.h.gch for debug build
# workaround to unquote the cxxflags
set(CXX_FLAGS_LIST ${CMAKE_CXX_FLAGS})
separate_arguments(CXX_FLAGS_LIST)
add_custom_command(
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/
    COMMAND ${CMAKE_CXX_COMPILER} ${CXX_FLAGS_LIST} -x c++-header ${STDAFX_H} -MMD -o ${STDAFX_H_GCH}
    DEPENDS ${STDAFX_H}
    OUTPUT ${STDAFX_H_GCH}
    COMMENT "Generating ${STDAFX_H_GCH}"
)
if(${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -include-pch ${STDAFX_H_GCH}")
endif()
target_sources(keeper PRIVATE ${STDAFX_H_GCH})
endif()

if(DATA_DIR)
    target_compile_definitions(keeper PRIVATE DATA_DIR=${DATA_DIR})
endif()
if(USER_DIR)
    target_compile_definitions(keeper PRIVATE USER_DIR=${USER_DIR})
endif()
if(DEBUG_STL)
    target_compile_definitions(keeper PRIVATE DEBUG_STL=1)
endif()
if(TEXT_SERIALIZATION)
    target_compile_definitions(keeper PRIVATE TEXT_SERIALIZATION=1)
endif()
if(ENABLE_LOCAL_USER_DIR)
    target_compile_definitions(keeper PRIVATE ENABLE_LOCAL_USER_DIR=1)
endif()
if(SANITIZE)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
endif()
if(PROF)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
endif()
if(EASY_PROFILER)
    target_compile_definitions(keeper PRIVATE EASY_PROFILER=1)
    target_link_libraries(keeper PRIVATE libeasy_profiler)
endif()
