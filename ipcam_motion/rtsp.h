#ifndef RTSP_H   
#define RTSP_H 

#include <libavformat/avformat.h>

typedef struct RTSPResource
{
    AVFormatContext *pFormatContext;
    AVCodecContext  *pCodecContext; 
    int videoStreamIndex;
    AVFrame *pInputFrame;
    AVRational frameRate;
}RTSPResource;




int openIpCam(const char *ipCamResource, RTSPResource *pResource);
int openVideoCodec(RTSPResource *pResource);
int takeAPicture(RTSPResource *pResource,                 
                 const char *pictureName,
                 int jpeqQuality,
                 int delay);
void closeVideoCodec(RTSPResource *pResource);
void closeIpCam(RTSPResource *pResource);


#endif 