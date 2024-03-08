cmake_minimum_required (VERSION 3.20)

project(keeper)

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(CMAKE_CXX_STANDARD 17)
else()
    set(CMAKE_CXX_STANDARD 14)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED on)

option(ENABLE_STEAMWORKS "Enable steamworks integration" off)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(STDAFX_H "${CMAKE_CURRENT_SOURCE_DIR}/stdafx.h")
set(STDAFX_H_GCH "${CMAKE_CURRENT_BINARY_DIR}/stdafx.h.gch")

# additional cmake modules directory
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

if(ENABLE_STEAMWORKS)
    add_library(steamworks SHARED IMPORTED)
    if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
        set_target_properties(steamworks PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/extern/steamworks/redistributable_bin/win64/steam_api64.dll")
        set_target_properties(steamworks PROPERTIES IMPORTED_IMPLIB "${CMAKE_CURRENT_SOURCE_DIR}/extern/steamworks/redistributable_bin/win64/steam_api64.lib")
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        set_target_properties(steamworks PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/extern/steamworks/redistributable_bin/linux64/libsteam_api.so")
    endif()
    target_include_directories(steamworks INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/extern/steamworks/public")
endif()

# find required libraries
find_package(SDL2 REQUIRED)
find_package(SDL2_image REQUIRED)
find_package(OpenGL REQUIRED)
if(APPLE)
    # assume built-in pthreads on MacOS
    set(CMAKE_THREAD_LIBS_INIT "-lpthread")
    set(CMAKE_HAVE_THREADS_LIBRARY 1)
    set(CMAKE_USE_WIN32_THREADS_INIT 0)
    set(CMAKE_USE_PTHREADS_INIT 1)
    set(THREADS_PREFER_PTHREAD_FLAG ON)
else()
    find_package(Threads REQUIRED)
endif()
find_package(CURL REQUIRED)
find_package(ZLIB REQUIRED)
find_package(OpenAL REQUIRED)
find_package(Vorbis REQUIRED)
find_package(Ogg REQUIRED)
find_package(VorbisFile REQUIRED)
find_package(THEORA REQUIRED)

# generate version.h
set(KEEPER_VERSION_H "${CMAKE_CURRENT_SOURCE_DIR}/version.h")
string(TIMESTAMP BUILD_DATE "%Y-%m-%d")
execute_process(COMMAND git describe --abbrev=4 --dirty --always OUTPUT_VARIABLE GIT_HASH OUTPUT_STRIP_TRAILING_WHITESPACE)
configure_file(
    ${KEEPER_VERSION_H}.in
    ${KEEPER_VERSION_H}
    @ONLY
)

# check member serialization
# the stamp file is not actually generated, but it doesn't seem to matter
if(ENABLE_CHECK_SERIAL)
    file(GLOB CHECK_SERIAL_SH "check_serial.sh")
    set(CHECK_SERIAL_STAMP "${CMAKE_CURRENT_SOURCE_DIR}/check_serial.stamp")
    add_custom_command(
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/
        COMMAND bash ${CHECK_SERIAL_SH}
        DEPENDS ${CHECK_SERIAL_SH}
        OUTPUT ${CHECK_SERIAL_STAMP}
        COMMENT "Checking serialization"
    )
endif()

# rules to create `keeper` binary
file(GLOB SOURCES "*.cpp" "extern/*.cpp")
add_executable(keeper ${SOURCES} ${KEEPER_VERSION_H} ${CHECK_SERIAL_STAMP})
target_include_directories(keeper PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(keeper PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/extern)
target_include_directories(keeper PRIVATE ${SDL2_INCLUDE_DIRS})
target_include_directories(keeper PRIVATE ${OPENAL_INCLUDE_DIRS})
target_link_libraries(keeper PRIVATE 
    SDL2::SDL2
    ${SDL2_IMAGE_LIBRARY}
    OpenGL::GL
    ${CMAKE_THREAD_LIBS_INIT}
    CURL::libcurl
    ${ZLIB_LIBRARIES}
    ${OPENAL_LIBRARY}
    ${VORBIS_LIBRARIES}
    ${OGG_LIBRARIES}
    ${THEORA_dec_LIBRARY}
    ${VorbisFile_LIBRARIES}
    )
if(ENABLE_STEAMWORKS)
    target_link_libraries(keeper PRIVATE steamworks)
    target_compile_definitions(keeper PRIVATE USE_STEAMWORKS)
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_compile_definitions(keeper PRIVATE WINDOWS)
    target_link_libraries(keeper PRIVATE dbghelp)
endif()
target_compile_options(keeper PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/w>)
target_compile_definitions(keeper PRIVATE $<$<CXX_COMPILER_ID:MSVC>:_SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING>)
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        set_source_files_properties(
            content_factory.cpp 
            player_control.cpp 
            gui_builder.cpp 
            pretty_printing.cpp 
            task.cpp
            PROPERTIES 
                COMPILE_FLAGS /bigobj
        )
    else()
        set_source_files_properties(
            content_factory.cpp 
            player_control.cpp 
            gui_builder.cpp 
            pretty_printing.cpp 
            task.cpp
            PROPERTIES 
                COMPILE_FLAGS -Wa,-mbig-obj
        )
    endif()
endif()

# set up definitions
if ((${CMAKE_BUILD_TYPE} MATCHES "Release"))
    target_compile_definitions(keeper PRIVATE RELEASE=1)
endif()
target_precompile_headers(keeper PRIVATE ${STDAFX_H})

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
