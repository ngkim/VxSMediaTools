#include "yuvutil.h"

void
SWS_Convert422toFlippedRGBA(void *PixelFrame, byte **rgba, int width, int height)
{
    PixelFormat in_format = PIX_FMT_UYVY422;
    PixelFormat out_format = PIX_FMT_BGR32;
    int bytes_per_pixel = 4;
    int flags = SWS_FAST_BILINEAR;
    static SwsContext *swsContext = NULL;
    byte *sws_src[1], *sws_out[1];
    int sws_src_stride[1], sws_out_stride[1];

    if ( ! swsContext )
        swsContext = sws_getContext(width, height, in_format,
                                    width, height, out_format,
                                    flags, NULL, NULL, NULL);
    if ( ! swsContext )
    {
        fprintf(stderr, "Can't allocate sws_getContext\n");
        return;
    }

    sws_src[0] = (byte*)PixelFrame;
    sws_src_stride[0] = width * 2;

    // Also flip the frame vertically for DXT compressor
    sws_out[0] = *rgba + width * 4 * (height - 1);
    sws_out_stride[0] = - width * bytes_per_pixel;

    sws_scale(swsContext, sws_src, sws_src_stride, 0, height, sws_out, sws_out_stride);
}


/* These are some other implementations to play with - they're not actually used.
   However they need some work to correct some discolouration of highlights
*/

BITMAP4 yuvtab[256][256][256];

/* Load up a lookup table for YUV->RGBA conversion */
void
Initialise422Converter()
{
	BITMAP4 rgb1;
	int y, u, v;

	for (y=0;y<256;y++)
	{
		for (u=0;u<256;u++)
		{
			for (v=0;v<256;v++)
			{
				rgb1 = UV_to_Bitmap(y,u,v);
				yuvtab[y][u][v].r = rgb1.r;
				yuvtab[y][u][v].g = rgb1.g;
				yuvtab[y][u][v].b = rgb1.b;
				yuvtab[y][u][v].a = rgb1.a;
			}
		}
	}
	fprintf(stderr, "Initialised YUV->RGBA lookup table\n");
}

BITMAP4
UV_to_Bitmap(int y, int u, int v)
{
	int r, g, b;
	BITMAP4 bm = {0,0,0,0};

	// u and v are +-0.5
	u -= 128;
	v -= 128;

	// Conversion
	bm.r = y + 1.370705 * v;
	bm.g = y - 0.698001 * v - 0.337633 * u;
	bm.b = y + 1.732446 * u;

	// Clamp to 0..1
	if (bm.r < 0) bm.r = 0;
	if (bm.g < 0) bm.g = 0;
	if (bm.b < 0) bm.b = 0;
	if (bm.r > 255) bm.r = 255;
	if (bm.g > 255) bm.g = 255;
	if (bm.b > 255) bm.b = 255;

	//bm.r = r;
	//bm.g = g;
	//bm.b = b;
	bm.a = 0;

	return (bm);
}

void
Convert422toFlippedRGBA(void *PixelFrame, byte **rgba, int width, int height)
{
	int bytesperline = width * 2;
	int samplesperline = bytesperline / 4;
	byte *yuv, *rgbaout;
	//BITMAP4 rgb1, rgb2;
	byte bmr, bmg, bmb, bma=0;
	int y1, y2, u, v, udiff, vdiff;

	rgbaout = *rgba;
	for (int line=height-1;line>=0;line--)
	{
		//printf("line = %d\n", line);
		yuv = (byte*)PixelFrame + line * bytesperline;
		for (int sample=0;sample<samplesperline;sample++)
		{
			u = yuv[0] - 128;
			v = yuv[2] - 128;
			y1 = yuv[1] - 16;
			y2 = yuv[3] - 16;

			//rgb1 = UV_to_Bitmap(yuv[1], yuv[0], yuv[2]);
			// Conversion
/*
			bmr = y1 + 1.370705 * v;
			bmg = y1 - 0.698001 * v - 0.337633 * u;
			bmb = y1 + 1.732446 * u;
*/
			bmr = y1 + 1.5958 * v;
			bmg = y1 - 0.81290 * v - 0.39173 * u;
			bmb = y1 + 2.017 * u;

/*
			// Clamp to 0..1
			if (bmr < 0) bmr = 0;
			if (bmg < 0) bmg = 0;
			if (bmb < 0) bmb = 0;
			if (bmr > 255) bmr = 255;
			if (bmg > 255) bmg = 255;
			if (bmb > 255) bmb = 255;
*/

                        *rgbaout++ = bmr * 220/256;
                        *rgbaout++ = bmg * 220/256;
                        *rgbaout++ = bmb * 220/256;
                        *rgbaout++ = bma;

			//rgb2 = UV_to_Bitmap(yuv[3], yuv[0], yuv[2]);
			// Conversion
/*
			bmr = y2 + 1.370705 * v;
			bmg = y2 - 0.698001 * v - 0.337633 * u;
			bmb = y2 + 1.732446 * u;
*/
			bmr = y2 + 1.5958 * v;
			bmg = y2 - 0.81290 * v - 0.39173 * u;
			bmb = y2 + 2.017 * u;

/*
			// Clamp to 0..1
			if (bmr < 0) bmr = 0;
			if (bmg < 0) bmg = 0;
			if (bmb < 0) bmb = 0;
			if (bmr > 255) bmr = 255;
			if (bmg > 255) bmg = 255;
			if (bmb > 255) bmb = 255;
*/

                        *rgbaout++ = bmr * 220/256;
                        *rgbaout++ = bmg * 220/256;
                        *rgbaout++ = bmb * 220/256;
                        *rgbaout++ = bma;


			yuv += 4;
		}
	}
}

