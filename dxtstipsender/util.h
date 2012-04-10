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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <vector>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>

#if !defined(WIN32)
#include <unistd.h>
#include <sys/time.h>
#else
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#pragma warning(disable:4786)   // symbol size limitation ... STL
#endif


#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#include <stdarg.h>
//#include <values.h>


void   aInitialize();

double aTime();

void   aLog(char* format,...);
void aError(char* format,...);

void* aAlloc(size_t const n);
void aFree(void* const p);

#if defined(WIN32)
float drand48(void);
#endif

#if defined(__APPLE__)
#define memalign(x,y) malloc((y))
#else
#include <malloc.h>
#endif


#if defined(WIN32)
void *aligned_malloc(size_t size, size_t align_size);
#define memalign(x,y) aligned_malloc(y, x)
void aligned_free(void *ptr);
#define memfree(x) aligned_free(x)
#else
#define memfree(x) free(x)
#endif


// From NVIDIA Toolkit

#ifndef DATA_PATH_H
#define DATA_PATH_H

class data_path
{
public:
  std::string              file_path;
  std::string              path_name;
  std::vector<std::string> path;

  std::string get_path(std::string filename);
  std::string get_file(std::string filename);

  FILE *fopen(std::string filename, const char * mode = "rb");

#ifdef WIN32
  int fstat(std::string filename, struct _stat * stat);
#else
  int fstat(std::string filename, struct stat * stat);
#endif
};

#endif
