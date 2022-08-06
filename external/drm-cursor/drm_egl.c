/*
 *  Copyright (c) 2021, Jeffy Chen <jeffy.chen@rock-chips.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 */

#include <errno.h>
#include <malloc.h>
#include <unistd.h>

#include <xf86drm.h>
#include <xf86drmMode.h>

#include <gbm.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include "drm_common.h"
#include "drm_egl.h"

static const GLfloat texcoords[] = {
  0.0f,  1.0f,
  1.0f,  1.0f,
  0.0f,  0.0f,
  1.0f,  0.0f,
};

static const char vertex_shader_source[] =
"attribute vec4 position;\n"
"attribute vec2 texcoord;\n"
"varying vec2 v_texcoord;\n"
"void main()\n"
"{\n"
"   gl_Position = position;\n"
"   v_texcoord = texcoord;\n"
"}\n";

static const char fragment_shader_source[] =
"#extension GL_OES_EGL_image_external : require\n"
"precision mediump float;\n"
"varying vec2 v_texcoord;\n"
"uniform samplerExternalOES tex;\n"
"void main()\n"
"{\n"
"    gl_FragColor = texture2D(tex, v_texcoord);\n"
"}\n";

/* HACK: use multiple surfaces to avoid AFBC corruption */
#define MAX_NUM_SURFACES 64

typedef struct {
  int fd;

  struct gbm_device *gbm_dev;
  struct gbm_surface *gbm_surfaces[MAX_NUM_SURFACES];

  EGLDisplay egl_display;
  EGLContext egl_context;
  EGLConfig egl_config;
  EGLSurface egl_surfaces[MAX_NUM_SURFACES];
  GLuint vertex_shader, fragment_shader, program;

  int width;
  int height;

  int format;
  uint64_t modifier;

  int current_surface;
  int num_surfaces;
} egl_ctx;

drm_private void egl_free_ctx(void *data)
{
  egl_ctx *ctx = data;
  int i;

  if (ctx->egl_display != EGL_NO_DISPLAY) {
    eglMakeCurrent(ctx->egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE,
                   EGL_NO_CONTEXT);

    if (ctx->program)
      glDeleteProgram(ctx->program);

    if (ctx->fragment_shader)
      glDeleteShader(ctx->fragment_shader);

    if (ctx->vertex_shader)
      glDeleteShader(ctx->vertex_shader);

    for (i = 0; i < ctx->num_surfaces; i++) {
      if (ctx->egl_surfaces[i] != EGL_NO_SURFACE)
        eglDestroySurface(ctx->egl_display, ctx->egl_surfaces[i]);
    }

    if (ctx->egl_context != EGL_NO_CONTEXT)
      eglDestroyContext(ctx->egl_display, ctx->egl_context);

    eglTerminate(ctx->egl_display);
    eglReleaseThread();
  }

  for (i = 0; i < ctx->num_surfaces; i++) {
    if (ctx->gbm_surfaces[i])
      gbm_surface_destroy(ctx->gbm_surfaces[i]);
  }

  if (ctx->gbm_dev)
    gbm_device_destroy(ctx->gbm_dev);

  if (ctx->fd >= 0)
    close(ctx->fd);

  free(ctx);
}

static int egl_flush_surfaces(egl_ctx *ctx)
{
  int i;

  /* Re-create surfaces */

  eglMakeCurrent(ctx->egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE,
                 EGL_NO_CONTEXT);

  for (i = 0; i < ctx->num_surfaces; i++) {
    if (ctx->egl_surfaces[i] != EGL_NO_SURFACE) {
      eglDestroySurface(ctx->egl_display, ctx->egl_surfaces[i]);
      ctx->egl_surfaces[i] = EGL_NO_SURFACE;
    }
  }

  for (i = 0; i < ctx->num_surfaces; i++) {
    if (ctx->gbm_surfaces[i]) {
      gbm_surface_destroy(ctx->gbm_surfaces[i]);
      ctx->gbm_surfaces[i] = NULL;
    }
  }

  for (i = 0; i < ctx->num_surfaces; i++) {
    if (!ctx->modifier)
      ctx->gbm_surfaces[i] =
        gbm_surface_create(ctx->gbm_dev, ctx->width, ctx->height,
                           ctx->format, 0);
    else
      ctx->gbm_surfaces[i] =
        gbm_surface_create_with_modifiers(ctx->gbm_dev,
                                          ctx->width, ctx->height,
                                          ctx->format, &ctx->modifier, 1);
    if (!ctx->gbm_surfaces[i]) {
      DRM_ERROR("failed to create GBM surface\n");
      return -1;
    }

    ctx->egl_surfaces[i] =
      eglCreateWindowSurface(ctx->egl_display, ctx->egl_config,
                             (EGLNativeWindowType)ctx->gbm_surfaces[i], NULL);
    if (ctx->egl_surfaces[i] == EGL_NO_SURFACE) {
      DRM_ERROR("failed to create EGL surface\n");
      return -1;
    }
  }

  ctx->current_surface = 0;
  return 0;
}

