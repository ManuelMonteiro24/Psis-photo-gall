  #include "serverutils.h"

//arg serveraddr(127.0.0.1) serverport(51717) gatewayaddress gatewayport
volatile sig_atomic_t flag = 0;
void my_handler(int sig){
  flag = 1;
}

//gateway_addr shared by all threads
struct sockaddr_in gateway_addr;

//Next peer scheme vars
struct sockaddr_in next_peer_addr;
socklen_t size_addr;

//NOT USING RIGTH NOW!
pthread_mutex_t next_peer_lock;

//Peer photo list thats going to be shared between the threads
photo* head = NULL;

// NOT THE OPTIMAL WAY TO SYNC BUT FOR NOW workerArgs
// CHANGE TO a mutex lock in each struct of the list TO DO..
pthread_mutex_t mutex;

void * peers_sync(void * arg){

  int sock_gt = *(int*) arg;

  message_gw auxm;

  int nbytes;

  while(1){

    //NOT WORKING AND WE SHOULD DISCUSS HOW WE GOING TO DO THIS, we bind a udp server for each peer???

    nbytes = recv(sock_gt, &auxm, sizeof( struct message_gw),0);
    if( nbytes< 0) perror("Receiving from gateway: ");
    printf("received %d with address %s and port %d\n", nbytes , auxm.address, auxm.port);

    //TO DO...
    //trough this thread make the heartbeating connection to the gateway
    //to check when if it is online
    //through here made a send to another peer of all those threads that we doesnt have
    //when he regists
    //RCV PHOTOS OR PHOTO TO ADD UPLOAD OR DELETE FROM ANOTHER PEER

    if(auxm.type == 0){
      next_peer_addr.sin_addr.s_addr = inet_addr(auxm.address);
      next_peer_addr.sin_port = htons(auxm.port);
    }
  }
  //never goes here
}

