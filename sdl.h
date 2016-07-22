#pragma once

namespace SDL {
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
}

#undef TECHNOLOGY
#undef TRANSPARENT

using SDL::Uint8;

typedef SDL::SDL_Event Event;
typedef SDL::SDL_EventType EventType;
