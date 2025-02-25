
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <math.h>
#include <fcntl.h>
#include <string.h>
//#include <time.h>
#include <sys/time.h>
#include "yolov2.h"
#include "cnn.h"

#ifdef DMA_TRANSFER
#include "dma-proxy.h"
#endif

struct dma_proxy_channel_interface *tx_proxy_interface_p;
struct dma_proxy_channel_interface *rx_proxy_interface_p;
int rx_proxy_fd, tx_proxy_fd;
int dummy;

int main( int argc, char *argv[])
{
#ifdef DMA_TRANSFER	
	// Open the DMA proxy device for the transmit and receive channels
	tx_proxy_fd = open("/dev/h2c", O_RDWR);

	if (tx_proxy_fd < 1) {
		printf("Unable to open DMA proxy device file");
		exit(EXIT_FAILURE);
	}

	rx_proxy_fd = open("/dev/c2h", O_RDWR);
	if (tx_proxy_fd < 1) {
		printf("Unable to open DMA proxy device file");
		exit(EXIT_FAILURE);
	}

	// Map the transmit and receive channels memory into user space so it's accessible
	tx_proxy_interface_p = (struct dma_proxy_channel_interface *)mmap(NULL, sizeof(struct dma_proxy_channel_interface),
									PROT_READ | PROT_WRITE, MAP_SHARED, tx_proxy_fd, 0);

	rx_proxy_interface_p = (struct dma_proxy_channel_interface *)mmap(NULL, sizeof(struct dma_proxy_channel_interface),
									PROT_READ | PROT_WRITE, MAP_SHARED, rx_proxy_fd, 0);
	if ((rx_proxy_interface_p == MAP_FAILED) || (tx_proxy_interface_p == MAP_FAILED)) {
		printf("Failed to mmap\n");
		exit(EXIT_FAILURE);
	}
#endif
	//freopen("result.txt","w",stdout);
	//printf("sizeof(unsigned long) =%lu\n",sizeof(unsigned long));
	printf("YOLOv2 TEST Begin\n");
    	char **names = get_labels("coco.names");
	int x;
/*
	for(x=0;x<80;x++)//80 classe labels
	{
		printf("[%d]%s\n",x,names[x]);
	}
*/
    	image **alphabet = load_alphabet();
    	network *net = load_network("yolov2.cfg");
	set_batch_network(net, 1);

////////////////////load img resize img begin
	char img_buff[256];
	char *input_imgfn = img_buff;
	if(argc==1)
		strncpy(input_imgfn, "../test_imgs/kite.jpg", 256);
	else
		strncpy(input_imgfn, argv[1], 256);
	image im = load_image_stb(input_imgfn, 3);//3 channel img
	printf("Input img:%s\n w=%d,h=%d,c=%d\n", input_imgfn, im.w, im.h, im.c);
	image sized = letterbox_image(im, 416, 416);
	save_image_png(sized, "sized");// convert to yolov3 net input size 416x416x3
////////////////////load img resize img end

	double first, second;
	layer l = net->layers[net->n-1];
    	float *X = sized.data;

	first = what_time_is_it_now();
	yolov2_hls_ps(net, X);
	second = what_time_is_it_now();
	printf("%s: %s Predicted in %f seconds.\n", input_imgfn, argv[0], second - first);

	int nboxes = 0;
    	float nms=.45;
	float thresh = .5;
	float hier_thresh = .5;
	detection *dets = get_network_boxes(net, im.w, im.h, thresh, hier_thresh, 0, 1, &nboxes);
	printf("%d\n", nboxes);
	for(x=0;x<nboxes;x++)
	{
		if(dets[x].objectness > 0.0)
			printf("[%3d]:h=%f,w=%f,x=%f,y=%f,objectness=%f\n",x,dets[x].bbox.h,dets[x].bbox.w,dets[x].bbox.x,dets[x].bbox.y,dets[x].objectness);
	}

	if (nms) do_nms_sort(dets, nboxes, l.classes, nms);
	draw_detections(im, dets, nboxes, thresh, names, alphabet, l.classes);

	free_detections(dets, nboxes);	
///////////////////write predictions img
	save_image_png(im, "predictions");// output

	free_image(im);
	free_image(sized);

#ifdef DMA_TRANSFER
	//Unmap the proxy channel interface memory and close the device files before leaving
	munmap(tx_proxy_interface_p, sizeof(struct dma_proxy_channel_interface));
	munmap(rx_proxy_interface_p, sizeof(struct dma_proxy_channel_interface));
	close(tx_proxy_fd);
	close(rx_proxy_fd);
#endif
	printf("YOLOv2 TEST End\n");
	return 0;
}

