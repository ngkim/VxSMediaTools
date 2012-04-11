#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/time.h>
#include "../include/STIP.h"
#include "../include/dds.h"

#define PORT 55555

int i=0;
unsigned idx=0;
unsigned int processedCount = 0;

void saveframe(unsigned char *data, int w, int h, unsigned int write_size) {


	char str[100];
	sprintf( str, "output_dxt%d.dds", i++ );
	FILE* fp = fopen(str, "wb");

	DDSHeader dds_hdr;
	dds_hdr.fourcc = FOURCC_DDS;
	dds_hdr.size = 124;
	dds_hdr.flags  = (DDSD_WIDTH|DDSD_HEIGHT|DDSD_CAPS|DDSD_PIXELFORMAT|DDSD_LINEARSIZE);
	dds_hdr.height = h;
	dds_hdr.width = w;
	dds_hdr.pitch = write_size;
	dds_hdr.depth = 0;
	dds_hdr.mipmapcount = 0;
	memset(dds_hdr.reserved, 0, sizeof(dds_hdr.reserved));
	dds_hdr.pf.size = 32;
	dds_hdr.pf.flags = DDPF_FOURCC;
	dds_hdr.pf.fourcc = FOURCC_DXT1;
	dds_hdr.pf.bitcount = 0;
	dds_hdr.pf.rmask = 0;
	dds_hdr.pf.gmask = 0;
	dds_hdr.pf.bmask = 0;
	dds_hdr.pf.amask = 0;
	dds_hdr.caps.caps1 = DDSCAPS_TEXTURE;
	dds_hdr.caps.caps2 = 0;
	dds_hdr.caps.caps3 = 0;
	dds_hdr.caps.caps4 = 0;
	dds_hdr.notused = 0;
	fwrite(&dds_hdr, sizeof(DDSHeader), 1, fp);
	fwrite(data, write_size, 1, fp);
	fclose(fp);

	printf("a frame saved..\n");

}

void timer_handler (int signum)
{
        static int count = 0;
	count = processedCount - count;
        printf("current fps: %d \n", count);
}


int main (int argc, char** argv)
{
	int c;
	int winWidth, winHeight;
	char buffer[4096];
	struct sockaddr_in serveraddr, clientaddr;
	Packet packet;
	STIP_init init;
	unsigned char *frame;
	unsigned int ptr=0;
	unsigned int compressedSize;
	struct sigaction sa;
        struct itimerval timer;
        struct timeval tv;
	double proc_start, proc_end;

	// socket setup
	socklen_t cli_len = sizeof(clientaddr);

	int socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (socket_fd < 0)
	{
		perror("socket error : ");
		exit(0);
	}

	bzero(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(PORT);

	int state = bind(socket_fd, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
	if (state == -1)
	{
		perror("bind error : ");
		exit(0);
	}

	int recv_byte;
	bool stip_init = false;
	while(1)
	{
		if (!stip_init) {
			printf("Wait for STIP init packet...\n");
		}

		recv_byte = recvfrom(socket_fd, (void *)&buffer, sizeof(buffer), 0, (struct sockaddr *)&clientaddr, &cli_len);

		if ( !stip_init && recv_byte < 30 ) {
			printf ("STIP init packet received!!!\n");

			memcpy((void *)&init, (void *)&buffer, recv_byte);
			printf("reading initiation header info.\n");
			printf("\tsrc_video=%d\n", init.src_video);
			printf("\tproc_video=%d\n", init.proc_video);
			printf("\tsrc_px_width=%d\n", init.src_px_width);
			printf("\tsrc_px_height=%d\n", init.src_px_height);
			printf("\tsrc_bpb=%d\n", init.src_bpb);
			printf("\tproc_px_width=%d\n", init.proc_px_width);
			printf("\tproc_px_height=%d\n", init.proc_px_height);
			printf("\tproc_bpb=%d\n", init.proc_bpb);
			printf("\tframe_width=%d\n", init.frame_width);
			printf("\tframe_height=%d\n", init.frame_height);
			printf("\tmax_video_frame=%d\n", init.max_video_frame);
			printf("\tvideo_fps=%d\n", init.video_fps);

			if (init.proc_video == DXT1) {
				compressedSize = init.frame_width*init.frame_height/init.proc_px_width/init.proc_px_height*init.proc_bpb;
				// start timer to measure fps
				memset (&sa, 0, sizeof (sa));
				sa.sa_handler = &timer_handler;
				sigaction (SIGVTALRM, &sa, NULL);

				frame = (unsigned char *)malloc(compressedSize);
				printf("malloc: %d \n",compressedSize );

				/* Configure the timer to expire after 250 msec... */
				timer.it_value.tv_sec = 1;
				timer.it_value.tv_usec = 0;

				/* ... and every 250 msec after that. */
				timer.it_interval.tv_sec = 1;
				timer.it_interval.tv_usec = 0;

				/* Start a virtual timer. It counts down whenever this process is executing. */
				setitimer (ITIMER_VIRTUAL, &timer, NULL);
			}

			stip_init = true;
		} else {                // video transport
			if (!stip_init)	 { 
				printf("STIP has not been initiailized!!!\n");
				continue;
			}

			memcpy((void *)&packet, (void *)&buffer, recv_byte);
			idx = idx + packet.header.pblock_idx + packet.header.pblock_count;
			//printf("received packet (block) idx: %d, expected index: %d frame idx: %d \n", packet.header.pblock_idx, idx, packet.header.frame_idx);

			// start timer when new frame arrived	
			if (packet.header.pblock_idx == 0) {
				gettimeofday(&tv,NULL);
                                proc_start = tv.tv_usec;	
			}
			
			memcpy((void *)&frame[packet.header.pblock_idx*init.proc_bpb], (void *)&packet.payload, packet.header.pblock_count*init.proc_bpb);
			//printf("block count: %d, proc_bpb: %d \n", packet.header.pblock_count, init.proc_bpb);
			
			// in this case, downloaded dxt blocks of a frame
			if ( (packet.header.pblock_idx*8+packet.header.pblock_count*8)  == compressedSize) {
				processedCount++;
				//processing time
				gettimeofday(&tv,NULL);
				proc_end = tv.tv_usec;
			 //	printf("processing time (current frame): %2.2fms \n", proc_end - proc_start);

				saveframe( frame, 1920, 1080, compressedSize);
				bzero(frame, sizeof(frame));
			}

		}

	}
	close(socket_fd);
	return 0;
}

