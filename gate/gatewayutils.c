#include "gatewayutils.h"


//-1-> error 0->sucess, first peer to register 1-> sucess more than 1 peer register
//insert at the end of the list
int insert_server(servernode **head, char* address, int port){

  servernode *new_server = (servernode*) malloc(sizeof(servernode));
  if(new_server == NULL){
    printf("Error creating server\n");
    return(-1);
  }

  strcpy(new_server->address, address);
  new_server->port = port;
  new_server->next = NULL;
  new_server->available = 0;

  if(*head == NULL){
    *head = new_server;
    return(0);
  }

  servernode* aux = *head;
  servernode* aux2 = *head;
  //1 server in the list case
  if(aux->port == port && strcmp(aux->address,address) == 0)
    return(-1);

  while( aux->next != NULL){
    if(aux->port == port && strcmp(aux->address,address) == 0)
      return(-1);
    aux2 = aux;
    aux = aux->next;
  }
  aux->next = new_server;
  return(1);
}

//-1->error 0-> sucesssfull delete 1-> two or more peers on the list
int delete_server(servernode **head, char* address, int port){

  servernode *aux = *head;
  servernode *aux0= *head;


  //Empty list
  if(*head == NULL)
    return(-1);

  if(((*head)->next ==NULL) && (strcmp(address, (*head)->address)==0) && (port == (*head)->port)){
    *head=NULL;
    return(0);
  }

  while(aux !=NULL){
    if((strcmp(address, aux->address)==0) && (port == aux->port)){
      //server found
      aux0->next = aux->next;
      free(aux);
      return 0;
    }
    aux0 = aux;
    aux= aux->next;
  }
  //no server found
  return(-1);
}

//-1 error 0->sucess, type 0 -> add clients type 1-> remove clients
int modifyavail_server(servernode *head, char* address, int port, int newstate){

  servernode *aux = head;

  if(head == NULL)
    return(-1);

  while(aux !=NULL){
    if(strcmp(address, aux->address)==0 && port == aux->port){
      if(newstate== 0){
        aux->available++;
      }else{
        aux->available--;
      }
      return(0);
    }
    aux = aux->next;
  }
  //no server found
  return(-1);
}

//-1 -> error 0->sucess
int find_server(servernode *head, message_gw* mssg){

  //get a server a give to client
  if(head == NULL){
    printf("Empty list\n");
    return(-1);
  }

  servernode *aux = head;
  int lowOcupServer = aux->available;

  //search list for lower Ocup server
  while(aux != NULL){
    if(aux->available  < lowOcupServer){
      lowOcupServer = aux-> available;
    }
    aux = aux->next;
  }

  aux = head;

  while(aux != NULL){
    if(aux->available  <= lowOcupServer){
      mssg->type = 0;
      strcpy(mssg->address,aux->address);
      mssg->port = aux->port;
      return(0);
    }
    aux = aux->next;
  }

  //No server available
  mssg->type = 2;
  strcpy(mssg->address,"");
  mssg->port = 0;
  return(-1);
}


void print_server_list(servernode *head){
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

void clean_server_list(servernode *head){
  servernode * aux = head, *aux1 = head;
  while(aux != NULL){
    aux1 = aux;
    aux = aux->next;
    free(aux1);
  }
}