drm_private void *egl_init_ctx(int fd, int num_surfaces, int width, int height,
                               int format, uint64_t modifier)
{
  PFNEGLGETPLATFORMDISPLAYEXTPROC get_platform_display;

#define EGL_MAX_CONFIG 64
  EGLConfig configs[EGL_MAX_CONFIG];
  EGLint num_configs;
  egl_ctx *ctx;

  GLint texcoord;
  GLint status;
  const char *source;
  char msg[512];
  int i;

  static const EGLint context_attribs[] = {
    EGL_CONTEXT_CLIENT_VERSION, 2,
    EGL_NONE
  };

  static const EGLint config_attribs[] = {
    EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
    EGL_RED_SIZE, 1,
    EGL_GREEN_SIZE, 1,
    EGL_BLUE_SIZE, 1,
    EGL_ALPHA_SIZE, 0,
    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
    EGL_NONE
  };

  if (num_surfaces > MAX_NUM_SURFACES) {
    DRM_ERROR("too much surfaces: %d > %d\n", num_surfaces, MAX_NUM_SURFACES);
    return NULL;
  }

  get_platform_display = (void *)eglGetProcAddress("eglGetPlatformDisplayEXT");
  if (!get_platform_display) {
    DRM_ERROR("failed to get proc address\n");
    return NULL;
  }

  ctx = calloc(1, sizeof(*ctx));
  if (!ctx) {
    DRM_ERROR("failed to alloc ctx\n");
    return NULL;
  }

  ctx->width = width;
  ctx->height = height;
  ctx->format = format;
  ctx->modifier = modifier;
  ctx->num_surfaces = num_surfaces;

  ctx->fd = dup(fd);
  if (ctx->fd < 0) {
    DRM_ERROR("failed to dup drm fd\n");
    goto err;
  }

  ctx->gbm_dev = gbm_create_device(ctx->fd);
  if (!ctx->gbm_dev) {
    DRM_ERROR("failed to create gbm device\n");
    goto err;
  }

  ctx->egl_display = get_platform_display(EGL_PLATFORM_GBM_KHR,
                                          (void*)ctx->gbm_dev, NULL);
  if (ctx->egl_display == EGL_NO_DISPLAY) {
    DRM_ERROR("failed to get platform display\n");
    goto err;
  }

  if (!eglInitialize(ctx->egl_display, NULL, NULL)) {
    DRM_ERROR("failed to init egl\n");
    goto err;
  }

  if (!eglBindAPI(EGL_OPENGL_ES_API)) {
    DRM_ERROR("failed to bind api\n");
    goto err;
  }

  if (!eglChooseConfig(ctx->egl_display, config_attribs,
                       configs, EGL_MAX_CONFIG, &num_configs) ||
      num_configs < 1) {
    DRM_ERROR("failed to choose config\n");
    goto err;
  }

  for (i = 0; i < num_configs; i++) {
    EGLint value;

    if (!eglGetConfigAttrib(ctx->egl_display, configs[i],
                            EGL_NATIVE_VISUAL_ID, &value))
      continue;

    if (value == format)
      break;
  }

  if (i == num_configs) {
    DRM_ERROR("failed to find EGL config for %4s, force using the first\n",
              (char *)&format);
    ctx->egl_config = configs[0];
  } else {
    ctx->egl_config = configs[i];
  }

  ctx->egl_context = eglCreateContext(ctx->egl_display, ctx->egl_config,
                                      EGL_NO_CONTEXT, context_attribs);
  if (ctx->egl_context == EGL_NO_CONTEXT) {
    DRM_ERROR("failed to create EGL context\n");
    goto err;
  }

  if (egl_flush_surfaces(ctx) < 0) {
    DRM_ERROR("failed to flush surfaces\n");
    goto err;
  }

  eglMakeCurrent(ctx->egl_display, ctx->egl_surfaces[ctx->current_surface],
                 ctx->egl_surfaces[ctx->current_surface],
                 ctx->egl_context);

  source = vertex_shader_source;
  ctx->vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(ctx->vertex_shader, 1, &source, NULL);
  glCompileShader(ctx->vertex_shader);
  glGetShaderiv(ctx->vertex_shader, GL_COMPILE_STATUS, &status);
  if (!status) {
    glGetShaderInfoLog(ctx->vertex_shader, sizeof(msg), NULL, msg);
    DRM_ERROR("failed to compile shader: %s\n", msg);
    goto err;
  }

  source = fragment_shader_source;
  ctx->fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(ctx->fragment_shader, 1, &source, NULL);
  glCompileShader(ctx->fragment_shader);
  glGetShaderiv(ctx->fragment_shader, GL_COMPILE_STATUS, &status);
  if (!status) {
    glGetShaderInfoLog(ctx->fragment_shader, sizeof(msg), NULL, msg);
    DRM_ERROR("failed to compile shader: %s\n", msg);
    goto err;
  }

  ctx->program = glCreateProgram();
  glAttachShader(ctx->program, ctx->vertex_shader);
  glAttachShader(ctx->program, ctx->fragment_shader);
  glLinkProgram(ctx->program);

  glGetProgramiv(ctx->program, GL_LINK_STATUS, &status);
  if (!status) {
    glGetProgramInfoLog(ctx->program, sizeof(msg), NULL, msg);
    DRM_ERROR("failed to link: %s\n", msg);
    goto err;
  }

  glUseProgram(ctx->program);

  texcoord = glGetAttribLocation(ctx->program, "texcoord");
  glVertexAttribPointer(texcoord, 2, GL_FLOAT, GL_FALSE, 0, texcoords);
  glEnableVertexAttribArray(texcoord);

  glUniform1i(glGetUniformLocation(ctx->program, "tex"), 0);

  glViewport(0, 0, width, height);

  return ctx;
err:
  egl_free_ctx(ctx);
  return NULL;
}

