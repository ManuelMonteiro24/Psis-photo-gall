#include "gatewayutils.h"


//-1-> error 0->sucess, first peer to register 1-> sucess more than 1 peer register
//insert at the end of the list
int insert_server(servernode **head, char* address, int port, message_gw *auxm, message_gw *auxm2){

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
    auxm->type = 0;
    auxm->port = 0; //means that doesnt have next peer
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
  //the next peer of the inserted one is the first of the list
  auxm->type = 0;
  strcpy(auxm->address, (*head)->address);
  auxm->port = (*head)->port;

  //information of the peer before of the inserted
  auxm2->type = 0;
  strcpy(auxm2->address, aux2->address);
  auxm2->port = aux2->port;
  return(1);
}

//-1->error 0-> sucesssfull delete 1-> two or more peers on the list
int delete_server(servernode **head, char* address, int port, message_gw *auxm, message_gw *auxm2){

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

      //auxm
      auxm->type = 0;
      aux = aux->next;
      //last node of list, and if we dont have only one node in list
      if(aux == NULL && (*head)->next !=NULL){
        strcpy(auxm->address,(*head)->address);
        auxm->port = (*head)->port;
      }

      //auxm2
      auxm2->type = 0;
      strcpy(auxm2->address,aux0->address);
      auxm2->port = aux0->port;
      return(2);
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

  // This form is hack change to a better one TO DO...
  // Ideia change to circular linked list???
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
  mssg->type =2 ;
  strcpy(mssg->address,"");
  mssg->port =0;
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
