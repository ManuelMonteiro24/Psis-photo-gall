#ifndef CLIENTAPI
#define CLIENTAPI

#define BUFFERSIZE 100

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <netinet/in.h>
#include <errno.h>
#include <netdb.h>
#include <arpa/inet.h>

//serialize data to do....
typedef struct message_gw{
   int type;
   char address[20];
   int port;
} message_gw;

typedef struct keyword{
  char name[20];
  struct keyword *next;
}keyword;

typedef struct photo{
  int type; //-1-> disconnect 0->insert on list 1-> add keyword 2->search_by_keyword 3->delete photo 4->gallery_get_photo_name 5->gallery_get_photo
  uint32_t identifier;
  char name[20];
  struct keyword * key_header;
  //falta binary data
  struct photo *next;
}photo;


int gallery_connect(char * host, in_port_t port);
uint32_t gallery_add_photo(int peer_socket, char *file_name);
int gallery_add_keyword(int peer_socket, uint32_t id_photo, char *keyword);
int gallery_search_photo(int peer_socket, char * keyword, uint32_t ** id_photos);
int gallery_delete_photo(int peer_socket, uint32_t id_photo);
int gallery_get_photo_name(int peer_socket, uint32_t id_photo, char **photo_name);
int gallery_get_photo(int peer_socket, uint32_t id_photo, char *file_name);
int gallery_disconnect(int peer_socket);

#endif