void
LUT_Convert422toFlippedRGBA(void *PixelFrame, byte **rgba, int width, int height)
{
	int bytesperline = width * 2;
	int samplesperline = bytesperline / 4;
	byte *yuv, *rgbaout;
	BITMAP4 rgb1, rgb2;
	int y1, y2, u, v;

	rgbaout = *rgba;
	for (int line=height-1;line>=0;line--)
	{
		//printf("line = %d\n", line);
		yuv = (byte*)PixelFrame + line * bytesperline;
		for (int sample=0;sample<samplesperline;sample++)
		{
			//printf("sample = %d\n", sample);
			//u  = yuv[0];
			//y1 = yuv[1];
			//v  = yuv[2];
			//y2 = yuv[3];
			//printf("%d %d %d %d\n", y1, u, y2, v);

			rgb1 = yuvtab[yuv[1]][yuv[0]][yuv[2]];
			rgb2 = yuvtab[yuv[3]][yuv[0]][yuv[2]];
                        *rgbaout++ = rgb1.r;
                        *rgbaout++ = rgb1.g;
                        *rgbaout++ = rgb1.b;
                        *rgbaout++ = rgb1.a;
                        *rgbaout++ = rgb2.r;
                        *rgbaout++ = rgb2.g;
                        *rgbaout++ = rgb2.b;
                        *rgbaout++ = rgb2.a;

			yuv += 4;
		}
	}
}

void
SEP_Convert422toFlippedRGBA(void *PixelFrame, byte **rgba, int width, int height)
{
	int bytesperline = width * 2;
	int samplesperline = bytesperline / 4;
	byte *yuv, *rgbaout;
	BITMAP4 rgb1, rgb2;
	int y1, y2, u, v;

	rgbaout = *rgba;
	for (int line=height-1;line>=0;line--)
	{
		//printf("line = %d\n", line);
		yuv = (byte*)PixelFrame + line * bytesperline;
		for (int sample=0;sample<samplesperline;sample++)
		{
			//printf("sample = %d\n", sample);
			//u  = yuv[0];
			//y1 = yuv[1];
			//v  = yuv[2];
			//y2 = yuv[3];
			//printf("%d %d %d %d\n", y1, u, y2, v);

			rgb1 = UV_to_Bitmap(yuv[1], yuv[0], yuv[2]);
			rgb2 = UV_to_Bitmap(yuv[3], yuv[0], yuv[2]);
                        *rgbaout++ = rgb1.r;
                        *rgbaout++ = rgb1.g;
                        *rgbaout++ = rgb1.b;
                        *rgbaout++ = rgb1.a;
                        *rgbaout++ = rgb2.r;
                        *rgbaout++ = rgb2.g;
                        *rgbaout++ = rgb2.b;
                        *rgbaout++ = rgb2.a;

			yuv += 4;
		}
	}
}

