/*  Copyright 2005-2006 Guillaume Duhamel
    Copyright 2005-2006 Theo Berkau
    Copyright 2011 Shinya Miyamoto
    
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
#include <stdlib.h>
#include <math.h>
#include "ygl.h"
#include "yui.h"
#include "vidshared.h"

#ifdef WIN32
#include <windows.h>
#include <wingdi.h>
#elif HAVE_GLXGETPROCADDRESS
#include <GL/glx.h>
#endif
YglTextureManager * YglTM;
Ygl * _Ygl;

typedef struct
{
   u32 id;
   YglCache c;
} cache_struct;

static cache_struct *cachelist;
static int cachelistsize=0;

typedef struct
{
   float s, t, r, q;
} texturecoordinate_struct;

#ifdef HAVE_GLXGETPROCADDRESS
void STDCALL * (*yglGetProcAddress)(const char *szProcName) = (void STDCALL *(*)(const char *))glXGetProcAddress;
#elif WIN32
#define yglGetProcAddress wglGetProcAddress
#endif


// extention function pointers
GLuint (STDCALL *pfglCreateProgram)(void);
GLuint (STDCALL *pfglCreateShader)(GLenum);
void (STDCALL *pfglCompileShader)(GLuint);
void (STDCALL *pfglAttachShader)(GLuint,GLuint);
void (STDCALL *pfglLinkProgram)(GLuint);
void (STDCALL *pfglUseProgram)(GLuint);
GLint (STDCALL *pfglGetUniformLocation)(GLuint,const GLchar *);
void (STDCALL *pfglShaderSource)(GLuint,GLsizei,const GLchar **,const GLint *);
void (STDCALL *pfglUniform1i)(GLint,GLint);
void (STDCALL *pfglGetShaderInfoLog)(GLuint,GLsizei,GLsizei *,GLchar *);
void (STDCALL *pfglVertexAttribPointer)(GLuint index,GLint size, GLenum type, GLboolean normalized, GLsizei stride,const void *pointer);
GLint (STDCALL *pfglGetAttribLocation)(GLuint program,const GLchar *    name);
void (STDCALL *pfglBindAttribLocation)( GLuint program, GLuint index, const GLchar * name);
void (STDCALL *pfglGetProgramiv)( GLuint    program, GLenum pname, GLint * params);
void (STDCALL *pfglGetShaderiv)(GLuint shader,GLenum pname,GLint *    params);
void (STDCALL *pfglEnableVertexAttribArray)(GLuint index);
void (STDCALL *pfglDisableVertexAttribArray)(GLuint index);

// extention function dummys
GLuint STDCALL pfglCreateProgramdmy(void){return 0;}
GLuint STDCALL pfglCreateShaderdmy(GLenum a){ return 0;}
void STDCALL pfglCompileShaderdmy(GLuint a){return;}
void STDCALL pfglAttachShaderdmy(GLuint a,GLuint b){return;}
void STDCALL pfglLinkProgramdmy(GLuint a){return;}
void STDCALL pfglUseProgramdmy(GLuint a){return;}
GLint STDCALL pfglGetUniformLocationdmy(GLuint a,const GLchar * b){return 0;}
void STDCALL pfglShaderSourcedmy(GLuint a,GLsizei b,const GLchar **c,const GLint *d){return;}
void STDCALL pfglUniform1idmy(GLint a,GLint b){return;}
void STDCALL pfglVertexAttribPointerdmy(GLuint index,GLint size, GLenum type, GLboolean normalized, GLsizei stride,const void *pointer){return;}
GLint STDCALL pfglGetAttribLocationdmy(GLuint program,const GLchar * name){return 0;}
void STDCALL pfglBindAttribLocationdmy( GLuint program, GLuint index, const GLchar * name){return;}
void STDCALL pfglGetProgramivdmy(GLuint    program, GLenum pname, GLint * params)
{
   if( pname == GL_LINK_STATUS ) *params = GL_FALSE;
   return;
}
GLchar s_msg_no_opengl2[]="Your GPU driver does not support OpenGL 2.0.\nOpenGL Video Interface is running fallback mode.";
void STDCALL pfglGetShaderivdmy(GLuint shader,GLenum pname,GLint *    params)
{
   if( pname == GL_COMPILE_STATUS ) *params = GL_FALSE;
   if( pname == GL_INFO_LOG_LENGTH ) *params = strlen(s_msg_no_opengl2)+1;
   return;
}
void STDCALL pfglGetShaderInfoLogdmy(GLuint a,GLsizei b,GLsizei *c,GLchar *d)
{
   memcpy(d,s_msg_no_opengl2,b);
   *c=b;
   return;
}
void STDCALL pfglEnableVertexAttribArraydmy(GLuint index){return;}
void STDCALL pfglDisableVertexAttribArraydmy(GLuint index){return;}



#define STD_Q2 (1.0f)
#define EPS (1e-10)
#define EQ(a,b) (abs((a)-(b)) < EPS)
#define IS_ZERO(a) ( (a) < EPS && (a) > -EPS)

// AXB = |A||B|sin
INLINE float cross2d( float veca[2], float vecb[2] )
{
   return (veca[0]*vecb[1])-(vecb[0]*veca[1]);
}

/*-----------------------------------------
    b1+--+ a1
     /  / \
    /  /   \
  a2+-+-----+b2
      ans
      
  get intersection point for opssite edge.
--------------------------------------------*/  
int FASTCALL YglIntersectionOppsiteEdge(float * a1, float * a2, float * b1, float * b2, float * out ) 
{
  float veca[2];
  float vecb[2];
  float vecc[2];
  float d1;
  float d2;

  veca[0]=a2[0]-a1[0];
  veca[1]=a2[1]-a1[1];
  vecb[0]=b1[0]-a1[0];
  vecb[1]=b1[1]-a1[1];
  vecc[0]=b2[0]-a1[0];
  vecc[1]=b2[1]-a1[1];
  d1 = cross2d(vecb,vecc);
  if( IS_ZERO(d1) ) return -1;
  d2 = cross2d(vecb,veca);
  
  out[0] = a1[0]+vecc[0]*d2/d1;
  out[1] = a1[1]+vecc[1]*d2/d1;
 
  return 0;
}


