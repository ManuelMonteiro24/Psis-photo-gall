#include "clientapi.h"

//-1 -> error 0 -> sucess
int gallery_disconnect(int peer_socket){
  return close(peer_socket);
}

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

  int nbytes;
  uint32_t id;
  struct photo photo_aux;
  Message msg;
  char *file_bytes; //char type is 1 byte long

  file_name[strlen(file_name) - 1] = 0;
  FILE *fd = fopen(file_name,"r+");
  if(fd == NULL){
    perror("Photo file ");
    return 0;
  }

  fseek(fd, 0, SEEK_END); //set stream pointer @fd to end-of-file
  long file_size = ftell(fd); //get fd current possition in stream
  file_bytes = malloc(file_size);

  msg.type = 0;
  strcpy(msg.payload, file_name);
  rewind(fd); //start reading file from the beginning
  fread(file_bytes, file_size, 1, fd);
  fclose(fd);

  nbytes = write(peer_socket, &msg, sizeof(msg));
  if(nbytes< 0){
    perror("Write: ");
    return(0);
  }
  printf("sent add photo msg (%d bytes) type %d\n", nbytes, msg.type);

  nbytes = write(peer_socket, file_bytes, file_size);
  if(nbytes < 0){
    perror("Write: ");
    return(0);
  }
  printf("sent photo (%d bytes) file_size %d\n", nbytes, (int) file_size);

  //receive photo identifier froms server
  nbytes = read(peer_socket, &id, sizeof(int));
  if(nbytes < 0){
    perror("Read photo id: ");
    return(0);
  }
  printf("photo id: %d\n", id);

  return id;

}

//returns 1->sucess 0->no photo with that iden -1->error
int gallery_delete_photo(int peer_socket, uint32_t id_photo){

  int nbytes, ret;
  struct photo photo_aux;
  Message msg;

  msg.type = 3;
  msg.identifier = id_photo;
  printf("PHOTO TO DELETE ID %d\n", id_photo);
  /* send message to do... */
  nbytes = write(peer_socket, &msg, sizeof(msg));
  if(nbytes < 0){
    perror("Write: ");
    return(-1);
  }
  printf("sent %d %d\n", nbytes, msg.type);

  /* receive confirmation to do... */
  nbytes = read(peer_socket, &ret, sizeof(int));
  if(nbytes < 0){
    perror("Read keyword ret: ");
    return(-1);
  }
  printf("received %d bytes : %d \n", nbytes, msg.type);

  // 1-> REmoval sucesssfull 0-> that photo isnt on the servers -1 -> error (duplicates)
  return ret;

}

//return 1 if sucess, -1 if error, 0 if no photo found
int gallery_add_keyword(int peer_socket, uint32_t id_photo, char *keyword){

  int nbytes, ret;
  struct photo photo_aux;
  Message msg;

  msg.type = 1;
  msg.identifier = id_photo;
  strcpy(msg.payload, keyword);

  /* send message to do... */
  nbytes = write(peer_socket, &msg, sizeof(msg));
  if(nbytes < 0){
    perror("Write add keyword: ");
    return(-1);
  }
  printf("sent %d %d\n", nbytes, msg.type);

  /* receive confirmation to do... */
  nbytes = read(peer_socket, &ret, sizeof(int));
  if(nbytes < 0){
    perror("Read : ");
    return(-1);
  }

  // 1->Adding sucesssfull 0-> that photo isnt on the servers -1 -> error (duplicates or mem)
  return ret;

}

//ERROR HERE OR ON gallery_add_keyword
int gallery_search_photo(int peer_socket, char * keyword, uint32_t **photos_id){

       int nbytes, photosNumb, it;
       Message msg;

       msg.type = 2;
       strcpy(msg.payload, keyword);
       nbytes = write(peer_socket, &msg, sizeof(msg));
       if(nbytes< 0){
         perror("Write: ");
         return(-1);
       }
       printf("sent %d %s\n", nbytes, msg.payload);

       //receive amount of photos matched with sent keyword
       nbytes = read(peer_socket, &photosNumb, sizeof(int));
       if(nbytes< 0){
         perror("Read: ");
         return(-1);
       }
       printf("numb: %d\n", photosNumb);
       if(photosNumb != 0){
         *photos_id = (uint32_t *) calloc(photosNumb, sizeof(int));
         for(it = 0; it < photosNumb; it++){
           nbytes = read(peer_socket, &(*photos_id)[it], sizeof(int));
           if(nbytes < 0){
             perror("Read: ");
             return(-1);
           }
         }
       }
       return photosNumb;
}

//returns 1-> photo exists 0-> photo doesnt exist  -1-> error
int gallery_get_photo_name(int peer_socket, uint32_t id_photo, char **photo_name){

  char buffer[BUFFERSIZE];
  int nbytes, ret;
  struct photo photo_aux;
  Message msg;

  //send photo identifier
  msg.type = 4;
  msg.identifier = id_photo;

  nbytes = write(peer_socket, &msg, sizeof(msg));
  if(nbytes < 0){
    perror("Write: ");
    return(-1);
  }
  printf("sent %d %d\n", nbytes, msg.type);

  nbytes = read(peer_socket, &ret, 4);
  if(nbytes < 0){
    perror("Read: ");
    return(-1);
  }

  if(ret != 1)
    return ret;

  *photo_name = (char *) calloc(MAX_WORD_SIZE, sizeof(char));
  nbytes = read(peer_socket, *photo_name, MAX_WORD_SIZE);
  if(nbytes < 0){
    perror("Read: ");
    return(-1);
  }

  return ret;
}

//returns 1-> photo downloaded sucesssfully 0->photo doenst exists -1->error
int gallery_get_photo(int peer_socket, uint32_t id_photo, char *file_name){

  int nbytes, ret;
  Message msg;
  long file_size;

  char file_bytes[MAX_FILE_SIZE]; //char type is 1 byte long

  msg.type = 5;
  msg.identifier = id_photo;

  /* send message with identifier of photo to receive */
  nbytes = write(peer_socket, &msg, sizeof(msg));
  if(nbytes< 0){
    perror("Write: ");
    return(-1);
  }

  nbytes = read(peer_socket, &ret, 4);
  if(nbytes < 0){
    perror("Read: ");
    return(-1);
  }

  if(ret != 1){
    printf("dif 1\n");
    return ret;
  }
  /* receive message with photo size*/
  nbytes = read(peer_socket, &file_size, sizeof(file_size));
  if(nbytes< 0){
    perror("Read: ");
    return(-1);
  }

  /* receive message with photo*/
  nbytes = read(peer_socket, file_bytes, file_size);
  if(nbytes< 0){
    perror("Read: ");
    return(-1);
  }
  printf("recv photo (%d bytes)\n", nbytes);

  //generate file on disk to save photo data
  FILE *new_file;
  new_file = fopen(file_name, "w+");
  if(new_file == NULL){
    perror("RCV PHOTO: ");
    return -1;
  }

  //file not receving bytes??
  printf("bytes written: %d\n", nbytes);
  fwrite(file_bytes, nbytes, 1, new_file);
  fclose(new_file);

  return ret;
}
