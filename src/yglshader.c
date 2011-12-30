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


extern float vdp1wratio;
extern float vdp1hratio;

static void Ygl_printShaderError( GLuint shader )
{
  GLsizei bufSize;
  
  pfglGetShaderiv(shader, GL_INFO_LOG_LENGTH , &bufSize);
  
  if (bufSize > 1) {
    GLchar *infoLog;
    
    infoLog = (GLchar *)malloc(bufSize);
    if (infoLog != NULL) {
      GLsizei length;
      pfglGetShaderInfoLog(shader, bufSize, &length, infoLog);
      printf("Shaderlog:\n%s\n", infoLog);
      free(infoLog);
    }
  }   
}

static GLuint _prgid[PG_MAX] ={0};

const GLchar Yglprg_vdp1_gouraudshading_v[] = \
"attribute vec4 grcolor;\n" \
"varying vec4 vtxcolor;\n" \
"void main() {\n" \
" vtxcolor=grcolor;\n" \
" gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;\n" \
" gl_Position = ftransform();\n" \
"}\n";
const GLchar * pYglprg_vdp1_gouraudshading_v[] = {Yglprg_vdp1_gouraudshading_v, NULL};

const GLchar Yglprg_vdp1_gouraudshading_f[] = \
"uniform sampler2D sprite;\n" \
"varying vec4 vtxcolor;\n" \
"void main() {\n" \
"  vec2 addr = gl_TexCoord[0].st;\n" \
"  addr.s = addr.s / (gl_TexCoord[0].q);\n" \
"  addr.t = addr.t / (gl_TexCoord[0].q);\n" \
"  vec4 spriteColor = texture2D(sprite,addr);\n" \
"  gl_FragColor = spriteColor; \n" \
"  gl_FragColor += vec4(vtxcolor.r,vtxcolor.g,vtxcolor.b,0.0);\n" \
"  gl_FragColor.a = spriteColor.a * vtxcolor.a;\n" \
"}\n";

const GLchar * pYglprg_vdp1_gouraudshading_f[] = {Yglprg_vdp1_gouraudshading_f, NULL};
 
int Ygl_uniformGlowShading(void * p )
{
   YglProgram * prg;
   prg = p;
   if( prg->vertexAttribute != NULL )
   {
      pfglEnableVertexAttribArray(prg->vaid);
      pfglVertexAttribPointer(prg->vaid,4, GL_FLOAT, GL_FALSE, 0, prg->vertexAttribute);
   }
   return 0;
}

int Ygl_cleanupGlowShading(void * p )
{
   YglProgram * prg;
   prg = p;
   pfglDisableVertexAttribArray(prg->vaid);
   return 0;
}

