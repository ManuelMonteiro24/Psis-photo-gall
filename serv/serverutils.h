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
  int type; //0->insert on list 1-> add keyword 2->search_by_keyword 3->delete photo 4->gallery_get_photo_name 5->gallery_get_photo
  uint32_t identifier;
  char name[20];
  struct keyword * key_header;
  //falta binary data
  struct photo *next;
}photo;

typedef struct keyword{
  char name[20];
  struct keyword *next;
}keyword;

photo* create_photo_list();

int add_photo(photo** head,char *name);
int add_keyword(photo* head,uint32_t identifier, char *keyword);
int search_by_keyword(photo* head, uint32_t** id_photos, char *keyword);
int delete_photo(photo** head, uint32_t identifier);
int gallery_get_photo_name(photo* head, uint32_t id_photo,char **photo_name);
int gallery_get_photo(photo* head, uint32_t id_photo);
void print_list(photo * head);
void gallery_clean_list(photo * head);
void keyword_clean_list(keyword * head);
void * handle_client(void * arg);
#endif
