#pragma once

namespace SDL {
#ifdef VSTUDIO
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_opengl.h>
#include <SDL_opengl_glext.h>
#else
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_opengl_glext.h>
#endif
}

#undef TECHNOLOGY
#undef TRANSPARENT

using SDL::Uint8;

typedef SDL::SDL_Event Event;
typedef SDL::SDL_EventType EventType;
