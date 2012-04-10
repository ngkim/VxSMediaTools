#include "DeckLinkAPI.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "STIP.h"

/*
#define PKT_HEADER 16
#define BLOCK_SIZE 32
#define BLOCK 45
#define PKT_SIZE (BLOCK_SIZE * BLOCK)

struct _packet {
        STIP_hdr header;
        char payload[PKT_SIZE];
} packet;
*/

Packet packet;


int socket_fd;
struct sockaddr_in serveraddr;

void	print_input_modes (IDeckLink* deckLink);
void	print_output_modes (IDeckLink* deckLink);

void	print_input_capabilities (IDeckLink* deckLink);
void	print_output_capabilities (IDeckLink* deckLink);

void	print_capabilities();
void	capture(int device, int mode, int connection);

int GetFrameSize(int card, int mode, int *winWidth, int *winHeight);
void *PixelFrame;
unsigned *buffer;
unsigned int block_num, framecount=0;


int main (int argc, char** argv)
{
	int c;
	int done = 0;
	int card = 0;
	int mode = -1;
	int connexion = -1;
	int winWidth, winHeight;

	// add socket client
	char ip[16];
	unsigned int port;
	STIP_init init;
	

	socket_fd = socket (AF_INET, SOCK_DGRAM, 0);

	if (socket_fd < 0) {
		printf("socket error \n");
		return 0;
	} 
   
	while ((c = getopt(argc, argv, "lhc:m:i:d:a:p:")) != EOF )
	{
	    switch(c)
	    {
		case 'l':
		    print_capabilities();
		    exit(0);
		    break;

		case 'h':
		    fprintf(stderr, "Usage> %s \n", argv[0]);
		    fprintf(stderr, "\t\t\t -l list capabilities\n");
		    fprintf(stderr, "\t\t\t -c select Card to use (default 0)\n");
		    fprintf(stderr, "\t\t\t -m select input video Mode\n");
		    fprintf(stderr, "\t\t\t -i select connection Interface\n");
		    fprintf(stderr, "\t\t\t -d number of DMX threads (default 2)\n");
		    fprintf(stderr, "\t\t\t    (0 means no DMX compression)\n");
		    exit(0);
		    break;
		case 'a':
		    // server address
		    memcpy(ip,optarg, strlen(optarg));
		    ip[strlen(optarg)]='\0';
		    break;
		case 'p':
		    // server port 
		    port = atoi(optarg);
		    break;


		case 'c':
		    // The capture card to use
		    card = atoi(optarg);
		    break;

		case 'm':
		    // The capture video mode
		    mode = atoi(optarg);
		    break;

		case 'i':
		    // The interface to use
		    connexion = atoi(optarg);
		    break;

		default:
		    fprintf(stderr, "Decklink Capture Program\n");
		    sprintf(ip,"%s","210.125.84.90");
		    port = 55555;
		    exit(0);
		    break;
	    }
		
	}
	if ( (mode < 0) || (connexion < 0) )
	{
	    fprintf(stderr, "Please specify both input mode (-m) and connection interface (-i) from the following lists\n\n");
	    print_capabilities();
	    exit(1);
	}

	if ( GetFrameSize(card, mode, &winWidth, &winHeight) < 0 )
	{
		print_capabilities();
		exit(1);
	}

	block_num = winWidth / 4 * winHeight / 4;

	serveraddr.sin_family		= AF_INET;
	serveraddr.sin_port 		= htons(port);
	serveraddr.sin_addr.s_addr	= inet_addr(ip);
	bzero(&(serveraddr.sin_zero), 8);

	printf("address of PixelFrame in main: %016llx, &framecount: %016llx \n", &PixelFrame, &framecount);

	// initialization (socket server)
    init.version            = 0x01;
    init.hdr_len            = sizeof(init);
	init.src_video          = YUV422;
    init.proc_video         = DXT1;
    init.src_px_width       = 4;
    init.src_px_height      = 4;
    init.src_bpb            = 32;
    init.proc_px_width      = 4;
    init.proc_px_height     = 4;
    init.proc_bpb           = 8;
	init.frame_width		= winWidth;
	init.frame_height		= winHeight;
	init.max_video_frame	= 0;
	init.video_fps			= 20;

	sendto(socket_fd, (void *)&init, sizeof(init), 0, (struct sockaddr *)&serveraddr, sizeof(serveraddr));

	sleep(1);

        // Capture setup
	capture(card, mode, connexion);
 
        while (!done) {
            //sleep(1);
        }
        

	return 0;
}

