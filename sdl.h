#pragma once

namespace SDL {
#ifdef VSTUDIO
#include <SDL.h>
#include <SDL_image.h>
#else
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#endif
}

#undef TECHNOLOGY
#undef TRANSPARENT

using SDL::Uint8;

typedef SDL::SDL_Event Event;
typedef SDL::SDL_EventType EventType;
