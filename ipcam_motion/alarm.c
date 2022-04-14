#include "rtsp.h"
#include "rest.h"
#include "jwt_create.h"

const char *PROJECT_ID = "Project Id";
const char *DEVICE_ID = "MyFistDeviceID";
const char *RSA_PRIVATE_KEY_NAME = "rsa_private.pem";

#define TMP_FILE "pictureName.jpg"

// Pub/Sub host
const char *HOST_NAME = "192.168.1.36";
const char *HOST_PORT = "8080";
const char *HOST_RESOUCE = "resource/alarm/notification/image";

// IP CAM Host
const char *IP_CAM_RESOURCE = "rtsp://192.168.1.24:554/1/h264major";

const notificationStruct notificationBody =
    {
        .title = "IP CAM Alarm",
        .body = "Motion Detected",
        .image = TMP_FILE};

int alarmTakePictureAndSendNotification()
{
    RTSPResource resource;
    int ret;
    char *jwt;
    int socket;

    // opens ip cam resource
    ret = openIpCam(IP_CAM_RESOURCE, &resource);
    if (ret == 0)
        ret = openVideoCodec(&resource);
    // gets an image from the camera
    if (ret == 0)
        ret = takeAPicture(&resource, TMP_FILE, 9, 0);

    // releases resource
    closeVideoCodec(&resource);
    closeIpCam(&resource);

    if (ret == 0)
    {
        // creates authentification token for the device id
        jwt = jwtCreate(RSA_PRIVATE_KEY_NAME, PROJECT_ID, DEVICE_ID);

        // sends image notification to the subscribed mobiles
        restPostMultipart(HOST_NAME, HOST_PORT, HOST_RESOUCE, &notificationBody, jwt);
    }

    return(ret);
}