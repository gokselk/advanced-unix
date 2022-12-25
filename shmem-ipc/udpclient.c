#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <time.h>
#define SERVER "127.0.0.1"
#define BUFSIZE 512
#define PORT 1234
void msleep(int us)
{
    int sleep_time = us*1000;
    usleep(sleep_time);
}
void die(char *s)
{
    perror(s);
    exit(1);
}

int main (int argc, char** argv)
{
    srand(time(NULL));
    struct sockaddr_in sain;
    int s; 
    char buf[BUFSIZE];
    if((s = socket(AF_INET, SOCK_DGRAM, 0))< 0)
    {
        perror("udpclient error");
        exit(1);
    }

    memset(&sain, 0, sizeof(sain));
    sain.sin_family = AF_INET;
    sain.sin_port = htons(atoi(argv[1]));
    if(inet_aton(SERVER, &sain.sin_addr) == 0)
    {
        fprintf(stderr, "inet_aton() failed\n");
        exit(1);
    }
    char mes1[10] = "abc";
    char mes2[10] = "abc";
    char mes3[10] = "abc";
    char mes[10]; 
    while(1)
    {
        int a = rand() % 3; 
        memset(mes, 0, 10);
        if ( a == 0)
        {
            strncpy(mes, mes1, 10);
        }
        else if(a == 1)
        {
            strncpy(mes, mes2, 10);
        }
        else 
        {
            strncpy(mes, mes3, 10);
        }

        if(sendto(s, mes, 10, 0, (struct sockaddr *) & sain, sizeof(sain)) == -1)
        {
            die("sendto()\n");
        }
        msleep(atoi(argv[3]));
    }
}