int YglCalcTextureQ(
   int   *pnts,
   float *q   
)
{
   float p1[2],p2[2],p3[2],p4[2],o[2];
   float   q1, q3, q4, qw;
   float   x, y;
   float   dx, w;
   float   b;
   float   ww;
   
   // fast calculation for triangle
   if (( pnts[2*0+0] == pnts[2*1+0] ) && ( pnts[2*0+1] == pnts[2*1+1] )) {
      q[0] = 1.0f;
      q[1] = 1.0f;
      q[2] = 1.0f;
      q[3] = 1.0f;
      return 0;
      
   } else if (( pnts[2*1+0] == pnts[2*2+0] ) && ( pnts[2*1+1] == pnts[2*2+1] ))  {
      q[0] = 1.0f;
      q[1] = 1.0f;
      q[2] = 1.0f;
      q[3] = 1.0f;
      return 0;
   } else if (( pnts[2*2+0] == pnts[2*3+0] ) && ( pnts[2*2+1] == pnts[2*3+1] ))  {
      q[0] = 1.0f;
      q[1] = 1.0f;
      q[2] = 1.0f;
      q[3] = 1.0f;
      return 0;
   } else if (( pnts[2*3+0] == pnts[2*0+0] ) && ( pnts[2*3+1] == pnts[2*0+1] )) {
      q[0] = 1.0f;
      q[1] = 1.0f;
      q[2] = 1.0f;
      q[3] = 1.0f;
      return 0;
   }

   p1[0]=pnts[0];
   p1[1]=pnts[1];
   p2[0]=pnts[2];
   p2[1]=pnts[3];
   p3[0]=pnts[4];
   p3[1]=pnts[5];
   p4[0]=pnts[6];
   p4[1]=pnts[7];

   // calcurate Q1
   if( YglIntersectionOppsiteEdge( p3, p1, p2, p4,  o ) == 0 )
   {
      dx = o[0]-p1[0];
      if( !IS_ZERO(dx) )
      {
         w = p3[0]-p2[0];
         if( !IS_ZERO(w) )
          q1 = fabs(dx/w);
         else
          q1 = 0.0f;
      }else{
         w = p3[1] - p2[1];
         if ( !IS_ZERO(w) ) 
         {
            ww = ( o[1] - p1[1] );
            if ( !IS_ZERO(ww) )
               q1 = fabs(ww / w);
            else
               q1 = 0.0f;
         } else {
            q1 = 0.0f;
         }         
      }
   }else{
      q1 = 1.0f;
   }

   /* q2 = 1.0f; */

   // calcurate Q3
   if( YglIntersectionOppsiteEdge( p1, p3, p2,p4,  o ) == 0 )
   {
      dx = o[0]-p3[0];
      if( !IS_ZERO(dx) )
      {
         w = p1[0]-p2[0];
         if( !IS_ZERO(w) )
          q3 = fabs(dx/w);
         else
          q3 = 0.0f;
      }else{
         w = p1[1] - p2[1];
         if ( !IS_ZERO(w) ) 
         {
            ww = ( o[1] - p3[1] );
            if ( !IS_ZERO(ww) )
               q3 = fabs(ww / w);
            else
               q3 = 0.0f;
         } else {
            q3 = 0.0f;
         }         
      }
   }else{
      q3 = 1.0f;
   }

   
   // calcurate Q4
   if( YglIntersectionOppsiteEdge( p3, p1, p4, p2,  o ) == 0 )
   {
      dx = o[0]-p1[0];
      if( !IS_ZERO(dx) )
      {
         w = p3[0]-p4[0];
         if( !IS_ZERO(w) )
          qw = fabs(dx/w);
         else
          qw = 0.0f;
      }else{
         w = p3[1] - p4[1];
         if ( !IS_ZERO(w) ) 
         {
            ww = ( o[1] - p1[1] );
            if ( !IS_ZERO(ww) )
               qw = fabs(ww / w);
            else
               qw = 0.0f;
         } else {
            qw = 0.0f;
         }         
      }
      if ( !IS_ZERO(qw) )
      {
         w   = qw / q1;
      }
      else
      {
         w   = 0.0f;
      }
      if ( IS_ZERO(w) ) {
         q4 = 1.0f;
      } else {
         q4 = 1.0f / w;
      }      
   }else{
      q4 = 1.0f;
   }

   qw = q1;
   if ( qw < 1.0f )   /* q2 = 1.0f */
      qw = 1.0f;
   if ( qw < q3 )
      qw = q3;
   if ( qw < q4 )
      qw = q4;

   if ( 1.0f != qw )
   {
      qw      = 1.0f / qw;

      q[0]   = q1 * qw;
      q[1]   = 1.0f * qw;
      q[2]   = q3 * qw;
      q[3]   = q4 * qw;
   }
   else
   {
      q[0]   = q1;
      q[1]   = 1.0f;
      q[2]   = q3;
      q[3]   = q4;
   }
   return 0;
}



