#pragma once

#include "sdl.h"

struct Color;

// TODO: consistent naming
const char* openglErrorCode(SDL::GLenum code);
void checkOpenglError(const char* file, int line);
#define CHECK_OPENGL_ERROR() checkOpenglError(__FILE__, __LINE__)

// This is very useful for debugging OpenGL-related errors
bool installOpenglDebugHandler();

bool isOpenglExtensionAvailable(const char*);

void setupOpenglView(int width, int height, float zoom);
void pushOpenglView();
void popOpenglView();
void glColor(const Color&);
void glQuad(float x, float y, float ex, float ey);
void initializeGLExtensions();

enum class OpenglFeature { FRAMEBUFFER, SEPARATE_BLEND_FUNC, DEBUG };
bool isOpenglFeatureAvailable(OpenglFeature);

#ifdef WINDOWS

namespace SDL {
#define EXT_ENTRY __stdcall
#define EXT_API extern

extern "C" {
EXT_API void(EXT_ENTRY *glDeleteFramebuffers)(GLsizei n, const GLuint *framebuffers);
EXT_API void(EXT_ENTRY *glGenFramebuffers)(GLsizei n, GLuint *framebuffers);
EXT_API void(EXT_ENTRY *glBindFramebuffer)(GLenum target, GLuint framebuffer);
EXT_API void(EXT_ENTRY *glFramebufferTexture2D)(GLenum target, GLenum attachment, GLenum textarget,
    GLuint texture, GLint level);
EXT_API void(EXT_ENTRY *glDrawBuffers)(GLsizei n, const GLenum *bufs);
EXT_API void(EXT_ENTRY *glBlendFuncSeparate)(GLenum, GLenum, GLenum, GLenum);
EXT_API GLenum(EXT_ENTRY *glCheckFramebufferStatus)(GLenum target);
EXT_API void(EXT_ENTRY *glDebugMessageCallback)(GLDEBUGPROC callback, const void *userParam);
EXT_API void(EXT_ENTRY *glDebugMessageControl)(GLenum source, GLenum type, GLenum severity,
    GLsizei count, const GLuint *ids, GLboolean enabled);
}
#undef EXT_API
#undef EXT_ENTRY

}

#endif

