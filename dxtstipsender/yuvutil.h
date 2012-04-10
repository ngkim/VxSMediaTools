#ifndef YUVUTIL_H
#define YUVUTIL_H

#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "libdxt.h"
extern "C" {
#include <libswscale/swscale.h>
}

typedef struct {
	byte r,g,b,a;
} BITMAP4;



void	Initialise422Converter();
void	Convert422toRGBA(void*, byte**, int, int);
void	Convert422toFlippedRGBA(void*, byte**, int, int);
void	LUT_Convert422toFlippedRGBA(void*, byte**, int, int);
void	SWS_Convert422toFlippedRGBA(void*, byte**, int, int);
BITMAP4	UV_to_Bitmap(int, int, int);


#endif /* YUVUTIL_H */

