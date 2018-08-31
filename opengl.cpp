#include "debug.h"
#include "opengl.h"
#include "util.h"
#include "color.h"

using SDL::GLenum;
using SDL::GLuint;

const char* openglErrorCode(int code) {
  const char* err_code = "unknown";
  switch (code) {
#define CASE(e)                                                                                                        \
  case GL_##e:                                                                                                         \
    return #e;
    CASE(INVALID_ENUM)
    CASE(INVALID_VALUE)
    CASE(INVALID_OPERATION)
    CASE(INVALID_FRAMEBUFFER_OPERATION)
    CASE(STACK_OVERFLOW)
    CASE(STACK_UNDERFLOW)
    CASE(OUT_OF_MEMORY)
  default:
    break;
#undef CASE
  }
  return "UNKNOWN";
}

void checkOpenglError(const char* file, int line) {
  auto error = SDL::glGetError();
  if (error != GL_NO_ERROR)
    FatalLog.get() << "FATAL " << file << ":" << line << " "
                   << "OpenGL error: " << openglErrorCode(error) << " (" << error << ")";
}

static const char* debugSourceText(GLenum source) {
  switch (source) {
#define CASE(suffix, text)                                                                                             \
  case GL_DEBUG_SOURCE_##suffix:                                                                                       \
    return text;
    CASE(API, "API")
    CASE(WINDOW_SYSTEM, "window system")
    CASE(SHADER_COMPILER, "shader compiler")
    CASE(THIRD_PARTY, "third party")
    CASE(APPLICATION, "application")
    CASE(OTHER, "other")
#undef CASE
  }

  return "unknown";
}

static const char* debugTypeText(GLenum type) {
  switch (type) {
#define CASE(suffix, text)                                                                                             \
  case GL_DEBUG_TYPE_##suffix:                                                                                         \
    return text;
    CASE(ERROR, "error")
    CASE(DEPRECATED_BEHAVIOR, "deprecated behavior")
    CASE(UNDEFINED_BEHAVIOR, "undefined behavior")
    CASE(PORTABILITY, "portability issue")
    CASE(PERFORMANCE, "performance issue")
    CASE(MARKER, "marker")
    CASE(PUSH_GROUP, "push group")
    CASE(POP_GROUP, "pop group")
    CASE(OTHER, "other issue")
#undef CASE
  }

  return "unknown";
}

static const char* debugSeverityText(GLenum severity) {
  switch (severity) {
#define CASE(suffix, text)                                                                                             \
  case GL_DEBUG_SEVERITY_##suffix:                                                                                     \
    return text;
    CASE(HIGH, "HIGH")
    CASE(MEDIUM, "medium")
    CASE(LOW, "low")
    CASE(NOTIFICATION, "notification")
#undef CASE
  }

  return "unknown";
}

static void APIENTRY debugOutputCallback(GLenum source, GLenum type, GLuint id, GLenum severity, SDL::GLsizei length,
                                         const SDL::GLchar* message, const void* userParam) {
  // TODO: ignore non-significant error/warning codes
  /*if(s_info.vendor == OpenglVendor::nvidia) {
		if(id == 131169 || id == 131185 || id == 131218 || id == 131204)
			return;
	} else if(s_info.vendor == OpenglVendor::intel) {
		Str msg((const char *)message);
		if(msg.find("warning: extension") != -1 && msg.find("unsupported in") != -1)
			return;
	}*/

  bool isSevere = severity == GL_DEBUG_SEVERITY_HIGH && type != GL_DEBUG_TYPE_OTHER;

  char header[1024];
  snprintf(header, sizeof(header), "%sOpengl %s [%s] id:%d source:%s\n", isSevere ? "FATAL: " : "", debugTypeText(type),
           debugSeverityText(severity), id, debugSourceText(source));
  (isSevere ? FatalLog : InfoLog).get() << header << message;
}

bool installOpenglDebugHandler() {
  static bool isInitialized = false, properlyInitialized = false;
  if (isInitialized)
    return properlyInitialized;
  isInitialized = true;

  if (isOpenglExtensionAvailable("KHR_debug")) {
    SDL::glEnable(GL_DEBUG_OUTPUT);
    SDL::glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    SDL::glDebugMessageCallback(debugOutputCallback, nullptr);
    SDL::glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
    SDL::glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);
    properlyInitialized = true;
    return true;
  }

  return false;
}

bool isOpenglExtensionAvailable(const char* text) {
  auto* exts = (const char*)SDL::glGetString(GL_EXTENSIONS);
  return strstr(exts, text) != nullptr;
}

void setupOpenglView(int width, int height, float zoom) {
  SDL::glMatrixMode(GL_PROJECTION);
  SDL::glLoadIdentity();
  SDL::glViewport(0, 0, width, height);
  SDL::glOrtho(0.0, double(width) / zoom, double(height) / zoom, 0.0, -1.0, 1.0);
  CHECK_OPENGL_ERROR();

  SDL::glMatrixMode(GL_MODELVIEW);
  SDL::glLoadIdentity();
}

void pushOpenglView() {
  SDL::glPushAttrib(GL_VIEWPORT_BIT);
  SDL::glMatrixMode(GL_PROJECTION);
  SDL::glPushMatrix();
  SDL::glMatrixMode(GL_MODELVIEW);
  SDL::glPushMatrix();
}

void popOpenglView() {
  SDL::glPopAttrib();
  SDL::glMatrixMode(GL_PROJECTION);
  SDL::glPopMatrix();
  SDL::glMatrixMode(GL_MODELVIEW);
  SDL::glPopMatrix();
}

void glColor(const Color& col) {
  SDL::glColor4f((float)col.r / 255, (float)col.g / 255, (float)col.b / 255, (float)col.a / 255);
}

void glQuad(float x, float y, float ex, float ey) {
  SDL::glBegin(GL_QUADS);
  SDL::glTexCoord2f(0.0f, 0.0f), SDL::glVertex2f(x, ey);
  SDL::glTexCoord2f(1.0f, 0.0f), SDL::glVertex2f(ex, ey);
  SDL::glTexCoord2f(1.0f, 1.0f), SDL::glVertex2f(ex, y);
  SDL::glTexCoord2f(0.0f, 1.0f), SDL::glVertex2f(x, y);
  SDL::glEnd();
}