//////////////////////////////////////////////////////////////////////////////

void YglTMInit(unsigned int w, unsigned int h) {
   YglTM = (YglTextureManager *) malloc(sizeof(YglTextureManager));
   YglTM->texture = (unsigned int *) malloc(sizeof(unsigned int) * w * h);
   YglTM->width = w;
   YglTM->height = h;

   YglTMReset();
}

//////////////////////////////////////////////////////////////////////////////

void YglTMDeInit(void) {
   free(YglTM->texture);
   free(YglTM);
}

//////////////////////////////////////////////////////////////////////////////

void YglTMReset(void) {
   YglTM->currentX = 0;
   YglTM->currentY = 0;
   YglTM->yMax = 0;
}

//////////////////////////////////////////////////////////////////////////////

void YglTMAllocate(YglTexture * output, unsigned int w, unsigned int h, unsigned int * x, unsigned int * y) {
   if ((YglTM->height - YglTM->currentY) < h) {
      fprintf(stderr, "can't allocate texture: %dx%d\n", w, h);
      *x = *y = 0;
      output->w = 0;
      output->textdata = YglTM->texture;
      return;
   }

   if ((YglTM->width - YglTM->currentX) >= w) {
      *x = YglTM->currentX;
      *y = YglTM->currentY;
      output->w = YglTM->width - w;
      output->textdata = YglTM->texture + YglTM->currentY * YglTM->width + YglTM->currentX;
      YglTM->currentX += w;
      if ((YglTM->currentY + h) > YglTM->yMax)
         YglTM->yMax = YglTM->currentY + h;
   }
   else {
      YglTM->currentX = 0;
      YglTM->currentY = YglTM->yMax;
      YglTMAllocate(output, w, h, x, y);
   }
}

//////////////////////////////////////////////////////////////////////////////

int YglGLInit(int width, int height) {
   glClear(GL_COLOR_BUFFER_BIT);

   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glOrtho(0, 320, 224, 0, 1, 0);

   glMatrixMode(GL_TEXTURE);
   glLoadIdentity();
   glOrtho(-width, width, -height, height, 1, 0);

   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   glEnableClientState(GL_VERTEX_ARRAY);
   glEnableClientState(GL_TEXTURE_COORD_ARRAY);

   glGenTextures(1, &_Ygl->texture);
   glBindTexture(GL_TEXTURE_2D, _Ygl->texture);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, YglTM->texture);
   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

int YglScreenInit(int r, int g, int b, int d) {
   YuiSetVideoAttribute(RED_SIZE, r);
   YuiSetVideoAttribute(GREEN_SIZE, g);
   YuiSetVideoAttribute(BLUE_SIZE, b);
   YuiSetVideoAttribute(DEPTH_SIZE, d);
   return (YuiSetVideoMode(320, 224, 32, 0) == 0);
}

//////////////////////////////////////////////////////////////////////////////


