#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>

#include "rest.h"

#define TIMEOUT 5.0
#define USER_AGENT "RPI4B"
const char *BOUNDARY = "xxBOUNDARYxx";

int receiveRespose(int server);

int multiPartBody(unsigned char **pBuffer,
                  const notificationStruct *notificationBody)
{
    FILE *file;
    unsigned long fileLen;
    const int boundaryLen = strlen(BOUNDARY);
    unsigned char *buffer;

    // Open file
    file = fopen(notificationBody->image, "rb");
    if (!file)
    {
        fprintf(stderr, "Unable to open file %s", notificationBody->image);
        return (0);
    }

    // Get file length
    fseek(file, 0, SEEK_END);
    fileLen = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Allocate memory
    buffer = (char *)malloc(fileLen + 512);
    if (!buffer)
    {
        fprintf(stderr, "Memory error!");
        fclose(file);
        return 0;
    }

    sprintf(buffer, "--%s\r\n", BOUNDARY);
    sprintf(buffer + strlen(buffer), "Content-Disposition: form-data; name=\"title\"\r\n\r\n");
    sprintf(buffer + strlen(buffer), "%s\r\n", notificationBody->title);
    sprintf(buffer + strlen(buffer), "--%s\r\n", BOUNDARY);
    sprintf(buffer + strlen(buffer), "Content-Disposition: form-data; name=\"body\"\r\n\r\n");
    sprintf(buffer + strlen(buffer), "%s\r\n", notificationBody->body);
    sprintf(buffer + strlen(buffer), "--%s\r\n", BOUNDARY);
    sprintf(buffer + strlen(buffer), "Content-Disposition: form-data; name=\"image\"; filename=\"%s\"\r\n", notificationBody->image);
    sprintf(buffer + strlen(buffer), "Content-Type: image/jpeg\r\n\r\n");
    int len = strlen(buffer);

    fread(buffer + len, fileLen, sizeof(unsigned char), file);
    fclose(file);
    len = len + fileLen;
    sprintf(buffer + len, "\r\n--%s--\r\n", BOUNDARY);
    len = len + boundaryLen + 8;

    *pBuffer = buffer;

    return (len);
}

void sendPostMultipart(int socket, const char *hostname,
                       const char *port,
                       const char *path,
                       char *jwt,
                       const notificationStruct *notificationBody)
{
    char header[1024];
    unsigned char *buffer = NULL;
    int len;

    len = multiPartBody(&buffer, notificationBody);

    sprintf(header, "POST /%s HTTP/1.1\r\n", path);
    sprintf(header + strlen(header), "Host: %s:%s\r\n", hostname, port);
    sprintf(header + strlen(header), "Connection: close\r\n");
    sprintf(header + strlen(header), "User-Agent: %s\r\n", USER_AGENT);
    sprintf(header + strlen(header), "Content-Type: multipart/form-data; boundary=%s\r\n", BOUNDARY);
    sprintf(header + strlen(header), "Authorization: Bearer %s\r\n", jwt);
    sprintf(header + strlen(header), "Content-Length: %d\r\n", len);
    sprintf(header + strlen(header), "\r\n");

    printf("Sent Headers:\n%s", header);

    send(socket, header, strlen(header), 0);
    send(socket, buffer, len, 0);

    free(buffer);
}

void sendPost(int socket, const char *hostname, const char *port, const char *path, char *jwt, char *json)
{
    char buffer[2048];

    sprintf(buffer, "POST /%s HTTP/1.1\r\n", path);
    sprintf(buffer + strlen(buffer), "Host: %s:%s\r\n", hostname, port);
    sprintf(buffer + strlen(buffer), "Connection: close\r\n");
    sprintf(buffer + strlen(buffer), "User-Agent: %s\r\n", USER_AGENT);
    sprintf(buffer + strlen(buffer), "Content-Type: application/json\r\n");
    sprintf(buffer + strlen(buffer), "Authorization: Bearer %s\r\n", jwt);
    sprintf(buffer + strlen(buffer), "Content-Length: %d\r\n", strlen(json));
    sprintf(buffer + strlen(buffer), "\r\n");
    sprintf(buffer + strlen(buffer), "%s\r\n", json);

    send(socket, buffer, strlen(buffer), 0);
}

