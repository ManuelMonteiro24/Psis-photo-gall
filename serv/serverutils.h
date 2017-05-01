#ifndef SERVERUTILS
#define SERVERUTILS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <netinet/in.h>
#include <errno.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "storyserver.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <ctype.h>
#include <pthread.h>

typedef struct workerArgs{
  int gatesock;
  int clisock;
  struct sockaddr_in gateway_addr;
  char address[20], port[20];
}workerArgs;

typedef struct photo{
  uint32_t identifier;
  char name[20];
  //falta binary data
  struct photo *next;
} photo;

typedef struct keyword{
  char name[20];
  struct keyword *next;
}keyword;

void * handle_client(void * arg);

#endif