int Ygl_uniformStartUserClip(void * p )
{
   YglProgram * prg;
   prg = p;
   
   if( prg->ux1 != -1 )
   {
      GLint vertices[12];
      glColorMask( GL_FALSE,GL_FALSE,GL_FALSE,GL_FALSE );
      glStencilMask(0xffffffff);
      glClearStencil(0);
      glClear(GL_STENCIL_BUFFER_BIT);
      glEnable(GL_STENCIL_TEST);
      glStencilFunc(GL_ALWAYS,0x1,0x01);
      glStencilOp(GL_REPLACE,GL_REPLACE,GL_REPLACE);
      glDisable(GL_TEXTURE_2D);
      glColor4f(0.0f,0.0f,0.0f,1.0f);      
      
      // render
      vertices[0] = (int)((float)prg->ux1 * vdp1wratio);
      vertices[1] = (int)((float)prg->uy1 * vdp1hratio);
      vertices[2] = (int)((float)(prg->ux2+1) * vdp1wratio);
      vertices[3] = (int)((float)prg->uy1 * vdp1hratio);
      vertices[4] = (int)((float)(prg->ux2+1) * vdp1wratio);
      vertices[5] = (int)((float)(prg->uy2+1) * vdp1hratio);

      vertices[6] = (int)((float)prg->ux1 * vdp1wratio);
      vertices[7] = (int)((float)prg->uy1 * vdp1hratio);
      vertices[8] = (int)((float)(prg->ux2+1) * vdp1wratio);
      vertices[9] = (int)((float)(prg->uy2+1) * vdp1hratio);
      vertices[10] = (int)((float)prg->ux1 * vdp1wratio);
      vertices[11] = (int)((float)(prg->uy2+1) * vdp1hratio);  
      
      glVertexPointer(2, GL_INT, 0, vertices);
      glDrawArrays(GL_TRIANGLES, 0, 6);
      
      glColorMask( GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE );
      glStencilFunc(GL_ALWAYS,0,0x0);
      glStencilOp(GL_KEEP,GL_KEEP,GL_KEEP);
      glDisable(GL_STENCIL_TEST);
      glEnable(GL_TEXTURE_2D);
      glColor4f(1.0f,1.0f,1.0f,1.0f);
   }
   
   glEnable(GL_STENCIL_TEST);
   glStencilOp(GL_KEEP,GL_KEEP,GL_KEEP);
   if( prg->uClipMode == 0x02 )
   {
      glStencilFunc(GL_EQUAL,0x1,0xFF);
   }else if( prg->uClipMode == 0x03 )
   {
      glStencilFunc(GL_EQUAL,0x0,0xFF);      
   }else{
      glStencilFunc(GL_ALWAYS,0,0xFF);
   }
   
   return 0;
}

int Ygl_cleanupStartUserClip(void * p ){return 0;}

int Ygl_uniformEndUserClip(void * p )
{
   YglProgram * prg;
   prg = p;
   glDisable(GL_STENCIL_TEST);
   glStencilFunc(GL_ALWAYS,0,0xFF);
   return 0;
}

int Ygl_cleanupEndUserClip(void * p ){return 0;}


int Ygl_uniformAddBlend(void * p )
{
   glBlendFunc(GL_ONE,GL_ONE);
   return 0;
}

int Ygl_cleanupAddBlend(void * p )
{
   glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
   return 0;
}


int YglGetProgramId( int prg ) 
{
   return _prgid[prg];
}

int YglProgramInit()
{
   GLchar shaderInfoLog[256];
   GLint compiled,linked;
   
   // 
   _prgid[PG_NORMAL] = 0;
   
   // 
   _prgid[PG_VFP1_GOURAUDSAHDING] = pfglCreateProgram();
    if (_prgid[PG_VFP1_GOURAUDSAHDING])
    {
      GLuint vshader = pfglCreateShader(GL_VERTEX_SHADER);
      GLuint fshader = pfglCreateShader(GL_FRAGMENT_SHADER);
      GLuint id;

      pfglShaderSource(vshader, 1, pYglprg_vdp1_gouraudshading_v, NULL);
      pfglCompileShader(vshader);
      pfglGetShaderiv(vshader, GL_COMPILE_STATUS, &compiled);
      if (compiled == GL_FALSE) {
         printf( "Compile error in vertex shader.\n");
         Ygl_printShaderError(vshader);
         _prgid[PG_VFP1_GOURAUDSAHDING] = 0;
         return 0;
      }
  
      pfglShaderSource(fshader, 1, pYglprg_vdp1_gouraudshading_f, NULL);
      pfglCompileShader(fshader);
      pfglGetShaderiv(fshader, GL_COMPILE_STATUS, &compiled);
      if (compiled == GL_FALSE) {
         printf( "Compile error in fragment shader.\n");
         Ygl_printShaderError(fshader);         
         _prgid[PG_VFP1_GOURAUDSAHDING] = 0;
         return 0;
      }
      
      pfglAttachShader(_prgid[PG_VFP1_GOURAUDSAHDING], vshader);
      pfglAttachShader(_prgid[PG_VFP1_GOURAUDSAHDING], fshader);
      pfglLinkProgram(_prgid[PG_VFP1_GOURAUDSAHDING]);
      pfglGetProgramiv(_prgid[PG_VFP1_GOURAUDSAHDING], GL_LINK_STATUS, &linked);
      if (linked == GL_FALSE) {
         printf("Link error..\n");
         Ygl_printShaderError(_prgid[PG_VFP1_GOURAUDSAHDING]);
         _prgid[PG_VFP1_GOURAUDSAHDING] = 0;
         return 0;
      }

   }
   
   _prgid[PG_VFP1_STARTUSERCLIP] = 0;
   _prgid[PG_VFP1_ENDUSERCLIP] = 0;   
  
   _prgid[PG_VDP2_ADDBLEND] = 0;
   
   return 0;
}