void * handle_client(void * arg){

  // get arguments
  struct workerArgs *wa;
  wa = (struct workerArgs*) arg;

  message_gw auxm;
  int nbytes, sock_gt, newsockfd;
  uint32_t ret;
  sock_gt = wa->gatesock;
  newsockfd = wa->clisock;

  struct photo photo_aux;
  Message msg;
  char file_bytes[MAX_FILE_SIZE], file_name[MAX_WORD_SIZE];
  struct identifier *ids, *aux_id, *rm;
  ids = aux_id = NULL;

  pthread_detach(pthread_self());

  //if accept was sucesssfull communicate to gateway to change state
  auxm.type = 3;
  strcpy(auxm.address, wa->address);
  auxm.port = atoi(wa->port);
  nbytes = sendto(sock_gt, &auxm, sizeof( struct message_gw),0, (const struct sockaddr *) &gateway_addr, sizeof(gateway_addr));
  if(nbytes< 0){
    perror("Sending to gateway: ");
    free(wa);
    pthread_exit(NULL);
  }

  while(1){
      // read message
      memset(&msg, -1, sizeof(msg));
      nbytes = read(newsockfd, &msg, sizeof(msg));
      if( nbytes < 0 ){
        perror("Received message: ");
        free(wa);
        pthread_exit(NULL);
      } else if(nbytes == 0){
        printf("Connection closed by the client..\n");
        free(wa);
        pthread_exit(NULL);
      }

      /*Process message:
      type 0: add photo,
      type 1: add keyword,
      type 2: search by keyword,
      type 3: delete photo,
      type 4: get photo name by id,
      type 5: ??
      */
      pthread_mutex_lock(&mutex);
      switch(msg.type){

        case 0:
          nbytes = read(newsockfd, file_bytes, MAX_FILE_SIZE); //read file
          printf("received photo with %d bytes\n", nbytes);
          if( nbytes < 0 ){
            perror("Read: ");
            free(wa);
            pthread_exit(NULL);
          }
          ret = add_photo(&head, msg.payload, file_bytes, nbytes);
          print_list(head);
          //UPDATE ALL OTHER PEERS
          break;

        case 1:
          ret = add_keyword(head, msg.identifier, msg.payload);
          print_list(head);
          //UPDATE ALL OTHER PEERS
          break;

        case 2:
          ret = get_photo_by_keyword(head, &ids, msg.payload);
          nbytes = write(newsockfd, &ret, sizeof(int)); //send number of found photos to client
          if( nbytes < 0 ){
            perror("Write ret type 2(1): ");
            free(wa);
            pthread_exit(NULL);
          }
          for(aux_id = rm = ids; aux_id != NULL; rm = aux_id, aux_id = aux_id->next){
              nbytes = write(newsockfd, &aux_id->id, sizeof(int)); //send return signal to client
              if( nbytes < 0 ){
                perror("Write ret type 2(1): ");
                free(wa);
                pthread_exit(NULL);
              }
              if(aux_id != ids){
                free(rm);
              }
          }
          free(rm); //free last element
          ids = NULL;
          break;

        case 3:
          ret = delete_photo(&head, msg.identifier);
          print_list(head);
          //SEND UPDATE TO THE NEXT PEER TO DO...
          break;

        case 4:
          ret = gallery_get_photo_name(head, msg.identifier, file_name);
          nbytes = write(newsockfd, file_name, MAX_WORD_SIZE); //send return signal to client
          if( nbytes < 0 ){
            perror("Write type 4 ");
            free(wa);
            pthread_exit(NULL);
          }
          memset(file_name, 0, MAX_WORD_SIZE);
          break;

        case 5:
          ret = gallery_get_photo(head, msg.identifier, &photo_aux);
          break;

        default:
          printf("ERROR: received message type matched by default\n");
          //ERROR ON EXIT TO RESOLVE
          //disconnect client and close thread
          free(wa);
          pthread_exit(NULL);
          break;
      }

      if(msg.type <= 5 && msg.type != 2){
        nbytes = write(newsockfd, &ret, sizeof(int)); //send return signal to client
        if( nbytes < 0 ){
          perror("Write ret: ");
          free(wa);
          pthread_exit(NULL);
        }
      }

      pthread_mutex_unlock(&mutex);

    //send answer (echo)
  /*nbytes = write(newsockfd, &photo_aux, sizeof(photo));
    if( nbytes< 0){
      perror("Write: ");
      free(wa);
      pthread_exit(NULL);
    }
    printf("replying %d bytes message type:%d\n", nbytes, photo_aux.type);*/
  }

  // communicate to gateway to change state
  auxm.type = 4;
  strcpy(auxm.address, wa->address);
  auxm.port = atoi(wa->port);
  nbytes = sendto(sock_gt, &auxm, sizeof( struct message_gw),0, (const struct sockaddr *) &gateway_addr, sizeof(gateway_addr));
  if( nbytes< 0){
    perror("Sending to gateway: ");
    free(wa);
    pthread_exit(NULL);
  }

  //Confirmate disconnection to client
  nbytes = write(newsockfd, &photo_aux, sizeof(photo));
  if( nbytes< 0){
    perror("Write: ");
    free(wa);
    pthread_exit(NULL);
  }
  printf("replying %d bytes message:%d\n", nbytes, photo_aux.type);

  printf("Exiting thread\n");
  gallery_clean_list(head);
  free(wa);
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
    char buffer[BUFFERSIZE];
    message_gw auxm;
    struct sockaddr_in local_addr;
    struct sockaddr_in client_addr;
    socklen_t size_addr;

    //var for threads
    pthread_t thread_id, sync_thread;
    struct workerArgs *wa;

    int nbytes, newsockfd, portno;
    unsigned int clilen;

    if (argc < 5) {
         fprintf(stderr,"Usage: serveraddr serverport gatewayaddress gatewayport\n");
         exit(1);
     }

    /* create socket  */
    int sock_fd = socket(AF_INET, SOCK_STREAM,0);

    if(sock_fd == -1){
      perror("socket: ");
      exit(-1);
    }

    /* create socket  */
    int sock_gt = socket(AF_INET, SOCK_DGRAM,0);
    if(sock_fd == -1){
      perror("socket: ");
      exit(-1);
    }

    //contact with gateway
    bzero((char *) &gateway_addr, sizeof(gateway_addr));
    gateway_addr.sin_family = AF_INET;
    gateway_addr.sin_addr.s_addr = inet_addr(argv[3]);
    gateway_addr.sin_port = htons(atoi(argv[4]));

    //server
    bzero((char *) &local_addr, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = inet_addr(argv[1]);
    local_addr.sin_port = htons(atoi(argv[2]));

    int err = bind(sock_fd, (struct sockaddr *)&local_addr, sizeof(local_addr));
    if(err == -1){
      perror("bind: ");
      close(sock_gt);
      close(sock_fd);
      exit(-1);
    }

    //process message to gateway (register)
    auxm.type = 1;
    strcpy(auxm.address, argv[1]);
    auxm.port = atoi(argv[2]);

    //send checkin message to gateway
    nbytes = sendto(sock_gt, &auxm, sizeof( struct message_gw),0, (const struct sockaddr *) &gateway_addr, sizeof(gateway_addr));
    if( nbytes < 0) perror("Sending to gateway: ");

    nbytes = recv(sock_gt, &auxm, sizeof( struct message_gw),0);
    if( nbytes< 0) perror("Receiving from gateway: ");
    printf("received %d bytes with address %s and port %d\n", nbytes , auxm.address, auxm.port);

    /*
    //SET NEXT PEER if it isnt peer online
    if(auxm.port != 0){
      bzero((char *) &next_peer_addr, sizeof(next_peer_addr));
      next_peer_addr.sin_family = AF_INET;
      next_peer_addr.sin_addr.s_addr = inet_addr(auxm.address);
      next_peer_addr.sin_port = htons(auxm.port);
    }
    pthread_mutex_init(&next_peer_lock, NULL);


    //Peers sync thread
    if(pthread_create(&sync_thread, NULL, peers_sync, &sock_gt) != 0){
      perror("Could not create thread");
      close(newsockfd);
      close(sock_gt);
      close(sock_fd);
      pthread_mutex_destroy(&next_peer_lock);
      exit(-1);
    }

    //Initiate mutex that protects list
    pthread_mutex_init(&mutex, NULL);
    */

    listen(sock_fd,5);
    clilen = sizeof(client_addr);
    while(1){

        printf("accept\n");
        newsockfd = accept(sock_fd, (struct sockaddr *) &client_addr, &clilen);
        if(newsockfd < 0){
          close(newsockfd);
          perror("Accept: ");
        }

        wa = malloc(sizeof(struct workerArgs));
        wa->gatesock = sock_gt;
        wa->clisock = newsockfd;
        strcpy(wa->address, argv[1]);
        strcpy(wa->port, argv[2]);

        //ctrl-c pressed!
          if(flag ==1){
            //send message to remove from gateway type 5
            auxm.type = 5;
            strcpy(auxm.address, wa->address);
            auxm.port = atoi(wa->port);
            nbytes = sendto(sock_gt, &auxm, sizeof( struct message_gw),0, (const struct sockaddr *) &gateway_addr, sizeof(gateway_addr));
            if( nbytes < 0 ) perror("Sending to gateway: ");
            pthread_mutex_destroy(&mutex);
            pthread_mutex_destroy(&next_peer_lock);
            close(sock_gt);
            close(sock_fd);
            exit(0);
          }


          if(pthread_create(&thread_id, NULL, handle_client, wa) != 0){
            perror("Could not create thread");
            close(newsockfd);
            close(sock_fd);
            free(wa);
          }
    }
    // never goes here
    return 0;

}
