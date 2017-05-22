#ifndef GATEWAYUTILS
#define GATEWAYUTILS

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

//serialize data to do....
typedef struct message_gw{
   int type; //0->next peer
   char address[20];
   int port;
} message_gw;

servernode* create_server_list();


int insert_server(servernode **head,char* address, int port, message_gw *auxm, message_gw *auxm2);

int delete_server(servernode **head,char* address, int port, message_gw *auxm, message_gw *auxm2);

int modifyavail_server(servernode *head,char* address, int port, int newstate);

int find_server(servernode *head,message_gw* mssg);

void print_server_list(servernode *head);

void clean_server_list(servernode *head);
/*
void * client_server(void * arg);

void * peers_server(void * arg);
*/
#endif
