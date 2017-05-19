#include "gatewayutils.h"

volatile sig_atomic_t flag = 0;
void my_handler(int sig){
  flag = 1;
}

servernode* head = NULL;

//only one mutex for all list now we only have problem in sync with the pointer
//to head that can be changed inserting or deleting the first node
//IS THIS THE BEST WAY??
pthread_mutex_t mutex;

//erro a passar pointer
void * client_server(void * arg){

  // get arguments
  int sock_fd = *(int*) arg;

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
      pthread_mutex_lock(&mutex);
      find_server(head,&auxm);
      pthread_mutex_unlock(&mutex);
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
    pthread_mutex_lock(&mutex);
    if(auxm.type ==1){
      insert_server(&head, auxm.address,auxm.port);
      print_server_list(head);
    }else if(auxm.type == 3){
      modifyavail_server(head,auxm.address, auxm.port, 0);
      print_server_list(head);
    }else if(auxm.type == 4){
      modifyavail_server(head,auxm.address, auxm.port, 1);
      print_server_list(head);
    }else{
      delete_server(&head,auxm.address, auxm.port);
      print_server_list(head);
    }
    pthread_mutex_unlock(&mutex);
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

    //Initiate mutex that protects list
    pthread_mutex_init(&mutex, NULL);

    if(pthread_create(&thread_id, NULL, client_server,(void*) &sock_fd) != 0){
      perror("Could not create clients thread");
      close(sock_fd);
      close(sock_peers);
      exit(-1);
    }


    if(pthread_create(&thread_id0, NULL, peers_server,(void*) &sock_peers) != 0){
      perror("Could not create peers thread");
      close(sock_fd);
      close(sock_peers);
      exit(-1);
    }


    while(1){

      //best way to shut down the threads ???? exit()=???
      //ctrl-c pressed!
        if(flag ==1){
          close(sock_fd);
          close(sock_peers);
          clean_server_list(head);
          pthread_mutex_destroy(&mutex);
          exit(0);
        }
    }
    // never goes here!
    return 0;

}
