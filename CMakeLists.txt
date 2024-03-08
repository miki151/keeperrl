cmake_minimum_required(VERSION 3.14)

set(BUILD_SHARED_LIBS off)
set(BUILD_STATIC_LIBS on)

project(KeeperRL)

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(CMAKE_CXX_STANDARD 17)
else()
    set(CMAKE_CXX_STANDARD 14)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED on)

option(ENABLE_STEAMWORKS "Enable steamworks integration" ON)

if(ENABLE_STEAMWORKS)
    add_library(steamworks SHARED IMPORTED)
    if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
        set_target_properties(steamworks PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/extern/steamworks/redistributable_bin/win64/steam_api64.dll")
        set_target_properties(steamworks PROPERTIES IMPORTED_IMPLIB "${CMAKE_CURRENT_SOURCE_DIR}/extern/steamworks/redistributable_bin/win64/steam_api64.lib")
        install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/extern/steamworks/redistributable_bin/win64/steam_api64.dll" TYPE BIN)
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        set_target_properties(steamworks PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/extern/steamworks/redistributable_bin/linux64/libsteam_api.so")
    endif()
    target_include_directories(steamworks INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/extern/steamworks/public")
endif()

include(FetchContent)

FetchContent_Declare(SDL2 GIT_REPOSITORY https://github.com/libsdl-org/SDL GIT_TAG release-2.30.1)
FetchContent_Declare(SDL2_image GIT_REPOSITORY https://github.com/libsdl-org/SDL_image GIT_TAG release-2.8.2)
FetchContent_Declare(CURL GIT_REPOSITORY https://github.com/curl/curl GIT_TAG curl-8_6_0)
FetchContent_Declare(ZLIB GIT_REPOSITORY https://github.com/madler/zlib GIT_TAG v1.3.1)
FetchContent_Declare(OGG GIT_REPOSITORY https://github.com/xiph/ogg GIT_TAG v1.3.5)
FetchContent_Declare(VORBIS GIT_REPOSITORY https://github.com/xiph/vorbis GIT_TAG v1.3.7)
FetchContent_Declare(THEORA GIT_REPOSITORY https://github.com/xiph/theora GIT_TAG v1.2.0alpha1)
FetchContent_MakeAvailable(SDL2 SDL2_image CURL ZLIB OGG VORBIS THEORA)

find_package(OpenGL REQUIRED)
find_package(OpenAL REQUIRED)

add_library(
    theoradec
    "${theora_SOURCE_DIR}/lib/apiwrapper.c"
	"${theora_SOURCE_DIR}/lib/bitpack.c" "${theora_SOURCE_DIR}/lib/dequant.c"
	"${theora_SOURCE_DIR}/lib/fragment.c" "${theora_SOURCE_DIR}/lib/idct.c"
	"${theora_SOURCE_DIR}/lib/info.c" "${theora_SOURCE_DIR}/lib/internal.c"
	"${theora_SOURCE_DIR}/lib/state.c" "${theora_SOURCE_DIR}/lib/quant.c"
	"${theora_SOURCE_DIR}/lib/decapiwrapper.c" "${theora_SOURCE_DIR}/lib/decinfo.c"
	"${theora_SOURCE_DIR}/lib/decode.c" "${theora_SOURCE_DIR}/lib/huffdec.c"
)
target_link_libraries(theoradec PRIVATE ogg)
target_include_directories(theoradec PUBLIC ${theora_SOURCE_DIR}/include)

# generate version.h
set(KEEPER_VERSION_H "${CMAKE_CURRENT_SOURCE_DIR}/version.h")
string(TIMESTAMP BUILD_DATE "%Y-%m-%d")
execute_process(COMMAND git describe --abbrev=4 --dirty --always OUTPUT_VARIABLE GIT_HASH OUTPUT_STRIP_TRAILING_WHITESPACE)
configure_file(
    ${KEEPER_VERSION_H}.in
    ${KEEPER_VERSION_H}
    @ONLY
)

file(GLOB KEEPER_SOURCES "*.cpp" "extern/*.cpp")
add_executable(
    keeper
    ${KEEPER_SOURCES}
    ${KEEPER_VERSION_H}
)
target_include_directories(keeper PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(keeper PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/extern)
target_include_directories(keeper PRIVATE ${sdl2_image_SOURCE_DIR}/include ${theora_SOURCE_DIR}/include)
target_link_libraries(
    keeper
    PRIVATE
    SDL2-static
    SDL2main
    SDL2_image
    libcurl_static
    zlibstatic
    ogg
    vorbis
    vorbisfile
    theoradec
    OpenGL::GL
    OpenAL::OpenAL
)
target_precompile_headers(keeper PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/stdafx.h")
if(ENABLE_STEAMWORKS)
    target_link_libraries(keeper PRIVATE steamworks)
    target_compile_definitions(keeper PRIVATE USE_STEAMWORKS)

    if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
        add_custom_command(
            TARGET keeper
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different 
                "${CMAKE_CURRENT_SOURCE_DIR}/extern/steamworks/redistributable_bin/win64/steam_api64.dll"
                "$<TARGET_FILE_DIR:keeper>"
        )
    endif()
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_compile_definitions(keeper PRIVATE WINDOWS $<$<CXX_COMPILER_ID:MSVC>:_SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING>)
    target_compile_options(keeper PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/w>)
    target_link_libraries(keeper PRIVATE dbghelp)

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

if (CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDbgInfo")
    target_compile_definitions(keeper PRIVATE RELEASE=1)
endif()




install(
    TARGETS keeper
    RUNTIME
)
