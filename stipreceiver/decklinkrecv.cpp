#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "STIP.h"
#include "dds.h"

#define PORT 55555

void saveframe(unsigned char *data, int w, int h, unsigned int write_size) {


	FILE* fp = fopen("output_dxt.dds", "wb");

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

int main(int argc, char **argv)
{
	int socket_fd;
	socklen_t cli_len;

	int recv_byte;
	char buffer[4096];
	struct sockaddr_in serveraddr, clientaddr;
	Packet packet;
	STIP_init init;
	unsigned char *frame;
	unsigned int ptr=0;

	cli_len = sizeof(clientaddr);

	socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
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

	while(1)
	{
		recv_byte = recvfrom(socket_fd, (void *)&buffer, sizeof(buffer), 0, (struct sockaddr *)&clientaddr, &cli_len);
	
		//printf("packet received: %d \n", recv_byte);
		if (recv_byte < 30) {	// initiation packet
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
			//printf("video w: %d, video h: %d, fps: %d \n", init.frame_width, init.frame_height, init.video_fps);
			if (init.proc_video == DXT1) {
				frame = (unsigned char *)malloc(init.frame_width*init.frame_height/init.proc_px_width/init.proc_px_height*init.proc_bpb);
			printf("malloc: %d \n",init.frame_width*init.frame_height/init.proc_px_width/init.proc_px_height*init.proc_bpb );
			}
		} else {		// video transport
			memcpy((void *)&packet, (void *)&buffer, recv_byte);


			//memcpy((void *)&frame[ptr], (void *)&packet.payload, init.proc_bpb*packet.header.pblock_count);	
			memcpy((void *)&frame[packet.header.pblock_idx*init.proc_bpb], (void *)&packet.payload, packet.header.pblock_count*init.proc_bpb);	
			//printf("frame index to write: %d, block_idx: %d \n ", packet.header.pblock_idx*init.proc_bpb, packet.header.pblock_idx);
			//if (packet.header.frame_idx == 0) printf("frame idx: %d \n", ptr);

			//printf("ptr: %d, 1: %d, 2: %d, proc_bpb: %d, count: %d\n",ptr,init.proc_bpb*packet.header.pblock_count, sizeof(packet.payload), init.proc_bpb,packet.header.pblock_count);
			//ptr = ptr + init.proc_bpb*packet.header.pblock_count;
			//if (ptr != packet.header.pblock_count) printf("packet loss !! \n");			
		//	printf("pblock_idx: %d, count: %d, bpb: %d\n",packet.header.pblock_idx, packet.header.pblock_count, init.proc_bpb);
			ptr = ptr + packet.header.pblock_count;
			

		//	printf("save pointer: %d, total size: %d \n", packet.header.pblock_idx*8+packet.header.pblock_count*8, init.frame_width*init.frame_height/init.proc_px_width/init.proc_px_height*init.proc_bpb);
			if ( (packet.header.pblock_idx*8+packet.header.pblock_count*8)  == (init.frame_width*init.frame_height/init.proc_px_width/init.proc_px_height*init.proc_bpb)) {
				ptr = 0;
				//printf("save frame? \n");
				if (packet.header.frame_idx == 0)  { 
					//printf("packet recv, idx: %d \n", packet.header.pblock_idx);		
					saveframe(frame, init.frame_width, init.frame_height, init.frame_width*init.frame_height/init.proc_px_width/init.proc_px_height*init.proc_bpb);
				}

			}
		}

	}
	close(socket_fd);
}
