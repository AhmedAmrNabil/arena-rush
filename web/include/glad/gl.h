#pragma once

#include <GLES3/gl3.h>
#include <GLES2/gl2ext.h>

using GLADloadproc = void* (*)(const char* name);

static inline int gladLoadGL(GLADloadproc) {
    return 1;
}

#define glClearDepth glClearDepthf
#define glCreateVertexArrays glGenVertexArrays

#ifndef GL_CLAMP_TO_BORDER
#define GL_CLAMP_TO_BORDER GL_CLAMP_TO_EDGE
#endif

#ifndef GL_MIRROR_CLAMP_TO_EDGE
#define GL_MIRROR_CLAMP_TO_EDGE GL_CLAMP_TO_EDGE
#endif

#ifndef GL_TEXTURE_BORDER_COLOR
#define GL_TEXTURE_BORDER_COLOR 0x1004
#endif

#ifndef GL_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_TEXTURE_MAX_ANISOTROPY_EXT 0x84FE
#endif

#ifndef GL_POINT
#define GL_POINT 0x1B00
#endif

#ifndef GL_LINE
#define GL_LINE 0x1B01
#endif

#ifndef GL_FILL
#define GL_FILL 0x1B02
#endif
