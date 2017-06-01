#ifndef SERVERUTILS
#define SERVERUTILS

#define BUFFERSIZE 100
#define MAX_FILE_SIZE 100000 //bytes
#define MAX_WORD_SIZE 20 //bytes

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <netinet/in.h>
#include <errno.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <ctype.h>
#include <pthread.h>
#include <time.h>

typedef struct workerArgs{
  int gatesock;
  int clisock;
  char address[20], port[20];
  struct sockaddr_in gateway_addr;
}workerArgs;

typedef struct message{
    int type;
    uint32_t identifier;
    char payload[MAX_WORD_SIZE];
    int update;
} Message;

typedef struct photo{
  int type; // -1->disconnect 0->insert on list 1-> add keyword 2->search_by_keyword 3->delete photo 4->gallery_get_photo_name 5->gallery_get_photon
  uint32_t identifier;
  char name[MAX_WORD_SIZE];
  int numKw;
  struct keyword * key_header;
  //falta binary data
  struct photo *next;
}photo;

typedef struct keyword{
  char word[MAX_WORD_SIZE];
  struct keyword *next;
}keyword;

typedef struct message_gw{
   int type;
   char address[20];
   int port;
} message_gw;

struct identifier{
    uint32_t id;
    struct identifier *next;
};

photo* create_photo_list();

uint32_t add_photo(photo **head, char *name, uint32_t identifier, int update, char *file_bytes, int file_size);
int add_keyword(photo* head,uint32_t identifier, char *keyword);
int get_photo_by_keyword(photo* head, struct identifier **ids, char *keyword);
int delete_photo(photo** head, uint32_t identifier);
int gallery_get_photo_name(photo* head, uint32_t id_photo, char file_name[MAX_WORD_SIZE]);
int gallery_get_photo(photo* head, uint32_t id_photo, char *file_bytes, int * file_size);
int send_database(int updateSocket, photo *head, int numbPhotos);
int update_database(int updateSocket, photo **head);
int read_file(char file_name[MAX_WORD_SIZE], char *file_bytes, int *file_size);
void print_list(photo * head);
void gallery_clean_list(photo * head);
void keyword_clean_list(keyword * head);
void * handle_client(void * arg);
#endif
