/******************************************************************************
 * Fast DXT - a realtime DXT compression tool
 *
 * Author : Luc Renambot
 *
 * Copyright (C) 2007 Electronic Visualization Laboratory,
 * University of Illinois at Chicago
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following disclaimer
 *    in the documentation and/or other materials provided with the distribution.
 *  * Neither the name of the University of Illinois at Chicago nor
 *    the names of its contributors may be used to endorse or promote
 *    products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Direct questions, comments etc about SAGE to http://www.evl.uic.edu/cavern/forum/
 *
 *****************************************************************************/

#include "libdxt.h"

#if defined(__APPLE__)
#define memalign(x,y) malloc((y))
#else
#include <malloc.h>
#endif

#include <pthread.h>

typedef struct _work_t {
	int width, height;
	int nbb;
	byte *in, *out;
} work_t;



void *slave1(void *arg)
{
	work_t *param = (work_t*) arg;
	int nbbytes = 0;
	CompressImageDXT1( param->in, param->out, param->width, param->height, nbbytes);
	param->nbb = nbbytes;
	return NULL;
}

void *slave5(void *arg)
{
	work_t *param = (work_t*) arg;
	int nbbytes = 0;
	CompressImageDXT5( param->in, param->out, param->width, param->height, nbbytes);
	param->nbb = nbbytes;
	return NULL;
}

void *slave5ycocg(void *arg)
{
	work_t *param = (work_t*) arg;
	int nbbytes = 0;
	CompressImageDXT5YCoCg( param->in, param->out, param->width, param->height, nbbytes);
	param->nbb = nbbytes;
	return NULL;
}

int CompressDXT(const byte *in, byte *out, int width, int height, int format, int numthreads)
{
  pthread_t *pid;
  work_t    *job;
  int        nbbytes;

  if ( (numthreads!=1) && (numthreads!=2) && (numthreads!=4) ) {
	fprintf(stderr, "DXT> Errror, number of thread should be 1, 2 or 4\n");
	return 0;
  }

  job = new work_t[numthreads];
  pid = new pthread_t[numthreads];

  for (int k=0;k<numthreads;k++)
    {
      job[k].width = width;
      job[k].height = height/numthreads;
      job[k].nbb = 0;
      job[k].in =  (byte*)in  + (k*width*4*height/numthreads);
      if (format == FORMAT_DXT1)
	job[k].out = out + (k*width*4*height/(numthreads*8));
      else
	job[k].out = out + (k*width*4*height/(numthreads*4));

      switch (format) {
      case FORMAT_DXT1:
	pthread_create(&pid[k], NULL, slave1, &job[k]);
	break;
      case FORMAT_DXT5:
	pthread_create(&pid[k], NULL, slave5, &job[k]);
	break;
      case FORMAT_DXT5YCOCG:
	pthread_create(&pid[k], NULL, slave5ycocg, &job[k]);
	break;
      }
    }

  // Join all the threads
  nbbytes = 0;
  for (int k=0;k<numthreads;k++)
  {
    pthread_join(pid[k], NULL);
    nbbytes += job[k].nbb;
  }
  return nbbytes;
}