void
SINGLOOP_Convert422toRGBA(void *PixelFrame, byte **rgba, int width, int height)
{
	int bytesperline = width * 2;
	int samplesperline = bytesperline / 4;
	int imagebytes = bytesperline * height;
	byte *yuv, *rgbaout;
	byte bmr, bmg, bmb, bma=0;
	int y1, y2, u, v;

	rgbaout = *rgba;
	yuv = (byte*)PixelFrame;
	while ( yuv < ((byte*)PixelFrame + imagebytes) )
	//for (int line=0;line<height;line++)
	{
		//printf("line = %d\n", line);
//		for (int sample=0;sample<samplesperline;sample++)
//		{
			//printf("sample = %d\n", sample);
			//u  = yuv[0];
			//y1 = yuv[1];
			//v  = yuv[2];
			//y2 = yuv[3];
			//printf("%d %d %d %d\n", y1, u, y2, v);

			// u and v are +-0.5
			//yuv[0] -= 128;
			//yuv[2] -= 128;
			u = yuv[0] - 128;
			v = yuv[2] - 128;

			//rgb1 = UV_to_Bitmap(yuv[1], yuv[0], yuv[2]);
			// Conversion
			bmr = yuv[1] + 1.370705 * v;
			bmg = yuv[1] - 0.698001 * v - 0.337633 * u;
			bmb = yuv[1] + 1.732446 * u;

			// Clamp to 0..1
			if (bmr < 0) bmr = 0;
			if (bmg < 0) bmg = 0;
			if (bmb < 0) bmb = 0;
			if (bmr > 255) bmr = 255;
			if (bmg > 255) bmg = 255;
			if (bmb > 255) bmb = 255;

                        *rgbaout++ = bmr;
                        *rgbaout++ = bmg;
                        *rgbaout++ = bmb;
                        *rgbaout++ = bma;


			//rgb2 = UV_to_Bitmap(yuv[3], yuv[0], yuv[2]);
			// Conversion
			bmr = yuv[3] + 1.370705 * v;
			bmg = yuv[3] - 0.698001 * v - 0.337633 * u;
			bmb = yuv[3] + 1.732446 * u;

			// Clamp to 0..1
			if (bmr < 0) bmr = 0;
			if (bmg < 0) bmg = 0;
			if (bmb < 0) bmb = 0;
			if (bmr > 255) bmr = 255;
			if (bmg > 255) bmg = 255;
			if (bmb > 255) bmb = 255;

                        *rgbaout++ = bmr;
                        *rgbaout++ = bmg;
                        *rgbaout++ = bmb;
                        *rgbaout++ = bma;


			yuv += 4;
//		}
	}
}

void
Convert422toRGBA(void *PixelFrame, byte **rgba, int width, int height)
{
	int bytesperline = width * 2;
	int samplesperline = bytesperline / 4;
	byte *yuv, *rgbaout;
	//BITMAP4 rgb1, rgb2;
	byte bmr, bmg, bmb, bma=0;
	int y1, y2, u, v;

	rgbaout = *rgba;
	for (int line=0;line<height;line++)
	{
		//printf("line = %d\n", line);
		yuv = (byte*)PixelFrame + line * bytesperline;
		for (int sample=0;sample<samplesperline;sample++)
		{
			//printf("sample = %d\n", sample);
			//u  = yuv[0];
			//y1 = yuv[1];
			//v  = yuv[2];
			//y2 = yuv[3];
			//printf("%d %d %d %d\n", y1, u, y2, v);

			// u and v are +-0.5
			//yuv[0] -= 128;
			//yuv[2] -= 128;
			u = yuv[0] - 128;
			v = yuv[2] - 128;

			//rgb1 = UV_to_Bitmap(yuv[1], yuv[0], yuv[2]);
			// Conversion
			bmr = yuv[1] + 1.370705 * v;
			bmg = yuv[1] - 0.698001 * v - 0.337633 * u;
			bmb = yuv[1] + 1.732446 * u;

			// Clamp to 0..1
			if (bmr < 0) bmr = 0;
			if (bmg < 0) bmg = 0;
			if (bmb < 0) bmb = 0;
			if (bmr > 255) bmr = 255;
			if (bmg > 255) bmg = 255;
			if (bmb > 255) bmb = 255;

                        *rgbaout++ = bmr;
                        *rgbaout++ = bmg;
                        *rgbaout++ = bmb;
                        *rgbaout++ = bma;


			//rgb2 = UV_to_Bitmap(yuv[3], yuv[0], yuv[2]);
			// Conversion
			bmr = yuv[3] + 1.370705 * v;
			bmg = yuv[3] - 0.698001 * v - 0.337633 * u;
			bmb = yuv[3] + 1.732446 * u;

			// Clamp to 0..1
			if (bmr < 0) bmr = 0;
			if (bmg < 0) bmg = 0;
			if (bmb < 0) bmb = 0;
			if (bmr > 255) bmr = 255;
			if (bmg > 255) bmg = 255;
			if (bmb > 255) bmb = 255;

                        *rgbaout++ = bmr;
                        *rgbaout++ = bmg;
                        *rgbaout++ = bmb;
                        *rgbaout++ = bma;


			yuv += 4;
		}
	}
}

