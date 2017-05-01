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
#pragma pack(0)
