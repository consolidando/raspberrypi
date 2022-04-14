#ifndef REST_H   
#define REST_H 

typedef struct notificationStruct
{
    const char * title;
    const char * body;
    const char * image;
}notificationStruct;

int restPostJSON(const char *hostname,const char *port,const char *path, char *json, char *jwt);
void restPostMultipart(const char *hostname, const char *port,const char *path, const notificationStruct *notificationBody, char *jwt);



int connectToHost(const char *hostname, const char *port);
int receiveRespose(int server);

#endif 