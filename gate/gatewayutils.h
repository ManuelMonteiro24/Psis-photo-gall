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
  //pthread_mutex_t lock; for whatt''???'??'
}servernode;

servernode* create_server_list();


int insert_server(servernode **head,char* address, int port);

int delete_server(servernode **head,char* address, int port);

int modifyavail_server(servernode *head,char* address, int port, int newstate);

int find_server(servernode *head,message_gw* mssg);

void print_server_list(servernode *head);

void clean_server_list(servernode *head);
/*
void * client_server(void * arg);

void * peers_server(void * arg);
*/
#endif