int YglInit(int width, int height, unsigned int depth) {
   unsigned int i,j;

   VideoInitGlut();

   YglTMInit(width, height);
   
   if ((_Ygl = (Ygl *) malloc(sizeof(Ygl))) == NULL)
      return -1;
   _Ygl->depth = depth;
   if ((_Ygl->levels = (YglLevel *) malloc(sizeof(YglLevel) * depth)) == NULL)
      return -1;
   for(i = 0;i < depth;i++) {
     _Ygl->levels[i].prgcurrent = 0;
     _Ygl->levels[i].uclipcurrent = 0;
     _Ygl->levels[i].prgcount = 1;
     _Ygl->levels[i].prg = (YglProgram*)malloc(sizeof(YglProgram)*_Ygl->levels[i].prgcount);
     memset(  _Ygl->levels[i].prg,0,sizeof(YglProgram)*_Ygl->levels[i].prgcount);
     if( _Ygl->levels[i].prg == NULL ) return -1;
     
     for(j = 0;j < _Ygl->levels[i].prgcount; j++) {
       _Ygl->levels[i].prg[j].prg=0;
      _Ygl->levels[i].prg[j].currentQuad = 0;
      _Ygl->levels[i].prg[j].maxQuad = 8 * 2000;
      if ((_Ygl->levels[i].prg[j].quads = (int *) malloc(_Ygl->levels[i].prg[j].maxQuad * sizeof(int))) == NULL)
         return -1;

      if ((_Ygl->levels[i].prg[j].textcoords = (float *) malloc(_Ygl->levels[i].prg[j].maxQuad * sizeof(float) * 2)) == NULL)
         return -1;

       if ((_Ygl->levels[i].prg[j].vertexAttribute = (float *) malloc(_Ygl->levels[i].prg[j].maxQuad * sizeof(float)*2)) == NULL)
         return -1;
     }
   }

   YuiSetVideoAttribute(DOUBLEBUFFER, 1);

   if (!YglScreenInit(8, 8, 8, 24))
   {
      if (!YglScreenInit(4, 4, 4, 24))
      {
         if (!YglScreenInit(5, 6, 5, 16))
         {
       YuiErrorMsg("Couldn't set GL mode\n");
            return -1;
         }
      }
   }

   YglGLInit(width, height);

   // Set up Extention
   pfglCreateProgram = (GLuint (STDCALL *)(void)) yglGetProcAddress("glCreateProgram");
   if( pfglCreateProgram == NULL )
   {
      YuiErrorMsg(s_msg_no_opengl2);
      pfglCreateProgram = pfglCreateProgramdmy;
   }
   pfglCreateShader = (GLuint (STDCALL *)(GLenum))yglGetProcAddress("glCreateShader");
   if( pfglCreateShader == NULL ) pfglCreateShader = pfglCreateShaderdmy;
   pfglCompileShader = (void (STDCALL *)(GLuint))yglGetProcAddress("glCompileShader");
   if(pfglCompileShader==NULL) pfglCompileShader = pfglCompileShaderdmy;
   pfglAttachShader = (void (STDCALL *)(GLuint,GLuint))yglGetProcAddress("glAttachShader");
   if(pfglAttachShader == NULL) pfglAttachShader = pfglAttachShaderdmy;
   pfglLinkProgram = (void (STDCALL *)(GLuint))yglGetProcAddress("glLinkProgram");
   if(pfglLinkProgram == NULL ) pfglLinkProgram = pfglLinkProgramdmy;
   pfglUseProgram = (void (STDCALL *)(GLuint))yglGetProcAddress("glUseProgram");
   if(pfglUseProgram == NULL ) pfglUseProgram = pfglUseProgramdmy;
   pfglGetUniformLocation = (GLint (STDCALL *)(GLuint,const GLchar *))yglGetProcAddress("glGetUniformLocation");
   if( pfglGetUniformLocation == NULL ) pfglGetUniformLocation = pfglGetUniformLocationdmy;
   pfglShaderSource = (void (STDCALL *)(GLuint,GLsizei,const GLchar **,const GLint *))yglGetProcAddress("glShaderSource");
   if( pfglShaderSource == NULL ) pfglShaderSource = pfglShaderSourcedmy;
   pfglUniform1i = (void (STDCALL *)(GLint,GLint))yglGetProcAddress("glUniform1i");
   if( pfglUniform1i == NULL ) pfglUniform1i = pfglUniform1idmy;
   pfglGetShaderInfoLog = (void (STDCALL *)(GLuint,GLsizei,GLsizei *,GLchar *))yglGetProcAddress("glGetShaderInfoLog");
   if( pfglGetShaderInfoLog == NULL ) pfglGetShaderInfoLog = pfglGetShaderInfoLogdmy;
   pfglVertexAttribPointer = (void (STDCALL *)(GLuint index,GLint size, GLenum type, GLboolean normalized, GLsizei stride,const void *pointer)) yglGetProcAddress("glVertexAttribPointer");
   if( pfglVertexAttribPointer == NULL ) pfglVertexAttribPointer = pfglVertexAttribPointerdmy;
   pfglGetAttribLocation  = (GLint (STDCALL *)(GLuint program,const GLchar *    name)) yglGetProcAddress("glGetAttribLocation");
   if( pfglGetAttribLocation == NULL ) pfglGetAttribLocation = pfglGetAttribLocationdmy;
   pfglBindAttribLocation = (void (STDCALL *)( GLuint program, GLuint index, const GLchar * name))yglGetProcAddress("glBindAttribLocation");
   if( pfglGetAttribLocation == NULL ) pfglGetAttribLocation = pfglGetAttribLocationdmy;
   pfglGetProgramiv = (void (STDCALL *)( GLuint    program, GLenum pname, GLint * params))yglGetProcAddress("glGetProgramiv");
   if( pfglGetProgramiv == NULL ) pfglGetProgramiv = pfglGetProgramivdmy;
   pfglGetShaderiv  = (void (STDCALL *)(GLuint shader,GLenum pname,GLint *    params))yglGetProcAddress("glGetShaderiv");
   if( pfglGetShaderiv == NULL ) pfglGetShaderiv = pfglGetShaderivdmy;
   pfglEnableVertexAttribArray = (void (STDCALL *)(GLuint index))yglGetProcAddress("glEnableVertexAttribArray");
   if( pfglEnableVertexAttribArray == NULL ) pfglEnableVertexAttribArray = pfglEnableVertexAttribArraydmy;
   pfglDisableVertexAttribArray = (void (STDCALL *)(GLuint index))yglGetProcAddress("glDisableVertexAttribArray");
   if( pfglDisableVertexAttribArray == NULL ) pfglDisableVertexAttribArray = pfglDisableVertexAttribArraydmy;
   
      
   if( YglProgramInit() != 0 ) 
      return -1;
   
   _Ygl->st = 0;
   _Ygl->msglength = 0;

   // This is probably wrong, but it'll have to do for now
   if ((cachelist = (cache_struct *)malloc(0x100000 / 8 * sizeof(cache_struct))) == NULL)
      return -1;

   return 0;
}


