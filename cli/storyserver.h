#define SOCKET_NAME "sock_16"
#define MESSAGE_LEN 100

typedef struct message{
    char buffer[MESSAGE_LEN];
} message;


//serialize data to do....
#pragma pack(1)
typedef struct message_gw{
   int type;
   char address[20];
   int port;
} message_gw;

typedef struct photo{
  int type; //0->insert on list 1-> add keyword 2->search_by_keyword 3->delete photo 4->gallery_get_photo_name 5->gallery_get_photo
  uint32_t identifier;
  char name[20];
  struct keyword * key_header;
  //falta binary data
  struct photo *next;
}photo;
