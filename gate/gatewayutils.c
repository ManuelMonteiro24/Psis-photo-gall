#include "gatewayutils.h"

//de momento esta a permitir duplicados
void insert(char* address, int port){

  servernode *aux = (servernode*) malloc(sizeof(servernode));

  strcpy(aux->address, address);
  aux->port = port;
  aux->next = head;
  aux->available = 1;

  head = aux;
  countservers++;
}

//1 -> sucesssfull delete
void delete(char* address, int port){

  servernode *aux = head;
  servernode *aux0= head;

  if(head == NULL)
    return;

  if(countservers == 1){
    head = NULL;
    return;
  }

  while(aux !=NULL){
    if( (strcmp(address, aux->address)==0) && (port == aux->port)){
      aux0->next = aux->next;
      countservers--;
      return;
    }
    aux0 = aux;
    aux= aux->next;

  }
  return;
}

void modifyavail(char* address, int port, int newstate){

  servernode *aux = head;

  if(head == NULL)
    return;

  while(aux !=NULL){
    if( strcmp(address, aux->address)==0 && port == aux->port){
      aux->available = newstate;
      return;
    }
    aux = aux->next;
  }
}

void printlist(){
  servernode * aux = head;

  while(aux != NULL){
    printf("address %s port %d available %d\n",aux->address, aux->port, aux->available);
    aux = aux->next;
  }
}

void cleanlist(){
  servernode * aux = head, *aux1;

  while(aux != NULL){
    aux1 = aux;
    aux = aux->next;
    free(aux1);
  }
}

void * client_server(void * arg){

  // get arguments
  int sock_fd = *(int*)arg;

  message_gw auxm;
  message m;
  struct sockaddr_in client_addr;
  servernode *auxpointer = NULL;
  socklen_t size_addr;
  int nbytes,flagaux =0;

  while(1){
    //read
    size_addr = sizeof(struct sockaddr_in);
    nbytes = recvfrom(sock_fd,&auxm,sizeof(struct message_gw),0,(struct sockaddr *) & client_addr, &size_addr);
    if( nbytes< 0) perror("Read: ");
    printf("received: %d %d %s %d\n", nbytes, auxm.type, auxm.address, auxm.port);

    //process message, process for server avalbility to do...
    if(auxm.type ==0){
      //case for no servers on list
      if(countservers ==0){
        auxm.type = 2;
      }else{
        //get a server a give to client
        auxpointer = head;
        flagaux = 0;
        while(flagaux == 0 && auxpointer != NULL){
          if(auxpointer->available ==1){
            auxm.type = 0;
            strcpy(auxm.address,auxpointer->address);
            auxm.port = auxpointer->port;
            flagaux = 1;
          }else{
            auxpointer = auxpointer->next;
          }
        }
        if(flagaux == 0){
          auxm.type =2 ;
          strcpy(auxm.address,"");
          auxm.port =0;
        }
      }
      //send answer for clients
      nbytes = sendto(sock_fd, &auxm, sizeof(struct message_gw), 0, ( struct sockaddr *) &client_addr, sizeof(client_addr));
      if( nbytes< 0) perror("Write: ");
       printf("replying %d bytes with address %s and port %d\n", nbytes , auxm.address, auxm.port);
    }
  }

  pthread_exit(NULL);
}

void * peers_server(void * arg){

  // get arguments
  int sock_fd = *(int*)arg;

  message_gw auxm;
  message m;
  int nbytes;

  while(1){
    //read
    nbytes = recv(sock_fd,&auxm,sizeof(struct message_gw),0);
    if( nbytes< 0) perror("Read: ");
    printf("received: %d %d %s %d\n", nbytes, auxm.type, auxm.address, auxm.port);

    //process message, process for server avalbility to do...
    if(auxm.type ==1){
      insert(auxm.address,auxm.port);
      printf("Server List:\n");
      printlist();
    }else if(auxm.type == 3){
      modifyavail(auxm.address, auxm.port, 0);
      printf("Server List:\n");
      printlist();
    }else if(auxm.type == 4){
      modifyavail(auxm.address, auxm.port, 1);
      printf("Server List:\n");
      printlist();
    }else{
      delete(auxm.address, auxm.port);
      printf("Server List:\n");
      printlist();
    }
  }
  pthread_exit(NULL);
}