static int egl_handle_to_fd(int fd, uint32_t handle)
{
  struct drm_prime_handle args = {
    .fd = -1,
    .handle = handle,
  };
  int ret;

  ret = drmIoctl(fd, DRM_IOCTL_PRIME_HANDLE_TO_FD, &args);
  if (ret < 0) {
    DRM_ERROR("failed to get fd (%d)\n", errno);
    return ret;
  }

  return args.fd;
}

static uint32_t egl_bo_to_fb(int fd, struct gbm_bo* bo, int format,
                             uint64_t modifier)
{
  uint32_t width = gbm_bo_get_width(bo);
  uint32_t height = gbm_bo_get_height(bo);
  uint32_t bpp = gbm_bo_get_bpp(bo) ?: 32;
  uint32_t handles[4] = { 0 };
  uint32_t strides[4] = { 0 };
  uint32_t offsets[4] = { 0 };
  uint64_t modifiers[4] = { 0 };
  uint32_t fb = 0;
  int ret;

  handles[0] = gbm_bo_get_handle(bo).u32;
  strides[0] = gbm_bo_get_stride(bo);
  modifiers[0] = modifier;

  if (!modifier)
    ret = drmModeAddFB(fd, width, height, bpp, bpp,
                       strides[0], handles[0], &fb);
  else
    ret = drmModeAddFB2WithModifiers(fd, width, height, format,
                                     handles, strides,
                                     offsets, modifiers, &fb,
                                     DRM_MODE_FB_MODIFIERS);

  if (ret < 0) {
    DRM_ERROR("failed to add fb (%d)\n", errno);
    return 0;
  }

  return fb;
}

