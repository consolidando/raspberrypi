#include <stddef.h>
#include <stdio.h>
#include <sys/time.h>

long long currentTime_ms() 
{
    struct timeval te; 
    gettimeofday(&te, NULL); // get current time
    long long milliseconds = te.tv_sec*1000LL + te.tv_usec/1000;    
    return milliseconds;
}

void printfTimeDifference()
{
    static long long begin = 0, total = 0;
    if(begin == 0) begin = currentTime_ms();

    long long aux = currentTime_ms() - begin;
    total += aux;
    printf("--- %lld ms of %lld\n", aux, total);       
    begin = currentTime_ms();
}

void printfTimeDifferenceTag(char *tag)
{
    printf("--- %s ", tag);
    printfTimeDifference();
}