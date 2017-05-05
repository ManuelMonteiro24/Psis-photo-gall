#include "clientapi.h"

int gallery_connect(char * host, in_port_t port){

  message_gw auxm;

  //struct sockaddr_in local_addr;
  struct sockaddr_in server_addr;
  struct sockaddr_in gateway_addr;
  int nbytes, portno;

  int sock_gt = socket(AF_INET, SOCK_DGRAM, 0);
  if(sock_gt == -1){
    perror("socket: ");
    return(-1); //gateway cannot be accessed
  }

  //contact with gateway
  bzero((char *) &gateway_addr, sizeof(gateway_addr));
  gateway_addr.sin_family = AF_INET;
  gateway_addr.sin_addr.s_addr = inet_addr(host);
  gateway_addr.sin_port = htons(port);

  //send checkin message to gateway
  auxm.type =0;
  nbytes = sendto(sock_gt, &auxm, sizeof(struct message_gw),0, (const struct sockaddr *) &gateway_addr, sizeof(gateway_addr));
  if(nbytes< 0){
    perror("Sending to gateway: ");
    close(sock_gt);
    return(-1); //gateway cannot be accessed
  }

  //rcv response from gateway, what do do if doesnt recv response???
  nbytes = recv(sock_gt, &auxm, sizeof(struct message_gw), 0);
  if(nbytes< 0){
    perror("Response from gateway: ");
    close(sock_gt);
    return(-1);
  }
  printf("received: %d %d %s %d\n", nbytes, auxm.type, auxm.address, auxm.port);
  close(sock_gt);

  //caso nao estiver nenhum server disponivel acabar aqui
  if(auxm.type == 2){
    return(0); //no peer available
  }

  int sock_serv = socket(AF_INET, SOCK_STREAM, 0);
  if(sock_serv == -1){
    perror("socket: ");
    return(0); //gateway cannot be accessed or -1?? cannot acess server
  }

  // server
  bzero((char *) &server_addr, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = inet_addr(auxm.address);
  server_addr.sin_port = htons(auxm.port);

  if(connect(sock_serv,(struct sockaddr *)&server_addr, sizeof(server_addr)) < 0){
    perror("Connect: ");
    close(sock_serv);
    return(0); //No peer available
  }else
    return (sock_serv);
}

uint32_t gallery_add_photo(int peer_socket, char *file_name){

  message m;
  int nbytes;

  //process photo file
  FILE * f =  fopen("file_name","r");
  if(f==NULL){
    perror("Photo file: ");
    return 0;
  }
  //get photo to do...
  printf("message: ");
  fgets(m.buffer, MESSAGE_LEN, stdin);

  /* send photo to do... */
  nbytes = write(peer_socket, m.buffer, MESSAGE_LEN);
  if(nbytes< 0){
    perror("Write: ");
    return(0);
  }
  printf("sent %d %s\n", nbytes, m.buffer);

  /* receive photo identifier to do... */
  nbytes = read(peer_socket, m.buffer, nbytes);
  if(nbytes< 0){
    perror("Read: ");
    return(0);
  }
  printf("received %d bytes : %s", nbytes,  m.buffer);

  //return photo idetifier to do...
  return(1);
}

int gallery_add_keyword(int peer_socket, uint32_t id_photo, char *keyword){
  char buffer[BUFFERSIZE];
  int nbytes;

  //send keyword and photoid to do...
  strcpy(buffer, keyword);
  nbytes = write(peer_socket,buffer, BUFFERSIZE);
  if(nbytes< 0){
    perror("Write: ");
    return(-1);
  }
  printf("sent %d %s\n", nbytes, m.buffer);

  /* receive confirmation to do... */
  nbytes = read(peer_socket, m.buffer, nbytes);
  if(nbytes< 0){
    perror("Read: ");
    return(-1);
  }
  printf("received %d bytes : %s", nbytes,  m.buffer);

  //return sucess or no photo found
  return(1);
}

int gallery_search_photo(int peer_socket, char * keyword,
     uint32_t ** id_photos){

       char buffer[BUFFERSIZE];
       int nbytes;

       //send keyword
       strcpy(buffer, keyword);
       nbytes = write(peer_socket,buffer, BUFFERSIZE);
       if(nbytes< 0){
         perror("Write: ");
         return(-1);
       }
       printf("sent %d %s\n", nbytes, m.buffer);

       /* receive photos identifier to do... */
       nbytes = read(peer_socket, m.buffer, nbytes);
       if(nbytes< 0){
         perror("Read: ");
         return(-1);
       }
       printf("received %d bytes : %s", nbytes,  m.buffer);

       //return number of photos found to do...
       return(1);
}
