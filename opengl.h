#define GL_GLEXT_PROTOTYPES
#include "sdl.h"

void checkOpenglError();

// This is very useful for debugging OpenGL-related errors
bool installOpenglDebugHandler();

bool isOpenglExtensionAvailable(const char*);
