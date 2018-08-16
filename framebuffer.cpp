#include "framebuffer.h"

#include "debug.h"
#include "opengl.h"

bool Framebuffer::isExtensionAvailable() {
  return isOpenglExtensionAvailable("GL_ARB_framebuffer_object");
}

static unsigned allocFramebuffer() {
  SDL::GLuint id = 0;
  SDL::glGenFramebuffers(1, &id);
  return id;
}

static unsigned allocTexture(int width, int height) {
  SDL::GLuint id = 0;
  SDL::glGenTextures(1, &id);

  // TODO: something better than 8bits per component for blending ?
  // TODO: power of 2 textures
  SDL::glBindTexture(GL_TEXTURE_2D, id);
  SDL::glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  SDL::glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  SDL::glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
  SDL::glBindTexture(GL_TEXTURE_2D, 0);

  return id;
}

Framebuffer::Framebuffer(int width, int height)
    : width(width), height(height), id(allocFramebuffer()), texId(allocTexture(width, height)) {
  SDL::glBindFramebuffer(GL_FRAMEBUFFER, id);
  SDL::glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texId, 0);
  SDL::GLenum drawTargets[1] = {GL_COLOR_ATTACHMENT0};
  SDL::glDrawBuffers(1, drawTargets);

  auto ret = SDL::glCheckFramebufferStatus(GL_FRAMEBUFFER);
  CHECK(ret == GL_FRAMEBUFFER_COMPLETE);
  unbind();
}

void Framebuffer::bind() {
  SDL::glBindFramebuffer(GL_FRAMEBUFFER, id);
}

void Framebuffer::unbind() {
  SDL::glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

Framebuffer::~Framebuffer() {
  if (id)
    SDL::glDeleteFramebuffers(1, &id);
  if (texId)
    SDL::glDeleteTextures(1, &texId);
}
