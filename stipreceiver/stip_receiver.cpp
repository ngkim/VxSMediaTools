#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <signal.h>
#include <time.h>
#include "../include/STIP.h"
#include "../include/dds.h"

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

int main (int argc, char** argv)
{
	int c;
	int winWidth, winHeight;
	char buffer[4096];
	struct sockaddr_in serveraddr, clientaddr;

	unsigned char *frame_buffer;
	unsigned int ptr=0;
	unsigned int compressedSize;
    time_t start,end;
  	double dif;
	unsigned int frame_count=0;

	if (argc == 1) {
		printf("Usage: %s [port_number]\n", argv[0]);
		exit(1);
	}

	int port=atoi(argv[1]);

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
	serveraddr.sin_port = htons(port);

	int state = bind(socket_fd, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
	if (state == -1)
	{
		perror("bind error : ");
		exit(0);
	}

	int recv_byte;
	bool stip_init = false;

	Packet* packet;
	STIP_init* init;
	while(1)
	{
		if (!stip_init) {
			printf("Wait for STIP init packet...\n");
		}

		recv_byte = recvfrom(socket_fd, (void *)&buffer, sizeof(buffer), 0, (struct sockaddr *)&clientaddr, &cli_len);

		if ( !stip_init && recv_byte < 30 ) {
			printf ("STIP init packet received!!!\n");

			init = (STIP_init *)buffer;

			//memcpy((void *)&init, (void *)&buffer, recv_byte);
			printf("reading initiation header info.\n");
			printf("\tsrc_video=%d\n", init->src_video);
			printf("\tproc_video=%d\n", init->proc_video);
			printf("\tsrc_px_width=%d\n", init->src_px_width);
			printf("\tsrc_px_height=%d\n", init->src_px_height);
			printf("\tsrc_bpb=%d\n", init->src_bpb);
			printf("\tproc_px_width=%d\n", init->proc_px_width);
			printf("\tproc_px_height=%d\n", init->proc_px_height);
			printf("\tproc_bpb=%d\n", init->proc_bpb);
			printf("\tframe_width=%d\n", init->frame_width);
			printf("\tframe_height=%d\n", init->frame_height);
			printf("\tmax_video_frame=%d\n", init->max_video_frame);
			printf("\tvideo_fps=%d\n", init->video_fps);

			if (init->proc_video == DXT1) {
				compressedSize = (init->frame_width * init->frame_height) * init->proc_bpb / ( init->src_px_width * init->src_px_height );

				frame_buffer = (unsigned char *)malloc(compressedSize);
				printf("malloc: %d \n", compressedSize );
			}

			stip_init = true;
		} else {     
			// video transport
			if (!stip_init)	 { 
				printf("STIP has not been initiailized!!!\n");
				continue;
			}

			packet = (Packet *)buffer;
			//printf("Frame idx= %d Received packet (block) idx= %d Expected index= %d\n", packet->header.frame_idx, packet->header.pblock_idx, idx );
			//memcpy((void *)&packet, (void *)&buffer, recv_byte);
			idx = idx + packet->header.pblock_idx + packet->header.pblock_count;

			// start timer when new frame arrived	
			if (packet->header.pblock_idx == 0) {
				// increase frame counter if all packets belonging to a frame received
				frame_count++;

				time (&end);
  				dif = difftime (end,start);
				if ( dif >= 1) {
					printf("frame rate= %5.1f\n", frame_count / dif);
					// reset timer and counter
					time(&start);
					frame_count = 0;
				}
			}
			
			//printf("block count: %d, proc_bpb: %d \n", packet.header.pblock_count, init.proc_bpb);
			memcpy((void *)&frame_buffer[packet->header.pblock_idx*init->proc_bpb], (void *)&packet->payload, packet->header.pblock_count*init->proc_bpb);
			
			// in this case, downloaded dxt blocks of a frame
			if ( ((packet->header.pblock_idx * init->proc_bpb) + (packet->header.pblock_count * init->proc_bpb))  
				== compressedSize) {
				printf("finished receiving packets of a frame...\n");
				printf("Frame idx= %d Received packet (block) idx= %d Expected index= %d\n", packet->header.frame_idx, packet->header.pblock_idx, idx );
  				

				//saveframe( frame, 1920, 1080, compressedSize);
				//bzero(frame, sizeof(frame));
			}

		}

	}
	close(socket_fd);
	return 0;
}

