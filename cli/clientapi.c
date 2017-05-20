#include "clientapi.h"

//-1 -> error 0 -> sucess
int gallery_disconnect(int peer_socket){

  int nbytes;
  struct photo photo_aux;

  photo_aux.type = -1;

  nbytes = write(peer_socket, &photo_aux, sizeof(photo));
  if(nbytes< 0){
    perror("Write: ");
    return(-1);
  }
  printf("sent %d message type:%d\n", nbytes, photo_aux.type);

  nbytes = read(peer_socket, &photo_aux, sizeof(photo));
  if(nbytes< 0){
    perror("Read: ");
    return(-1);
  }
  printf("received %d message type:%d\n", nbytes, photo_aux.type);

  if(photo_aux.type == -1){
    close(peer_socket);
    return(0);
  }else{
    //error
    return(-1);
  }
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

  message m;
  int nbytes;
  struct photo photo_aux;

  //process photo file
  //error opening file dunno why???????????????????
  /*
  FILE *f=fopen(file_name,"r");
  if(f==NULL){
    perror("Photo file: ");
    return 0;
  }
  fclose(f);
  */
  //get photo to do...
  photo_aux.type = 0;
  strcpy(photo_aux.name,file_name);

  /* send photo to do... */
  nbytes = write(peer_socket, &photo_aux, sizeof(photo));
  if(nbytes< 0){
    perror("Write: ");
    return(0);
  }
  printf("sent %d %s\n", nbytes, m.buffer);

  /* receive photo identifier to do... */
  nbytes = read(peer_socket, &photo_aux, sizeof(photo));
  if(nbytes< 0){
    perror("Read: ");
    return(0);
  }
  printf("received %d bytes : %s \n", nbytes,  m.buffer);

  //return photo idetifier to do...
  return(photo_aux.identifier);
}


int gallery_delete_photo(int peer_socket, uint32_t id_photo){

  message m;
  int nbytes;
  struct photo photo_aux;

  photo_aux.type = 3;
  photo_aux.identifier = id_photo;

  /* send message to do... */
  nbytes = write(peer_socket, &photo_aux, sizeof(photo));
  if(nbytes< 0){
    perror("Write: ");
    return(-1);
  }
  printf("sent %d %s\n", nbytes, m.buffer);

  /* receive confirmation to do... */
  nbytes = read(peer_socket, &photo_aux, sizeof(photo));
  if(nbytes< 0){
    perror("Read: ");
    return(-1);
  }
  printf("received %d bytes : %s \n", nbytes,  m.buffer);

  // 1-> REmoval sucesssfull 0-> that photo isnt on the servers -1 -> error (duplicates)
  return photo_aux.type;

}

//TEST TO DO!
//return 1 if sucess, -1 if error, 0 if no photo found
int gallery_add_keyword(int peer_socket, uint32_t id_photo, char *keyword){

  message m;
  int nbytes;
  struct photo photo_aux;

  photo_aux.type = 1;
  photo_aux.identifier = id_photo;
  //right now i am using name to pass it but we need to change it
  strcpy(photo_aux.name, keyword);

  /* send message to do... */
  nbytes = write(peer_socket, &photo_aux, sizeof(photo));
  if(nbytes< 0){
    perror("Write: ");
    return(-1);
  }
  printf("sent %d %s\n", nbytes, m.buffer);

  /* receive confirmation to do... */
  nbytes = read(peer_socket, &photo_aux, sizeof(photo));
  if(nbytes< 0){
    perror("Read: ");
    return(-1);
  }

  //ERROR HERE NOT RECVING 1 FROM server, the client is reciving 0 instead

  printf("received %d bytes message type: %d\n", nbytes,  photo_aux.type);

  // 1->Adding sucesssfull 0-> that photo isnt on the servers -1 -> error (duplicates or mem)
  return photo_aux.type;

}

int gallery_search_photo(int peer_socket, char * keyword,
     uint32_t ** id_photos){

       char buffer[BUFFERSIZE];
       int nbytes;

       /*
       //send keyword
       strcpy(buffer, keyword);
       nbytes = write(peer_socket,buffer, BUFFERSIZE);
       if(nbytes< 0){
         perror("Write: ");
         return(-1);
       }
       printf("sent %d %s\n", nbytes, buffer);

       // receive photos identifiers and photo count to do...
       nbytes = read(peer_socket, buffer, nbytes);
       if(nbytes< 0){
         perror("Read: ");
         return(-1);
       }
       printf("received %d bytes : %s", nbytes,  buffer);

       */

       // after photos identifiers and photo count received TO DO...
       int photos_found = 4;

       //NO photos contain the provided keyword
       if(photos_found == 0){
         return 0;
       }

       //create output vector
       *id_photos = (uint32_t*)calloc(photos_found, sizeof(uint32_t));
       if(id_photos == NULL){
         printf("Error creating id_photos vector\n");
         return -1;
       }

       //insert photos_id in id_photos to do... ERROR HERE
        /*for(int i=0; i<photos_found;i++){
         *id_photos[1] = 0;
         printf("%d", id_photos[i]);
        }
        */

       //return number of photos found
       return(photos_found);
}
