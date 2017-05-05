#include "gatewayutils.h"

volatile sig_atomic_t flag = 0;
void my_handler(int sig){
  flag = 1;
}

servernode* head = NULL;

//-1-> error 0->sucess
//de momento esta a permitir duplicados change to do,..
//insert at the end of the list
int insert_server(char* address, int port){

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
    head = new_server;
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
int delete_server( char* address, int port){

  servernode *aux = head;
  servernode *aux0= head;

  //Empty list
  if(head == NULL)
    return(-1);

  if((head->next ==NULL) && (strcmp(address, head->address)==0) && (port == head->port)){
    head=NULL;
    return(0);
  }


  while(aux !=NULL){
    if((strcmp(address, aux->address)==0) && (port == aux->port)){
      //server found
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
int modifyavail_server( char* address, int port, int newstate){

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
int find_server( message_gw* mssg){
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

void print_server_list(){
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

void clean_server_list(){
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
        find_server(&auxm);
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
      insert_server(auxm.address,auxm.port);
      print_server_list();
    }else if(auxm.type == 3){
      modifyavail_server(auxm.address, auxm.port, 0);
      print_server_list();
    }else if(auxm.type == 4){
      modifyavail_server(auxm.address, auxm.port, 1);
      print_server_list();
    }else{
      delete_server(auxm.address, auxm.port);
      print_server_list();
    }
  }
  pthread_exit(NULL);
}

int main(int argc, char *argv[]){

    //signals
    struct sigaction sa;
    sa.sa_handler = &my_handler;
    sa.sa_flags = 0;
  	sigfillset(&sa.sa_mask);
    if( sigaction(SIGINT, &sa,0) == -1){
      perror("sigaction");
    }

    //var for sockets
    message m;
    message_gw auxm;
    struct sockaddr_in local_addr, local_addr0;
    struct sockaddr_in client_addr;
    socklen_t size_addr;
    int nbytes, clilen, newsockfd, portno;

    //threads
    pthread_t thread_id, thread_id0;

    if (argc < 5) {
         fprintf(stderr,"Usage: clientserveraddr clientserverport peersserveraddr peersserverport\n");
         exit(1);
     }

    /* create client server socket  */
    int sock_fd = socket(AF_INET, SOCK_DGRAM,0);
    if(sock_fd == -1){
      perror("socket: ");
      exit(-1);
    }

    //client server
    bzero((char *) &local_addr, sizeof(local_addr));
    portno = atoi(argv[2]);
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = inet_addr(argv[1]);
    local_addr.sin_port = htons(portno);

    int err = bind(sock_fd, (struct sockaddr *)&local_addr, sizeof(local_addr));
    if(err == -1){
      perror("bind: ");
      close(sock_fd);
      exit(-1);
    }

    /* create peers server socket  */
    int sock_peers = socket(AF_INET, SOCK_DGRAM,0);
    if(sock_peers == -1){
      perror("socket: ");
      close(sock_fd);
      exit(-1);
    }


    //peers server
    bzero((char *) &local_addr0, sizeof(local_addr0));
    portno = atoi(argv[4]);
    local_addr0.sin_family = AF_INET;
    local_addr0.sin_addr.s_addr = inet_addr(argv[3]);
    local_addr0.sin_port = htons(portno);

    err = bind(sock_peers, (struct sockaddr *)&local_addr0, sizeof(local_addr0));
    if(err == -1){
      perror("bind: ");
      close(sock_fd);
      close(sock_peers);
      exit(-1);
    }

    //erro a passar pointer, mas acho que tem que se mudar para shared variable a lista
    struct workerArgs *wa;
    //servernode* head = create_server_list();
    wa = malloc(sizeof(struct workerArgs));
    wa->sock = sock_fd;
    //wa->list = head;

    if(pthread_create(&thread_id, NULL, client_server,wa) != 0){
      perror("Could not create clients thread");
      close(sock_fd);
      close(sock_peers);
      exit(-1);
    }

    struct workerArgs *wa1;
    wa1 = malloc(sizeof(struct workerArgs));
    wa1->sock = sock_peers;
    //wa1->list = head;

    if(pthread_create(&thread_id0, NULL, peers_server,wa1) != 0){
      perror("Could not create peers thread");
      close(sock_fd);
      close(sock_peers);
      exit(-1);
    }


    while(1){

      //best way to shut down the threads ????
      //ctrl-c pressed!
        if(flag ==1){

          close(sock_fd);
          close(sock_peers);
          int s = pthread_cancel(thread_id0);
          if(s != 0) perror("server thread cancel: ");
          s = pthread_cancel(thread_id);
          if(s != 0) perror("peers thread cancel: ");
          free(wa);
          free(wa1);
          clean_server_list(head);
          exit(0);
        }

    }
    // never goes here!
    return 0;

}
