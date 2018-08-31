#pragma once

#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES 1
#endif

namespace SDL {
#ifdef VSTUDIO
#include <SDL_opengl.h>
#include <SDL_opengl_glext.h>
#else
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_opengl_glext.h>
#endif
}

struct Color;

// TODO: consistent naming

void checkOpenglError(const char* file, int line);
#define CHECK_OPENGL_ERROR() checkOpenglError(__FILE__, __LINE__)

// This is very useful for debugging OpenGL-related errors
bool installOpenglDebugHandler();

bool isOpenglExtensionAvailable(const char*);

void setupOpenglView(int width, int height, float zoom);
void pushOpenglView();
void popOpenglView();
void glColor(const Color&);