int connectToHost(const char *hostname, const char *port)
{
    printf("Configuring remote address...\n");
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    struct addrinfo *peer_address;
    if (getaddrinfo(hostname, port, &hints, &peer_address))
    {
        fprintf(stderr, "getaddrinfo() failed. (%d)\n", errno);
        exit(1);
    }

    printf("Remote address is: ");
    char address_buffer[100];
    char service_buffer[100];
    getnameinfo(peer_address->ai_addr, peer_address->ai_addrlen,
                address_buffer, sizeof(address_buffer),
                service_buffer, sizeof(service_buffer),
                NI_NUMERICHOST);
    printf("%s %s\n", address_buffer, service_buffer);

    printf("Creating socket...\n");
    int server;
    server = socket(peer_address->ai_family,
                    peer_address->ai_socktype, peer_address->ai_protocol);
    if (server < 0)
    {
        fprintf(stderr, "socket() failed. (%d)\n", errno);
        exit(1);
    }

    printf("Connecting...\n");
    if (connect(server,
                peer_address->ai_addr, peer_address->ai_addrlen))
    {
        fprintf(stderr, "connect() failed. (%d)\n", errno);
        exit(1);
    }
    freeaddrinfo(peer_address);

    printf("Connected.\n\n");

    return server;
}

int restPostJSON(const char *hostname, const char *port, const char *path, char *json, char *jwt)
{

    int server = connectToHost(hostname, port);

    sendPost(server, hostname, port, path, jwt, json);

    receiveRespose(server);
}

void restPostMultipart(const char *hostname, const char *port, const char *path, const notificationStruct *notificationBody, char *jwt)
{
    int socket;
    socket = connectToHost(hostname, port);
    sendPostMultipart(socket, hostname, port, path, jwt, notificationBody);
    receiveRespose(socket);
}

int receiveRespose(int server)
{

    const clock_t start_time = clock();

#define RESPONSE_SIZE 32768
    char response[RESPONSE_SIZE + 1];
    char *p = response, *q;
    char *end = response + RESPONSE_SIZE;
    char *body = 0;

    enum
    {
        length,
        chunked,
        connection
    };
    int encoding = 0;
    int remaining = 0;

    while (1)
    {

        if ((clock() - start_time) / CLOCKS_PER_SEC > TIMEOUT)
        {
            fprintf(stderr, "timeout after %.2f seconds\n", TIMEOUT);
            return 1;
        }

        if (p == end)
        {
            fprintf(stderr, "out of buffer space\n");
            return 1;
        }

        fd_set reads;
        FD_ZERO(&reads);
        FD_SET(server, &reads);

        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 200000;

        if (select(server + 1, &reads, 0, 0, &timeout) < 0)
        {
            fprintf(stderr, "select() failed. (%d)\n", errno);
            return 1;
        }

        if (FD_ISSET(server, &reads))
        {
            int bytes_received = recv(server, p, end - p, 0);
            if (bytes_received < 1)
            {
                if (encoding == connection && body)
                {
                    printf("%.*s", (int)(end - body), body);
                }

                printf("\nConnection closed by peer.\n");
                break;
            }

            /*printf("Received (%d bytes): '%.*s'",
                    bytes_received, bytes_received, p);*/

            p += bytes_received;
            *p = 0;

            if (!body && (body = strstr(response, "\r\n\r\n")))
            {
                *body = 0;
                body += 4;

                printf("Received Headers:\n%s\n", response);

                q = strstr(response, "\nContent-Length: ");
                if (q)
                {
                    encoding = length;
                    q = strchr(q, ' ');
                    q += 1;
                    remaining = strtol(q, 0, 10);
                }
                else
                {
                    q = strstr(response, "\nTransfer-Encoding: chunked");
                    if (q)
                    {
                        encoding = chunked;
                        remaining = 0;
                    }
                    else
                    {
                        encoding = connection;
                    }
                }
                printf("\nReceived Body:\n");
            }

            if (body)
            {
                if (encoding == length)
                {
                    if (p - body >= remaining)
                    {
                        printf("%.*s", remaining, body);
                        break;
                    }
                }
                else if (encoding == chunked)
                {
                    do
                    {
                        if (remaining == 0)
                        {
                            if ((q = strstr(body, "\r\n")))
                            {
                                remaining = strtol(body, 0, 16);
                                if (!remaining)
                                    goto finish;
                                body = q + 2;
                            }
                            else
                            {
                                break;
                            }
                        }
                        if (remaining && p - body >= remaining)
                        {
                            printf("%.*s", remaining, body);
                            body += remaining + 2;
                            remaining = 0;
                        }
                    } while (!remaining);
                }
            } // if (body)
        }     // if FDSET
    }         // end while(1)
finish:

    printf("\nClosing socket...\n");
    close(server);

    printf("Finished.\n");
    return 0;
}