//////////////////////////////////////////////////////////////////////////////

void YglDeInit(void) {
   unsigned int i,j;

   YglTMDeInit();

   if (_Ygl)
   {
      if (_Ygl->levels)
      {
         for (i = 0; i < _Ygl->depth; i++)
         {
         for (j = 0; j < _Ygl->levels[i].prgcount; j++)
         {
            if (_Ygl->levels[i].prg[j].quads)
            free(_Ygl->levels[i].prg[j].quads);
            if (_Ygl->levels[i].prg[j].textcoords)
            free(_Ygl->levels[i].prg[j].textcoords);
            if (_Ygl->levels[i].prg[j].vertexAttribute)
            free(_Ygl->levels[i].prg[j].vertexAttribute);
         }
         free(_Ygl->levels[i].prg);
         }
         free(_Ygl->levels);
      }

      free(_Ygl);
   }

   if (cachelist)
      free(cachelist);
}

//////////////////////////////////////////////////////////////////////////////

YglProgram * YglGetProgram( YglSprite * input, int prg )
{
   YglLevel   *level;
   YglProgram *program;
   
   if (input->priority > 7) {
      VDP1LOG("sprite with priority %d\n", input->priority);
      return NULL;
   }   
   
   level = &_Ygl->levels[input->priority];
   
   if( input->uclipmode != level->uclipcurrent )
   {
      if( input->uclipmode == 0x02 || input->uclipmode == 0x03 )
      {
         YglProgramChange(level,PG_VFP1_STARTUSERCLIP);
         program = &level->prg[level->prgcurrent];
         program->uClipMode = input->uclipmode;
         if( level->ux1 != Vdp1Regs->userclipX1 || level->uy1 != Vdp1Regs->userclipY1 ||
            level->ux2 != Vdp1Regs->userclipX2 || level->uy2 != Vdp1Regs->userclipY2 ) 
         {
            program->ux1=Vdp1Regs->userclipX1;
            program->uy1=Vdp1Regs->userclipY1;
            program->ux2=Vdp1Regs->userclipX2;
            program->uy2=Vdp1Regs->userclipY2;
            level->ux1=Vdp1Regs->userclipX1;
            level->uy1=Vdp1Regs->userclipY1;
            level->ux2=Vdp1Regs->userclipX2;
            level->uy2=Vdp1Regs->userclipY2;
         }else{
            program->ux1=-1;
            program->uy1=-1;
            program->ux2=-1;
            program->uy2=-1;
         }
      }else{
         YglProgramChange(level,PG_VFP1_ENDUSERCLIP);
         program = &level->prg[level->prgcurrent];
         program->uClipMode = input->uclipmode;
      }
      level->uclipcurrent = input->uclipmode;
   
   }
   
   if( level->prg[level->prgcurrent].prgid != prg ) {
      YglProgramChange(level,prg);
   }
   program = &level->prg[level->prgcurrent];
   
   if (program->currentQuad == program->maxQuad) {
      program->maxQuad += 8*128;
      program->quads = (int *) realloc(program->quads, program->maxQuad * sizeof(int));
      program->textcoords = (float *) realloc(program->textcoords, program->maxQuad * sizeof(float) * 2);
      program->vertexAttribute = (float *) realloc(program->vertexAttribute, program->maxQuad * sizeof(float)*2);          
      YglCacheReset();
   }
   
   return program;
}


//////////////////////////////////////////////////////////////////////////////