static int egl_attach_dmabuf(egl_ctx *ctx, int dma_fd)
{
  static PFNGLEGLIMAGETARGETTEXTURE2DOESPROC image_target_texture_2d = NULL;
  static PFNEGLCREATEIMAGEKHRPROC create_image = NULL;
  static PFNEGLDESTROYIMAGEKHRPROC destroy_image = NULL;
  EGLImageKHR image;
  int width = ctx->width;
  int height = ctx->height;

  /* Cursor format should be ARGB8888 */
  const EGLint attrs[] = {
    EGL_WIDTH, width,
    EGL_HEIGHT, height,
    EGL_LINUX_DRM_FOURCC_EXT, DRM_FORMAT_ARGB8888,
    EGL_DMA_BUF_PLANE0_FD_EXT, dma_fd,
    EGL_DMA_BUF_PLANE0_OFFSET_EXT, 0,
    EGL_DMA_BUF_PLANE0_PITCH_EXT, width * 4,
    EGL_NONE,
  };

  if (!create_image)
    create_image = (void *) eglGetProcAddress("eglCreateImageKHR");

  if (!destroy_image)
    destroy_image = (void *) eglGetProcAddress("eglDestroyImageKHR");

  if (!image_target_texture_2d)
    image_target_texture_2d =
      (void *) eglGetProcAddress("glEGLImageTargetTexture2DOES");

  if (!create_image || !destroy_image || !image_target_texture_2d) {
    DRM_ERROR("failed to get proc address\n");
    return -1;
  }

  image = create_image(ctx->egl_display, ctx->egl_context,
                       EGL_LINUX_DMA_BUF_EXT, NULL, attrs);
  if (image == EGL_NO_IMAGE) {
    DRM_ERROR("failed to create egl image: 0x%x\n", eglGetError());
    return -1;
  }

  image_target_texture_2d(GL_TEXTURE_EXTERNAL_OES, (GLeglImageOES)image);
  destroy_image(ctx->egl_display, image);
  return 0;
}

drm_private uint32_t egl_convert_fb(void *data, uint32_t handle,
                                    int width, int height, int x, int y)
{
  egl_ctx *ctx = data;
  GLint position;
  GLuint texture;
  struct gbm_bo* bo;
  uint32_t fb = 0;
  int dma_fd;

  GLfloat verts[] = {
    -1.0f, -1.0f,
     1.0f, -1.0f,
    -1.0f,  1.0f,
     1.0f,  1.0f,
  };

  if (width != ctx->width || height != ctx->height) {
    ctx->width = width;
    ctx->height = height;

    if (egl_flush_surfaces(ctx) < 0) {
      DRM_ERROR("failed to flush surfaces\n");
      return 0;
    }
  }

  dma_fd = egl_handle_to_fd(ctx->fd, handle);
  if (dma_fd < 0) {
    DRM_ERROR("failed to get dma fd\n");
    return 0;
  }

  ctx->current_surface = (ctx->current_surface + 1) % ctx->num_surfaces;
  eglMakeCurrent(ctx->egl_display, ctx->egl_surfaces[ctx->current_surface],
                 ctx->egl_surfaces[ctx->current_surface],
                 ctx->egl_context);

  /* Apply offsets */
  for (int i = 0; i < 4; i++) {
    verts[2 * i] += x * 2.0 / ctx->width;
    verts[2 * i + 1] -= y * 2.0 / ctx->height;
  }

  position = glGetAttribLocation(ctx->program, "position");
  glVertexAttribPointer(position, 2, GL_FLOAT, GL_FALSE, 0, verts);
  glEnableVertexAttribArray(position);

  glGenTextures(1, &texture);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_EXTERNAL_OES, texture);

  if (egl_attach_dmabuf(ctx, dma_fd) < 0) {
    DRM_ERROR("failed to attach dmabuf\n");
    goto err_del_texture;
  }

  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  eglSwapBuffers(ctx->egl_display, ctx->egl_surfaces[ctx->current_surface]);

  bo = gbm_surface_lock_front_buffer(ctx->gbm_surfaces[ctx->current_surface]);
  if (!bo) {
    DRM_ERROR("failed to get front bo\n");
    goto err_del_texture;
  }

  fb = egl_bo_to_fb(ctx->fd, bo, ctx->format, ctx->modifier);
  gbm_surface_release_buffer(ctx->gbm_surfaces[ctx->current_surface], bo);

err_del_texture:
  glDeleteTextures(1, &texture);
  close(dma_fd);
  return fb;
}