class VideoDelegate : public IDeckLinkInputCallback
{
private:
	int32_t mRefCount;
	double  lastTime;
    unsigned int     framecount;
    
public:
    VideoDelegate () {
         framecount = 0;
    };
						
	virtual HRESULT		QueryInterface (REFIID iid, LPVOID *ppv)
        {};
	virtual ULONG		AddRef (void) {
        return mRefCount++;
    };
	virtual ULONG		Release (void) {
        int32_t		newRefValue;
        
        newRefValue = mRefCount--;
        if (newRefValue == 0)
        {
            delete this;
            return 0;
        }        
        return newRefValue;
    };

	virtual HRESULT	VideoInputFrameArrived (IDeckLinkVideoInputFrame* arrivedFrame,
                                                IDeckLinkAudioInputPacket*);
        virtual HRESULT VideoInputFormatChanged(BMDVideoInputFormatChangedEvents, IDeckLinkDisplayMode*, BMDDetectedVideoInputFormatFlags)
        {};

};

HRESULT	VideoDelegate::VideoInputFrameArrived (IDeckLinkVideoInputFrame* arrivedFrame,
                                               IDeckLinkAudioInputPacket*)
{
	BMDTimeValue		frameTime, frameDuration;
	int					hours, minutes, seconds, frames, width, height;
	HRESULT				theResult;

	struct                  timeval tv;
    long                    start, end;


	arrivedFrame->GetStreamTime(&frameTime, &frameDuration, 600);

	hours = (frameTime / (600 * 60*60));
	minutes = (frameTime / (600 * 60)) % 60;
	seconds = (frameTime / 600) % 60;
	frames = (frameTime / 6) % 100;
#if 0
	if ((frames % 8) == 0 )
	{
		fprintf(stderr, "drop > %02d:%02d:%02d:%02d\n", hours, minutes, seconds, frames);
		return S_OK;
	}
	else
		fprintf(stderr, "frame> %02d:%02d:%02d:%02d\n", hours, minutes, seconds, frames);
#endif
	//fprintf(stderr, "\t %dx%d\r", arrivedFrame->GetWidth(), arrivedFrame->GetHeight() );

	arrivedFrame->GetBytes(&PixelFrame);
	width = arrivedFrame->GetWidth();
	height = arrivedFrame->GetHeight();       

	unsigned char *t_data = NULL;
	unsigned char tt_data[4147200];
	t_data = (unsigned char*)malloc(width*height*2);

	int a;
	int *b;

	//printf("address of &PixelFrame: %016llx, PixelFrame: %016llx, &t_data: %016llx, t_data: %016llx, &tt_data: %016llx, tt_data: %016llx, &a: %016llx, &b: %016llx, b: %016llx \n", &PixelFrame, PixelFrame, &t_data, t_data, &tt_data, tt_data, &a, &b, b);
	//printf("address of &t_data: %016llx, t_data: %016llx, &a: %016llx, &b: %016llx, b: %016llx \n", &t_data, t_data, &a, &b, b);

/*
	gettimeofday(&tv,NULL);
        start = tv.tv_usec;
	memcpy(t_data, PixelFrame, width*height*2);
	gettimeofday(&tv, NULL);
	end = tv.tv_usec;
	printf("time of copy PixelFrame to t_data: %ld \n", end-start);
*/


	// send to DXT switch?
	if (framecount == 0) buffer = (unsigned *) malloc(sizeof(int)*width*height/2);
	// linear video	

	// Convert linear image to block linear. 
	for(unsigned int by = 0; by < height/4; by++) {
		for(unsigned int bx = 0; bx < width/4; bx++) {
			for (int i = 0; i < 8; i++) {
				const int x = i & 1;
				const int y = i / 2;
				buffer[(by * width/4 + bx) * 8 + i] = 
					((unsigned int *)PixelFrame)[(by * 4 + y) * 2 * (width/4) + bx * 2 + x];
				int a1 = (by * width/4 + bx) * 8 + i;
				int a2 = (by * 4 + y) * 2 * (width/4) + bx * 2 + x;
			//if (framecount == 0) printf("buf idx: %u, px idx: %u \n", (by * width/4 + bx) * 8 + i, (by * 4 + y) * 2 * (width/4) + bx * 2 + x); 
			}
		}
	}

	// set packet header
	packet.header.version 	= 0x01;
	packet.header.hdr_len 	= sizeof(packet.header);
	packet.header.frame_idx = framecount;
	packet.header.pblock_count = BLOCK;

	for (int i=0; i<block_num/BLOCK; i++) {
		packet.header.pblock_idx = i * BLOCK;
		memcpy(packet.payload,(void *)&buffer[BLOCK*i*8],PKT_SIZE);
		sendto(socket_fd, (void *)&packet, sizeof(packet), 0, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
	}

	// <-- by lucas


#if 0
        char fn[128];
        memset(fn, 0, 128);
        sprintf(fn, "frame%04d.yuv", framecount);
        FILE *f=fopen(fn, "w+");
        fwrite(PixelFrame, 1, 1920*1080*2, f);
        fclose(f);
#endif   

        framecount++;
        return S_OK;

}


void capture(int device, int mode, int connection)
{
    int dnum, mnum, cnum;
 	int	itemCount;
   
	IDeckLinkIterator *deckLinkIterator;
	IDeckLink         *deckLink;
	HRESULT            result;

	IDeckLinkInput*               deckLinkInput = NULL;
	IDeckLinkDisplayModeIterator* displayModeIterator = NULL;
	IDeckLinkDisplayMode*         displayMode = NULL;
	IDeckLinkConfiguration       *deckLinkConfiguration = NULL;
    VideoDelegate                *delegate;


    fprintf(stderr, "Starting> Capture on device %d, mode %d, connection %d\n", device, mode, connection);
    
        // Create an IDeckLinkIterator object to enumerate all DeckLink cards in the system
	deckLinkIterator = CreateDeckLinkIteratorInstance();
	if (deckLinkIterator == NULL)
	{
		fprintf(stderr, "A DeckLink iterator could not be created (DeckLink drivers may not be installed).\n");
		return;
	}

    dnum = 0;
    deckLink = NULL;
    
	while (deckLinkIterator->Next(&deckLink) == S_OK)
	{
        if (device != dnum) {
            dnum++;
                // Release the IDeckLink instance when we've finished with it to prevent leaks
            deckLink->Release();
            continue;
        }
        dnum++;
        
		const char *deviceNameString = NULL;
		
            // *** Print the model name of the DeckLink card
		result = deckLink->GetModelName(&deviceNameString);
		if (result == S_OK)
		{
			char deviceName[64];	
			fprintf(stderr, "Using device [%s]\n", deviceNameString);

                // Query the DeckLink for its configuration interface
            result = deckLink->QueryInterface(IID_IDeckLinkInput, (void**)&deckLinkInput);
            if (result != S_OK)
            {
                fprintf(stderr, "Could not obtain the IDeckLinkInput interface - result = %08x\n", result);
                return;
            }

                // Obtain an IDeckLinkDisplayModeIterator to enumerate the display modes supported on input
            result = deckLinkInput->GetDisplayModeIterator(&displayModeIterator);
            if (result != S_OK)
            {
                fprintf(stderr, "Could not obtain the video input display mode iterator - result = %08x\n", result);
                return;
            }

            mnum = 0;
            while (displayModeIterator->Next(&displayMode) == S_OK)
            {
                if (mode != mnum) {
                    mnum++;
                        // Release the IDeckLinkDisplayMode object to prevent a leak
                    displayMode->Release();
                    continue;
                }
                mnum++;                
     
                const char *displayModeString = NULL;
                
                result = displayMode->GetName(&displayModeString);
                if (result == S_OK)
                {
                    BMDPixelFormat pf = bmdFormat8BitYUV;

                    fprintf(stderr, "Stopping previous streams, if needed\n");
                    deckLinkInput->StopStreams();

                    const char *displayModeString = NULL;		
                    displayMode->GetName(&displayModeString);
                    //fprintf(stderr, "Enable video input: %s\n", displayModeString);
                    fprintf(stderr, "Enable video input: %s\n", displayModeString);

                    deckLinkInput->EnableVideoInput(displayMode->GetDisplayMode(), pf, 0);



                    // Query the DeckLink for its configuration interface
                    result = deckLinkInput->QueryInterface(IID_IDeckLinkConfiguration, (void**)&deckLinkConfiguration);
                    if (result != S_OK)
                    {
                        fprintf(stderr, "Could not obtain the IDeckLinkConfiguration interface: %08x\n", result);
                        return;
                    }
                    BMDVideoConnection conn;
                    switch (connection) {
                        case 0:
                            conn = bmdVideoConnectionSDI;
                            break;
                        case 1:
                            conn = bmdVideoConnectionHDMI;
                            break;
                        case 2:
                            conn = bmdVideoConnectionComponent;
                            break;
                        case 3:
                            conn = bmdVideoConnectionComposite;
                            break;
                        case 4:
                            conn = bmdVideoConnectionSVideo;
                            break;
                        case 5:
                            conn = bmdVideoConnectionOpticalSDI;
                            break;
                        default:
                            break;
                    }
                    
                    if (deckLinkConfiguration->SetVideoInputFormat(conn) == S_OK) {
                        fprintf(stderr, "Input set to: %d\n", connection);
                    }
                    
                    delegate = new VideoDelegate();
                    deckLinkInput->SetCallback(delegate);
                    
                    fprintf(stderr, "Start capture\n", connection);
                    deckLinkInput->StartStreams();

               }

            }
	
		}
	}


        // Release the IDeckLink instance when we've finished with it to prevent leaks
    if (deckLink) deckLink->Release();
        // Ensure that the interfaces we obtained are released to prevent a memory leak
	if (displayModeIterator != NULL)
		displayModeIterator->Release();

   return;
}
    

void print_capabilities()
{
	IDeckLinkIterator* deckLinkIterator;
	IDeckLink*         deckLink;
	int                numDevices = 0;
	HRESULT            result;
	
        // Create an IDeckLinkIterator object to enumerate all DeckLink cards in the system
	deckLinkIterator = CreateDeckLinkIteratorInstance();
	if (deckLinkIterator == NULL)
	{
		fprintf(stderr, "A DeckLink iterator could not be created (DeckLink drivers may not be installed).\n");
		return;
	}
	
        // Enumerate all cards in this system
	printf("Device(s):\n");
	while (deckLinkIterator->Next(&deckLink) == S_OK)
	{
		const char *		deviceNameString = NULL;
		
            // *** Print the model name of the DeckLink card
		result = deckLink->GetModelName(&deviceNameString);
		if (result == S_OK)
		{
			char			deviceName[64];
			
			printf("\t%3d: =============== %s ===============\n", numDevices, deviceNameString);
		}
		
            // ** List the video output display modes supported by the card
		print_input_modes(deckLink);
		
            // ** List the input capabilities of the card
		print_input_capabilities(deckLink);
		
            // Release the IDeckLink instance when we've finished with it to prevent leaks
		deckLink->Release();

            // Increment the total number of DeckLink cards found
		numDevices++;
	}
	
	
        // If no DeckLink cards were found in the system, inform the user
	if (numDevices == 0)
		printf("No Blackmagic Design devices were found.\n");
	printf("\n");
}



void print_input_modes (IDeckLink* deckLink)
{
	IDeckLinkInput*				deckLinkInput = NULL;
	IDeckLinkDisplayModeIterator*		displayModeIterator = NULL;
	IDeckLinkDisplayMode*			displayMode = NULL;
	HRESULT					result;	
	int mm = 0;
	
        // Query the DeckLink for its configuration interface
	result = deckLink->QueryInterface(IID_IDeckLinkInput, (void**)&deckLinkInput);
	if (result != S_OK)
	{
		fprintf(stderr, "Could not obtain the IDeckLinkInput interface - result = %08x\n", result);
		goto bail;
	}
	
        // Obtain an IDeckLinkDisplayModeIterator to enumerate the display modes supported on input
	result = deckLinkInput->GetDisplayModeIterator(&displayModeIterator);
	if (result != S_OK)
	{
		fprintf(stderr, "Could not obtain the video input display mode iterator - result = %08x\n", result);
		goto bail;
	}
	
        // List all supported output display modes
	printf("Supported video input display modes:\n");
	while (displayModeIterator->Next(&displayMode) == S_OK)
	{
		const char *			displayModeString = NULL;
		
		result = displayMode->GetName(&displayModeString);
		if (result == S_OK)
		{
			char		modeName[64];
			int		modeWidth;
			int		modeHeight;
			BMDTimeValue	frameRateDuration;
			BMDTimeScale	frameRateScale;
			
			
                // Obtain the display mode's properties
			modeWidth = displayMode->GetWidth();
			modeHeight = displayMode->GetHeight();
			displayMode->GetFrameRate(&frameRateDuration, &frameRateScale);
			printf("\t%3d) %-20s \t %d x %d \t %g FPS\n", mm, displayModeString, modeWidth, modeHeight, (double)frameRateScale / (double)frameRateDuration);
			mm++;
		}
		
            // Release the IDeckLinkDisplayMode object to prevent a leak
		displayMode->Release();
	}
	
	printf("\n");
	
  bail:
        // Ensure that the interfaces we obtained are released to prevent a memory leak
	if (displayModeIterator != NULL)
		displayModeIterator->Release();
	
	if (deckLinkInput != NULL)
		deckLinkInput->Release();
}


void	print_output_modes (IDeckLink* deckLink)
{
	IDeckLinkOutput*					deckLinkOutput = NULL;
	IDeckLinkDisplayModeIterator*		displayModeIterator = NULL;
	IDeckLinkDisplayMode*				displayMode = NULL;
	HRESULT								result;	
	
        // Query the DeckLink for its configuration interface
	result = deckLink->QueryInterface(IID_IDeckLinkOutput, (void**)&deckLinkOutput);
	if (result != S_OK)
	{
		fprintf(stderr, "Could not obtain the IDeckLinkOutput interface - result = %08x\n", result);
		goto bail;
	}
	
        // Obtain an IDeckLinkDisplayModeIterator to enumerate the display modes supported on output
	result = deckLinkOutput->GetDisplayModeIterator(&displayModeIterator);
	if (result != S_OK)
	{
		fprintf(stderr, "Could not obtain the video output display mode iterator - result = %08x\n", result);
		goto bail;
	}
	
        // List all supported output display modes
	printf("Supported video output display modes:\n");
	while (displayModeIterator->Next(&displayMode) == S_OK)
	{
		const char *			displayModeString = NULL;
		
		result = displayMode->GetName(&displayModeString);
		if (result == S_OK)
		{
			char			modeName[64];
			int				modeWidth;
			int				modeHeight;
			BMDTimeValue	frameRateDuration;
			BMDTimeScale	frameRateScale;
			
			
                // Obtain the display mode's properties
			modeWidth = displayMode->GetWidth();
			modeHeight = displayMode->GetHeight();
			displayMode->GetFrameRate(&frameRateDuration, &frameRateScale);
			printf("\t%-20s \t %d x %d \t %g FPS\n", displayModeString, modeWidth, modeHeight, (double)frameRateScale / (double)frameRateDuration);
		}
		
            // Release the IDeckLinkDisplayMode object to prevent a leak
		displayMode->Release();
	}
	
	printf("\n");
	
  bail:
        // Ensure that the interfaces we obtained are released to prevent a memory leak
	if (displayModeIterator != NULL)
		displayModeIterator->Release();
	
	if (deckLinkOutput != NULL)
		deckLinkOutput->Release();
}

void	print_input_capabilities (IDeckLink* deckLink)
{
	IDeckLinkConfiguration*		deckLinkConfiguration = NULL;
	IDeckLinkConfiguration*		deckLinkValidator = NULL;
	int							itemCount;
	HRESULT						result;	
	int mm = 0;
	
        // Query the DeckLink for its configuration interface
	result = deckLink->QueryInterface(IID_IDeckLinkConfiguration, (void**)&deckLinkConfiguration);
	if (result != S_OK)
	{
		fprintf(stderr, "Could not obtain the IDeckLinkConfiguration interface - result = %08x\n", result);
		goto bail;
	}
	
        // Obtain a validator object from the IDeckLinkConfiguration interface.
        // The validator object implements IDeckLinkConfiguration, however, all configuration changes are ignored
        // and will not take effect.  However, you can use the returned result code from the validator object
        // to determine whether the card supports a particular configuration.
	
	result = deckLinkConfiguration->GetConfigurationValidator(&deckLinkValidator);
	if (result != S_OK)
	{
		fprintf(stderr, "Could not obtain the configuration validator interface - result = %08x\n", result);
		goto bail;
	}
	
        // Use the validator object to determine which video input connections are available
	printf("Supported video input connections:\n  ");
	itemCount = 0;
	if (deckLinkValidator->SetVideoInputFormat(bmdVideoConnectionSDI) == S_OK)
	{
		printf("\t%3d) SDI\n", itemCount);
	}
    itemCount++;
	if (deckLinkValidator->SetVideoInputFormat(bmdVideoConnectionHDMI) == S_OK)
	{
		printf("\t%3d) HDMI\n", itemCount);
	}
    itemCount++;
	if (deckLinkValidator->SetVideoInputFormat(bmdVideoConnectionComponent) == S_OK)
	{
		printf("\t%3d) Component\n", itemCount);
	}
    itemCount++;
	if (deckLinkValidator->SetVideoInputFormat(bmdVideoConnectionComposite) == S_OK)
	{
		printf("\t%3d) Composite\n", itemCount);
	}
    itemCount++;
	if (deckLinkValidator->SetVideoInputFormat(bmdVideoConnectionSVideo) == S_OK)
	{
		printf("\t%3d) S-Video\n", itemCount);
	}
    itemCount++;
	if (deckLinkValidator->SetVideoInputFormat(bmdVideoConnectionOpticalSDI) == S_OK)
	{
		printf("\t%3d) Optical SDI\n", itemCount);
	}
	
	printf("\n");
	
  bail:
	if (deckLinkValidator != NULL)
		deckLinkValidator->Release();
	
	if (deckLinkConfiguration != NULL)
		deckLinkConfiguration->Release();
}

void	print_output_capabilities (IDeckLink* deckLink)
{
	IDeckLinkConfiguration*		deckLinkConfiguration = NULL;
	IDeckLinkConfiguration*		deckLinkValidator = NULL;
	int							itemCount;
	HRESULT						result;	
	
	// Query the DeckLink for its configuration interface
	result = deckLink->QueryInterface(IID_IDeckLinkConfiguration, (void**)&deckLinkConfiguration);
	if (result != S_OK)
	{
		fprintf(stderr, "Could not obtain the IDeckLinkConfiguration interface - result = %08x\n", result);
		goto bail;
	}
	
	// Obtain a validator object from the IDeckLinkConfiguration interface.
	// The validator object implements IDeckLinkConfiguration, however, all configuration changes are ignored
	// and will not take effect.  However, you can use the returned result code from the validator object
	// to determine whether the card supports a particular configuration.
	
	result = deckLinkConfiguration->GetConfigurationValidator(&deckLinkValidator);
	if (result != S_OK)
	{
		fprintf(stderr, "Could not obtain the configuration validator interface - result = %08x\n", result);
		goto bail;
	}
	
	// Use the validator object to determine which video output connections are available
	printf("Supported video output connections:\n  ");
	itemCount = 0;
	if (deckLinkValidator->SetVideoOutputFormat(bmdVideoConnectionSDI) == S_OK)
	{
		if (itemCount++ > 0)
			printf(", ");
		printf("SDI");
	}
	if (deckLinkValidator->SetVideoOutputFormat(bmdVideoConnectionHDMI) == S_OK)
	{
		if (itemCount++ > 0)
			printf(", ");
		printf("HDMI");
	}
	if (deckLinkValidator->SetVideoOutputFormat(bmdVideoConnectionComponent) == S_OK)
	{
		if (itemCount++ > 0)
			printf(", ");
		printf("Component");
	}
	if (deckLinkValidator->SetVideoOutputFormat(bmdVideoConnectionComposite) == S_OK)
	{
		if (itemCount++ > 0)
			printf(", ");
		printf("Composite");
	}
	if (deckLinkValidator->SetVideoOutputFormat(bmdVideoConnectionSVideo) == S_OK)
	{
		if (itemCount++ > 0)
			printf(", ");
		printf("S-Video");
	}
	if (deckLinkValidator->SetVideoOutputFormat(bmdVideoConnectionOpticalSDI) == S_OK)
	{
		if (itemCount++ > 0)
			printf(", ");
		printf("Optical SDI");
	}
	
	printf("\n\n");
	
bail:
	if (deckLinkValidator != NULL)
		deckLinkValidator->Release();
	
	if (deckLinkConfiguration != NULL)
		deckLinkConfiguration->Release();
}

// Given a mode number, determine the winWidth and winHeight for this particular card id
int
GetFrameSize(int card, int mode, int *winWidth, int *winHeight)
{
	IDeckLinkIterator* deckLinkIterator;
	IDeckLink*         deckLink;
	int                numDevices = 0;
	HRESULT            result;
	bool modeFound = 0;
	
        // Create an IDeckLinkIterator object to enumerate all DeckLink cards in the system
	deckLinkIterator = CreateDeckLinkIteratorInstance();
	if (deckLinkIterator == NULL)
	{
		fprintf(stderr, "A DeckLink iterator could not be created (DeckLink drivers may not be installed).\n");
		return -1;
	}

        // Try all cards in this system
	while (deckLinkIterator->Next(&deckLink) == S_OK)
	{
		const char *		deviceNameString = NULL;

		if ( card == numDevices )
		{
			IDeckLinkInput*				deckLinkInput = NULL;
			IDeckLinkDisplayModeIterator*		displayModeIterator = NULL;
			IDeckLinkDisplayMode*			displayMode = NULL;
			HRESULT					result;	
			int mm = 0;
			
			// Query the DeckLink for its configuration interface
			result = deckLink->QueryInterface(IID_IDeckLinkInput, (void**)&deckLinkInput);
			if (result != S_OK)
			{
				fprintf(stderr, "Could not obtain the IDeckLinkInput interface - result = %08x\n", result);
				goto bail;
			}
			
			// Obtain an IDeckLinkDisplayModeIterator to enumerate the display modes supported on input
			result = deckLinkInput->GetDisplayModeIterator(&displayModeIterator);
			if (result != S_OK)
			{
				fprintf(stderr, "Could not obtain the video input display mode iterator - result = %08x\n", result);
				goto bail;
			}
			
			// List all supported input modes
			while (displayModeIterator->Next(&displayMode) == S_OK)
			{
				const char *			displayModeString = NULL;
				
				result = displayMode->GetName(&displayModeString);
				if ( result == S_OK )
				{
					if (mode == mm )
					{
						*winWidth = displayMode->GetWidth();
						*winHeight = displayMode->GetHeight();
						modeFound = 1;

						displayMode->Release();
						break;
					}
					mm++;
				}
				
				// Release the IDeckLinkDisplayMode object to prevent a leak
				displayMode->Release();
			}
			
		  bail:
			// Ensure that the interfaces we obtained are released to prevent a memory leak
			if (displayModeIterator != NULL)
				displayModeIterator->Release();
			
			if (deckLinkInput != NULL)
				deckLinkInput->Release();
		}

		numDevices++;
	}
	
        // If no DeckLink cards were found in the system, inform the user
	if (numDevices == 0)
	{
		fprintf(stderr, "No Blackmagic Design devices were found.\n");
		return -2;
	}


	if ( modeFound )
		return 0;
	else
	{
		fprintf(stderr, "\tNo mode number %d available on this card\n",
                            mode);
		fprintf(stderr, "\tPlease choose one of the \"Supported video input display modes\" listed below.\n\n");
		return -3;
	}
}
