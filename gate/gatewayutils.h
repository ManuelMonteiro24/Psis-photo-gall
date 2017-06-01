#ifndef GATEWAYUTILS
#define GATEWAYUTILS

#define MAX_FILE_SIZE 100000 //bytes
#define MAX_WORD_SIZE 20 //bytes

#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <signal.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <pthread.h>


typedef struct workerArgs{
  int sock_fd;
  struct sockaddr_in sender_addr;
}workerArgs;

typedef struct message{
    int type;
    uint32_t identifier;
    char payload[MAX_WORD_SIZE];
    int update;
} Message;

//serverlist
typedef struct servernode{
  char address[20];
  int port;
  int available; //1 esta disponivel 0 nao esta disponivel
  struct servernode *next;
}servernode;

typedef struct message_gw{
   int type;
   char address[20];
   int port;
} message_gw;

int insert_server(servernode **head,char* address, int port);

int delete_server(servernode **head,char* address, int port);

int modifyavail_server(servernode *head,char* address, int port, int newstate);

int find_server(servernode *head,message_gw* mssg);

void print_server_list(servernode *head);

void clean_server_list(servernode *head);

#endif
