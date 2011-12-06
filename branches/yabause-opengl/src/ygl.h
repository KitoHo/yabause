/*  Copyright 2005 Guillaume Duhamel
    Copyright 2005-2006 Theo Berkau

    This file is part of Yabause.

    Yabause is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Yabause is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Yabause; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/

#ifdef HAVE_LIBGL
#ifdef _MSC_VER
#include <windows.h>
#include <GL/gl.h>
#include "windows/glext.h"
#endif

#ifdef HAVE_LIBSDL
 #ifdef __APPLE__
  #include <SDL/SDL.h>
 #else
  #include "SDL.h"
 #endif
#endif
#ifndef _arch_dreamcast
#if HAVE_LIBGLUT
    #ifdef __APPLE__
        #include <GLUT/glut.h>
    #else
        #include <GL/glut.h>
    #endif
#else
    #ifdef __APPLE__
        #include <OpenGL/gl.h>
    #else
        #include <GL/gl.h>
    #endif
#endif
#endif
#include <stdarg.h>
#include <string.h>

#ifndef YGL_H
#define YGL_H

#include "core.h"


typedef struct {
	int vertices[8];
	unsigned int w;
	unsigned int h;
	int flip;
	int priority;
	int dst;
} YglSprite;

typedef struct {
	float x;
	float y;
} YglCache;

typedef struct {
	unsigned int * textdata;
	unsigned int w;
} YglTexture;


typedef struct {
	unsigned int currentX;
	unsigned int currentY;
	unsigned int yMax;
	unsigned int * texture;
	unsigned int width;
	unsigned int height;
} YglTextureManager;

extern YglTextureManager * YglTM;

void YglTMInit(unsigned int, unsigned int);
void YglTMDeInit(void);
void YglTMReset(void);
void YglTMAllocate(YglTexture *, unsigned int, unsigned int, unsigned int *, unsigned int *);

enum
{
	PG_NORMAL=0,
	PG_VFP1_GOURAUDSAHDING,
	PG_MAX,
};
typedef struct {
	GLuint prg;
	int * quads;
	float * textcoords;
	float * vertexAttribute;
	int currentQuad;
	int maxQuad;
	int vaid;
	int (*setupUniform)(void *);
	int (*cleanupUniform)(void *);
} YglProgram;

typedef struct {
	int prgcount;
	int prgcurrent;
	YglProgram * prg;
} YglLevel;

typedef struct {
	GLuint texture;
	int st;
	char message[512];
	int msglength;
	unsigned int width;
	unsigned int height;
	unsigned int depth;
	YglLevel * levels;
}  Ygl;

extern Ygl * _Ygl;

int YglGLInit(int, int);
int YglScreenInit(int r, int g, int b, int d);
int YglInit(int, int, unsigned int);
void YglDeInit(void);
float * YglQuad(YglSprite *, YglTexture *,YglCache * c);
void YglCachedQuad(YglSprite *, YglCache *);
void YglRender(void);
void YglReset(void);
void YglShowTexture(void);
void YglChangeResolution(int, int);
void YglOnScreenDebugMessage(char *, ...);
void YglCacheQuadGrowShading(YglSprite * input, float * colors, YglCache * cache);
int YglQuadGrowShading(YglSprite * input, YglTexture * output, float * colors,YglCache * c);

int YglIsCached(u32,YglCache *);
void YglCacheAdd(u32,YglCache *);
void YglCacheReset(void);


#if 1  // Does anything need this?  It breaks a bunch of prototypes if
       // GLchar is typedef'd instead of #define'd  --AC
#ifndef GLchar
#define GLchar GLbyte
#endif
#endif  // 0

extern GLuint (STDCALL *pfglCreateProgram)(void);
extern GLuint (STDCALL *pfglCreateShader)(GLenum);
extern void (STDCALL *pfglShaderSource)(GLuint,GLsizei,const GLchar **,const GLint *);
extern void (STDCALL *pfglCompileShader)(GLuint);
extern void (STDCALL *pfglAttachShader)(GLuint,GLuint);
extern void (STDCALL *pfglLinkProgram)(GLuint);
extern void (STDCALL *pfglUseProgram)(GLuint);
extern GLint (STDCALL *pfglGetUniformLocation)(GLuint,const GLchar *);
extern void (STDCALL *pfglUniform1i)(GLint,GLint);
extern void (STDCALL *pfglGetShaderInfoLog)(GLuint,GLsizei,GLsizei *,GLchar *);
extern void (STDCALL *pfglVertexAttribPointer)(GLuint index,GLint size, GLenum type, GLboolean normalized, GLsizei stride,const void *pointer);
extern void (STDCALL *pfglBindAttribLocation)( GLuint program, GLuint index, const GLchar * name);
extern void (STDCALL *pfglGetProgramiv)( GLuint    program, GLenum pname, GLint * params);
extern void (STDCALL *pfglGetShaderiv)(GLuint shader,GLenum pname,GLint *    params);
extern GLint (STDCALL *pfglGetAttribLocation)(GLuint program,const GLchar *    name);

extern void (STDCALL *pfglEnableVertexAttribArray)(GLuint index);
extern void (STDCALL *pfglDisableVertexAttribArray)(GLuint index);
#endif
#endif
