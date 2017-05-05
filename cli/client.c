#include "clientapi.h"

//arg -- gatewayaddress gatewayport

int main(int argc, char *argv[]){

    char buffer[BUFFERSIZE];

    if (argc < 3) {
       fprintf(stderr,"usage gatewayhostname gatewayport\n");
       exit(1);
    }

    int sock_fd = gallery_connect(argv[1], atoi(argv[2]));
    if(sock_fd == 0){
      fprintf(stderr,"No peers available\n");
      exit(1);
    }
    if(sock_fd == -1){
      fprintf(stderr,"Gateway cannot be accessed\n");
      exit(1);
    }

    printf("Insert name of photo file to add to gallery: \n");
    fgets(buffer, BUFFERSIZE, stdin);

    int photo_id = gallery_add_photo(sock_fd,buffer);
    if(photo_id == 0){
      fprintf(stderr,"Photo insertion as failed!\n");
      exit(1);
    }

    close(sock_fd);
    exit(0);
}