float * YglQuad(YglSprite * input, YglTexture * output, YglCache * c) {
   unsigned int x, y;
   YglLevel   *level;
   YglProgram *program;
   texturecoordinate_struct *tmp;
   float q[4];
   int prg = PG_NORMAL;
   

   program = YglGetProgram(input,prg);
   if( program == NULL ) return NULL;
   
   tmp = (texturecoordinate_struct *)(program->textcoords + (program->currentQuad * 2));
   memcpy(program->quads + program->currentQuad, input->vertices, 8 * sizeof(int));

   program->currentQuad += 8;
   YglTMAllocate(output, input->w, input->h, &x, &y);

   tmp[0].r = tmp[1].r = tmp[2].r = tmp[3].r = 0; // these can stay at 0

   if (input->flip & 0x1) {
      tmp[0].s = tmp[3].s = (float)(x + input->w);
      tmp[1].s = tmp[2].s = (float)(x);
   } else {
      tmp[0].s = tmp[3].s = (float)(x);
      tmp[1].s = tmp[2].s = (float)(x + input->w);
   }
   if (input->flip & 0x2) {
      tmp[0].t = tmp[1].t = (float)(y + input->h);
      tmp[2].t = tmp[3].t = (float)(y);
   } else {
      tmp[0].t = tmp[1].t = (float)(y);
      tmp[2].t = tmp[3].t = (float)(y + input->h);
   }

   if( c != NULL )
   {
      switch(input->flip) {
        case 0:
          c->x = *(program->textcoords + ((program->currentQuad - 8) * 2));   // upper left coordinates(0)
          c->y = *(program->textcoords + ((program->currentQuad - 8) * 2)+1); // upper left coordinates(0)   
          break;
        case 1:
          c->x = *(program->textcoords + ((program->currentQuad - 6) * 2));   // upper left coordinates(0)
          c->y = *(program->textcoords + ((program->currentQuad - 6) * 2)+1); // upper left coordinates(0)       
          break;
       case 2:
          c->x = *(program->textcoords + ((program->currentQuad - 2) * 2));   // upper left coordinates(0)
          c->y = *(program->textcoords + ((program->currentQuad - 2) * 2)+1); // upper left coordinates(0)       
          break;
       case 3:
          c->x = *(program->textcoords + ((program->currentQuad - 4) * 2));   // upper left coordinates(0)
          c->y = *(program->textcoords + ((program->currentQuad - 4) * 2)+1); // upper left coordinates(0)       
          break;
      }
   }

   
   if( input->dst == 1 )
   {
      YglCalcTextureQ(input->vertices,q);
      tmp[0].s *= q[0];
      tmp[0].t *= q[0];
      tmp[1].s *= q[1];
      tmp[1].t *= q[1];
      tmp[2].s *= q[2];
      tmp[2].t *= q[2];
      tmp[3].s *= q[3];
      tmp[3].t *= q[3];
      tmp[0].q = q[0]; 
      tmp[1].q = q[1]; 
      tmp[2].q = q[2]; 
      tmp[3].q = q[3]; 
   }else{
      tmp[0].q = 1.0f; 
      tmp[1].q = 1.0f; 
      tmp[2].q = 1.0f; 
      tmp[3].q = 1.0f; 
   }


   return 0;
}

//////////////////////////////////////////////////////////////////////////////