int YglProgramChange( YglLevel * level, int prgid )
{
   YglProgram* tmp;
   level->prgcurrent++;

   if( level->prgcurrent >= level->prgcount)
   {
      level->prgcount++;
      tmp = (YglProgram*)malloc(sizeof(YglProgram)*level->prgcount);
      if( tmp == NULL ) return -1;
      memset(tmp,0,sizeof(YglProgram)*level->prgcount);
      memcpy(tmp,level->prg,sizeof(YglProgram)*(level->prgcount-1));
      level->prg = tmp;
      
      level->prg[level->prgcurrent].currentQuad = 0;
      level->prg[level->prgcurrent].maxQuad = 12 * 64;
      if ((level->prg[level->prgcurrent].quads = (int *) malloc(level->prg[level->prgcurrent].maxQuad * sizeof(int))) == NULL)
         return -1;

      if ((level->prg[level->prgcurrent].textcoords = (float *) malloc(level->prg[level->prgcurrent].maxQuad * sizeof(float) * 2)) == NULL)
         return -1;

       if ((level->prg[level->prgcurrent].vertexAttribute = (float *) malloc(level->prg[level->prgcurrent].maxQuad * sizeof(float)*2)) == NULL)
         return -1;
   }

   level->prg[level->prgcurrent].prgid=prgid;
   level->prg[level->prgcurrent].prg=_prgid[prgid];
   
   if( prgid == PG_VFP1_GOURAUDSAHDING )
   {
      GLuint id;
      level->prg[level->prgcurrent].setupUniform = Ygl_uniformGlowShading;
	  level->prg[level->prgcurrent].cleanupUniform = Ygl_cleanupGlowShading;
      id = pfglGetUniformLocation(_prgid[PG_VFP1_GOURAUDSAHDING], (const GLchar *)"sprite");
      pfglUniform1i(id, 0);
      level->prg[level->prgcurrent].vaid = 0;
      level->prg[level->prgcurrent].vaid = pfglGetAttribLocation(_prgid[PG_VFP1_GOURAUDSAHDING],(const GLchar *)"grcolor");
   }  
   else if( prgid == PG_VFP1_STARTUSERCLIP )
   {
      level->prg[level->prgcurrent].setupUniform = Ygl_uniformStartUserClip;
      level->prg[level->prgcurrent].cleanupUniform = Ygl_cleanupStartUserClip;
   }
   else if( prgid == PG_VFP1_ENDUSERCLIP )
   {
      level->prg[level->prgcurrent].setupUniform = Ygl_uniformEndUserClip;
      level->prg[level->prgcurrent].cleanupUniform = Ygl_cleanupEndUserClip;
   }
   else if( prgid == PG_VDP2_ADDBLEND )
   {
      level->prg[level->prgcurrent].setupUniform = Ygl_uniformAddBlend;
      level->prg[level->prgcurrent].cleanupUniform = Ygl_cleanupAddBlend;
   }
   else{
      level->prg[level->prgcurrent].setupUniform = NULL;
   }
   return 0;
   
}

#endif

