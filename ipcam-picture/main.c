#include <stddef.h>
#include <unistd.h>
#include "rtsp.h"
#include "util.h"

#define TMP_FILE_1 "pictureName_1.jpg"
#define TMP_FILE_2 "pictureName_2.jpg"
#define TMP_FILE_3 "pictureName_3.jpg"
// IP CAM Host
const char *IP_CAM_RESOURCE = "rtsp://192.168.1.24:554/1/h264major";

int main(int argc, char **argv)
{
    RTSPResource resource;
    int ret;

    ret = openIpCam(IP_CAM_RESOURCE, &resource);
    if (ret == 0)
        openVideoCodec(&resource);
    printf("***TAP start\n");
    if (ret == 0)
        ret = openIpCam(IP_CAM_RESOURCE, &resource);
    takeAPicture(&resource, TMP_FILE_1, 9, 0);
    printf("***TAP end\n");
    printf("***TAP start\n");
    if (ret == 0)
        ret = takeAPicture(&resource, TMP_FILE_2, 9, 5);
    printf("***TAP end\n");
    printf("***TAP start\n");
    if (ret == 0)
        ret = takeAPicture(&resource, TMP_FILE_3, 9, 5);
    printf("***TAP end\n");
    closeVideoCodec(&resource);
    closeIpCam(&resource);
}