int YglQuadGrowShading(YglSprite * input, YglTexture * output, float * colors,YglCache * c) {
   unsigned int x, y;
   YglLevel   *level;
   YglProgram *program;
   texturecoordinate_struct *tmp;
   float * vtxa;
   float q[4];
   int prg = PG_VFP1_GOURAUDSAHDING;

   program = YglGetProgram(input,prg);
   if( program == NULL ) return NULL;

   // Vertex
   memcpy(program->quads + program->currentQuad, input->vertices, 8 * sizeof(int));
   
   // Color
   vtxa = (program->vertexAttribute + (program->currentQuad * 2));
   memcpy( vtxa, colors, sizeof(float)*4*4);

   // texture
   tmp = (texturecoordinate_struct *)(program->textcoords + (program->currentQuad * 2));
   
   program->currentQuad += 8;

   YglTMAllocate(output, input->w, input->h, &x, &y);

   tmp[0].r = tmp[1].r = tmp[2].r = tmp[3].r = 0; // these can stay at 0

  if (input->flip & 0x1) {
      tmp[0].s = tmp[3].s = (float)(x + input->w);
      tmp[1].s = tmp[2].s = (float)(x);
   } else {
      tmp[0].s = tmp[3].s = (float)(x);
      tmp[1].s = tmp[2].s = (float)(x + input->w);
   }
   if (input->flip & 0x2) {
      tmp[0].t = tmp[1].t = (float)(y + input->h);
      tmp[2].t = tmp[3].t = (float)(y);
   } else {
      tmp[0].t = tmp[1].t = (float)(y);
      tmp[2].t = tmp[3].t = (float)(y + input->h);
   }

   if( c != NULL )
   {
      switch(input->flip) {
      case 0:
         c->x = *(program->textcoords + ((program->currentQuad - 8) * 2));   // upper left coordinates(0)
         c->y = *(program->textcoords + ((program->currentQuad - 8) * 2)+1); // upper left coordinates(0)   
         break;
      case 1:
         c->x = *(program->textcoords + ((program->currentQuad - 6) * 2));   // upper left coordinates(0)
         c->y = *(program->textcoords + ((program->currentQuad - 6) * 2)+1); // upper left coordinates(0)       
         break;
      case 2:
         c->x = *(program->textcoords + ((program->currentQuad - 2) * 2));   // upper left coordinates(0)
         c->y = *(program->textcoords + ((program->currentQuad - 2) * 2)+1); // upper left coordinates(0)       
         break;
      case 3:
         c->x = *(program->textcoords + ((program->currentQuad - 4) * 2));   // upper left coordinates(0)
         c->y = *(program->textcoords + ((program->currentQuad - 4) * 2)+1); // upper left coordinates(0)       
         break;
   }
   }

   if( input->dst == 1 )
   {
      YglCalcTextureQ(input->vertices,q);
      tmp[0].s *= q[0];
      tmp[0].t *= q[0];
      tmp[1].s *= q[1];
      tmp[1].t *= q[1];
      tmp[2].s *= q[2];
      tmp[2].t *= q[2];
      tmp[3].s *= q[3];
      tmp[3].t *= q[3];
      tmp[0].q = q[0]; 
      tmp[1].q = q[1]; 
      tmp[2].q = q[2]; 
      tmp[3].q = q[3]; 
   }else{
      tmp[0].q = 1.0f; 
      tmp[1].q = 1.0f; 
      tmp[2].q = 1.0f; 
      tmp[3].q = 1.0f; 
   }

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void YglCachedQuad(YglSprite * input, YglCache * cache) {
   YglLevel   * level;
   YglProgram * program;
   unsigned int x,y;
   texturecoordinate_struct *tmp;
   float q[4];

   int prg = PG_NORMAL;
  
   program = YglGetProgram(input,prg);
   if( program == NULL ) return NULL;
   
   x = cache->x;
   y = cache->y;

   // Vertex
   memcpy(program->quads + program->currentQuad, input->vertices, 8 * sizeof(int));

   // Color
   tmp = (texturecoordinate_struct *)(program->textcoords + (program->currentQuad * 2));
      
   program->currentQuad += 8;

   tmp[0].r = tmp[1].r = tmp[2].r = tmp[3].r = 0; // these can stay at 0

  if (input->flip & 0x1) {
      tmp[0].s = tmp[3].s = (float)(x + input->w);
      tmp[1].s = tmp[2].s = (float)(x);
   } else {
      tmp[0].s = tmp[3].s = (float)(x);
      tmp[1].s = tmp[2].s = (float)(x + input->w);
   }
   if (input->flip & 0x2) {
      tmp[0].t = tmp[1].t = (float)(y + input->h);
      tmp[2].t = tmp[3].t = (float)(y);
   } else {
      tmp[0].t = tmp[1].t = (float)(y);
      tmp[2].t = tmp[3].t = (float)(y + input->h);
   }

   if( input->dst == 1 )
   {
      YglCalcTextureQ(input->vertices,q);
      tmp[0].s *= q[0];
      tmp[0].t *= q[0];
      tmp[1].s *= q[1];
      tmp[1].t *= q[1];
      tmp[2].s *= q[2];
      tmp[2].t *= q[2];
      tmp[3].s *= q[3];
      tmp[3].t *= q[3];
      tmp[0].q = q[0]; 
      tmp[1].q = q[1]; 
      tmp[2].q = q[2]; 
      tmp[3].q = q[3]; 
   }else{
      tmp[0].q = 1.0f; 
      tmp[1].q = 1.0f; 
      tmp[2].q = 1.0f; 
      tmp[3].q = 1.0f; 
   }
  
}

//////////////////////////////////////////////////////////////////////////////

void YglCacheQuadGrowShading(YglSprite * input, float * colors,YglCache * cache) {
   YglLevel   * level;
   YglProgram * program;
   unsigned int x,y;
   texturecoordinate_struct *tmp;
   float q[4];
   int prg = PG_VFP1_GOURAUDSAHDING;
   int currentpg = 0;
   float * vtxa;

   program = YglGetProgram(input,prg);
   if( program == NULL ) return NULL;
   
   x = cache->x;
   y = cache->y;

   // Vertex
   memcpy(program->quads + program->currentQuad, input->vertices, 8 * sizeof(int));

   // Color 
   vtxa = (program->vertexAttribute + (program->currentQuad * 2));
   memcpy( vtxa, colors, sizeof(float)*4*4);
 
   // Texture 
   tmp = (texturecoordinate_struct *)(program->textcoords + (program->currentQuad * 2));
 
   program->currentQuad += 8;

   tmp[0].r = tmp[1].r = tmp[2].r = tmp[3].r = 0; // these can stay at 0
   
  if (input->flip & 0x1) {
      tmp[0].s = tmp[3].s = (float)(x + input->w);
      tmp[1].s = tmp[2].s = (float)(x);
   } else {
      tmp[0].s = tmp[3].s = (float)(x);
      tmp[1].s = tmp[2].s = (float)(x + input->w);
   }
   if (input->flip & 0x2) {
      tmp[0].t = tmp[1].t = (float)(y + input->h);
      tmp[2].t = tmp[3].t = (float)(y);
   } else {
      tmp[0].t = tmp[1].t = (float)(y);
      tmp[2].t = tmp[3].t = (float)(y + input->h);
   }

   if( input->dst == 1 )
   {
      YglCalcTextureQ(input->vertices,q);
      tmp[0].s *= q[0];
      tmp[0].t *= q[0];
      tmp[1].s *= q[1];
      tmp[1].t *= q[1];
      tmp[2].s *= q[2];
      tmp[2].t *= q[2];
      tmp[3].s *= q[3];
      tmp[3].t *= q[3];
      tmp[0].q = q[0]; 
      tmp[1].q = q[1]; 
      tmp[2].q = q[2]; 
      tmp[3].q = q[3]; 
   }else{
      tmp[0].q = 1.0f; 
      tmp[1].q = 1.0f; 
      tmp[2].q = 1.0f; 
      tmp[3].q = 1.0f; 
   }
}

//////////////////////////////////////////////////////////////////////////////

void YglRender(void) {
   YglLevel * level;
   GLuint cprg=0;

   glEnable(GL_TEXTURE_2D);
   glShadeModel(GL_SMOOTH);

   glBindTexture(GL_TEXTURE_2D, _Ygl->texture);

   glTexSubImage2D (GL_TEXTURE_2D, 0, 0, 0, YglTM->width, YglTM->yMax, GL_RGBA, GL_UNSIGNED_BYTE, YglTM->texture);

   cprg = PG_NORMAL;
   pfglUseProgram(0);

   if(_Ygl->st) {
      int vertices [] = { 0, 0, 320, 0, 320, 224, 0, 224 };
      int text [] = { 0, 0, YglTM->width, 0, YglTM->width, YglTM->height, 0, YglTM->height };
      glVertexPointer(2, GL_INT, 0, vertices);
      glTexCoordPointer(4, GL_INT, 0, text);
      glDrawArrays(GL_QUADS, 0, 4);
   } else {
      unsigned int i,j;
      for(i = 0;i < _Ygl->depth;i++) 
      {
         level = _Ygl->levels + i;
         glDisable(GL_STENCIL_TEST);
         for( j=0;j<(level->prgcurrent+1); j++ )
         {
            if( level->prg[j].prgid != cprg )
            {
               cprg = level->prg[j].prgid;
               pfglUseProgram(level->prg[j].prg);
            }
            glVertexPointer(2, GL_INT, 0, level->prg[j].quads);
            glTexCoordPointer(4, GL_FLOAT, 0, level->prg[j].textcoords);
            if(level->prg[j].setupUniform) 
			{
				level->prg[j].setupUniform((void*)&level->prg[j]);
			}
			if( level->prg[j].currentQuad != 0 )
            {
               glDrawArrays(GL_QUADS, 0, level->prg[j].currentQuad/2);
            }
			if( level->prg[j].cleanupUniform )
			{
				level->prg[j].cleanupUniform((void*)&level->prg[j]);
			}
         }
         level->prgcurrent = 0;
      }
   }


   glDisable(GL_TEXTURE_2D);
#ifndef _arch_dreamcast
#if HAVE_LIBGLUT
   if (_Ygl->msglength > 0) {
      int i;
      glColor3f(1.0f, 0.0f, 0.0f);
      glRasterPos2i(10, 22);
      for (i = 0; i < _Ygl->msglength; i++) {
         glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, _Ygl->message[i]);
      }
      glColor3f(1, 1, 1);
   }
#endif
#endif

   YuiSwapBuffers();
}

