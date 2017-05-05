#include "gatewayutils.h"

//creates empty list
servernode* create_server_list(){
  return NULL;
}

//-1-> error 0->sucess
//de momento esta a permitir duplicados change to do,..
//insert at the end of the list
int insert_server(servernode* head, char* address, int port){

  servernode *new_server = (servernode*) malloc(sizeof(servernode));
  if(new_server == NULL){
    printf("Error creating server\n");
    return(-1);
  }

  strcpy(new_server->address, address);
  new_server->port = port;
  new_server->next = NULL;
  new_server->available = 1;

  if(head == NULL){
    head=new_server;
    return(0);
  }

  servernode* aux = head;
  while( aux->next != NULL){
    aux = aux->next;
  }
  aux->next = new_server;
  return(0);
}

//-1->error 0-> sucesssfull delete
int delete_server(servernode* head, char* address, int port){

  servernode *aux = head;
  servernode *aux0= head;

  //Empty list
  if(head == NULL)
    return(-1);

  while(aux !=NULL){
    if( (strcmp(address, aux->address)==0) && (port == aux->port)){
      //server foun
      aux0->next = aux->next;
      return(0);
    }
    aux0 = aux;
    aux= aux->next;

  }
  //no server found
  return(-1);
}

//-1 error 0->sucess
int modifyavail_server(servernode* head, char* address, int port, int newstate){

  servernode *aux = head;

  if(head == NULL)
    return(-1);

  while(aux !=NULL){
    if(strcmp(address, aux->address)==0 && port == aux->port){
      aux->available = newstate;
      return(0);
    }
    aux = aux->next;
  }
  //no server found
  return(-1);
}

//-1 error 0->sucess
int find_server(servernode* head, message_gw* mssg){
  //get a server a give to client
  servernode *aux = head;
  int flagaux = 0;
  while(flagaux == 0 && aux != NULL){
    if(aux->available ==1){
      mssg->type = 0;
      strcpy(mssg->address,aux->address);
      mssg->port = aux->port;
      return(0);
    }
    aux = aux->next;
  }
  //No server available
  mssg->type =2 ;
  strcpy(mssg->address,"");
  mssg->port =0;
  return(-1);
}

void print_server_list(servernode* head){
  servernode * aux = head;
  printf("Server List:");
  if(aux == NULL){
    printf("Empty list\n");
  }else{
    while(aux != NULL){
      printf(" address %s port %d available %d\n",aux->address, aux->port, aux->available);
      aux = aux->next;
    }
  }
}

void clean_server_list(servernode* head){
  servernode * aux = head, *aux1 = head;

  while(aux != NULL){
    aux1 = aux;
    aux = aux->next;
    free(aux1);
  }
}

//erro a passar pointer
void * client_server(void * arg){

  // get arguments
  struct workerArgs *wa;
  wa = (struct workerArgs*) arg;
  int sock_fd = wa->sock;
  servernode* head;
  head = wa->list;

  message_gw auxm;
  message_gw* mssg_pointer;
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
        *mssg_pointer = auxm;
        find_server(head,mssg_pointer);
    }
      //send answer for clients
      nbytes = sendto(sock_fd, &auxm, sizeof(struct message_gw), 0, ( struct sockaddr *) &client_addr, sizeof(client_addr));
      if( nbytes< 0) perror("Write: ");
       printf("replying %d bytes with address %s and port %d\n", nbytes , auxm.address, auxm.port);
    }

  pthread_exit(NULL);
}

void * peers_server(void * arg){

  // get arguments
  struct workerArgs *wa;
  wa = (struct workerArgs*) arg;
  int sock_fd = wa->sock;
  servernode *head;
  head = wa->list;

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
      printf("Server insertion: %d\n", insert_server(head,auxm.address,auxm.port));
      print_server_list(head);
    }else if(auxm.type == 3){
      modifyavail_server(head, auxm.address, auxm.port, 0);
      print_server_list(head);
    }else if(auxm.type == 4){
      modifyavail_server(head,auxm.address, auxm.port, 1);
      print_server_list(head);
    }else{
      delete_server(head,auxm.address, auxm.port);
      print_server_list(head);
    }
  }
  pthread_exit(NULL);
}
