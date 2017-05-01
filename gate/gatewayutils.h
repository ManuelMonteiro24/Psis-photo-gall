#ifndef GATEWAYUTILS
#define GATEWAYUTILS

#include "storyserver.h"
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

//serverlist
typedef struct servernode{
  char address[20];
  int port;
  int available; //1 esta disponivel 0 nao esta disponivel
  struct servernode *next;
}servernode;

void insert(char* address, int port);

void delete(char* address, int port);

void modifyavail(char* address, int port, int newstate);

void printlist();

void cleanlist();

void * client_server(void * arg);

void * peers_server(void * arg);

#endif