//////////////////////////////////////////////////////////////////////////////

void YglReset(void) {
   YglLevel * level;
   unsigned int i,j;

   glClear(GL_COLOR_BUFFER_BIT);

   YglTMReset();

   for(i = 0;i < _Ygl->depth;i++) {
      level = _Ygl->levels + i;
     level->prgcurrent = 0;
     level->uclipcurrent = 0;
     level->ux1 = 0;
     level->uy1 = 0;
     level->ux2 = 0;
     level->uy2 = 0;     
     for( j=0; j< level->prgcount; j++ )
     {
      level->prg[j].currentQuad = 0;
     }
   }
   _Ygl->msglength = 0;
}

//////////////////////////////////////////////////////////////////////////////

void YglShowTexture(void) {
   _Ygl->st = !_Ygl->st;
}

//////////////////////////////////////////////////////////////////////////////

void YglChangeResolution(int w, int h) {
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glOrtho(0, w, h, 0, 1, 0);
}

//////////////////////////////////////////////////////////////////////////////

void YglOnScreenDebugMessage(char *string, ...) {
   va_list arglist;

   va_start(arglist, string);
   vsprintf(_Ygl->message, string, arglist);
   va_end(arglist);
   _Ygl->msglength = (int)strlen(_Ygl->message);
}

//////////////////////////////////////////////////////////////////////////////

int YglIsCached(u32 addr, YglCache * c ) {
   int i = 0;

   for (i = 0; i < cachelistsize; i++)
   {
      if (addr == cachelist[i].id)
     {
         c->x=cachelist[i].c.x;
       c->y=cachelist[i].c.y;
       return 1;
      }
   }

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void YglCacheAdd(u32 addr, YglCache * c) {
   cachelist[cachelistsize].id = addr;
   cachelist[cachelistsize].c.x = c->x;
   cachelist[cachelistsize].c.y = c->y;
   cachelistsize++;
}

//////////////////////////////////////////////////////////////////////////////

void YglCacheReset(void) {
   cachelistsize = 0;
}

//////////////////////////////////////////////////////////////////////////////



#endif

