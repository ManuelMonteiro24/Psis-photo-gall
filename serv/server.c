#include "serverutils.h"

//gateway_addr shared by all threads
struct sockaddr_in gateway_addr;
struct sockaddr_in gateway_addr_sync;
struct sockaddr_in gate_serv_addr;

//server to gateway socket TCP
int sock_gate_peer;
int numbPhotos;
int sock_gt, sock_fd;
int peer_port;
char peer_addr[20];

//Peer photo list thats going to be shared between the threads
photo* head = NULL;

pthread_rwlock_t rwlock = PTHREAD_RWLOCK_INITIALIZER;

//ctrl-c pressed!
void exit_handler(int sig){

  int nbytes;

  message_gw auxm;

  //send message to remove from gateway type 5
  auxm.type = 5;
  strcpy(auxm.address, peer_addr);
  auxm.port = peer_port;
  nbytes = sendto(sock_gt, &auxm, sizeof( struct message_gw),0, (const struct sockaddr *) &gateway_addr, sizeof(gateway_addr));
  if( nbytes < 0 ) perror("Sending to gateway: ");
  pthread_rwlock_destroy(&rwlock);
  gallery_clean_list(head);
  close(sock_gt);
  close(sock_fd);
  exit(0);
}

void * handle_hearbeat(void * arg){

  message_gw auxm;

  int nbytes;

  sleep(2);

  while(1){
    //update heartbeat message
    auxm.type = 0;
    strcpy(auxm.address, peer_addr);
    auxm.port = peer_port;
    nbytes = sendto(sock_gt, &auxm, sizeof( struct message_gw),0, (const struct sockaddr *) &gateway_addr, sizeof(gateway_addr));
    if( nbytes < 0 ) perror("Sending to gateway: ");
    sleep(5);
  }
  pthread_exit(NULL);

}

void * handle_client(void * arg){

  // get arguments
  struct workerArgs *wa;
  wa = (struct workerArgs*) arg;

  message_gw auxm;
  int nbytes, sock_gt, newsockfd, err, it;
  uint32_t ret;
  sock_gt = wa->gatesock;
  newsockfd = wa->clisock;

  struct photo photo_aux;
  Message msg;
  char *file_bytes = malloc(MAX_FILE_SIZE), file_name[MAX_WORD_SIZE];
  char *save_bytes = file_bytes;
  int file_size, fs_aux;
  struct identifier *ids, *aux_id, *rm;
  ids = aux_id = NULL;
  keyword *kws = NULL, *aux_kw;

  pthread_detach(pthread_self());

  //if accept was sucesssfull communicate to gateway to change state
  auxm.type = 3;
  strcpy(auxm.address, wa->address);
  auxm.port = atoi(wa->port);
  nbytes = sendto(sock_gt, &auxm, sizeof( struct message_gw),0, (const struct sockaddr *) &gateway_addr, sizeof(gateway_addr));
  if(nbytes< 0){
    perror("Sending to gateway: ");
    free(wa);
    free(save_bytes);
    close(newsockfd);
    pthread_exit(NULL);
  }

  while(1){
      // read message
      memset(&msg, -1, sizeof(msg));
      ret = -2;
      nbytes = read(newsockfd, &msg, sizeof(msg));
      if( nbytes <= 0 ){
        if(nbytes == 0){
          printf("Connection closed..\n");
        } else {
          perror("Received message: ");
        }
        break;
      }

      /*Process message:
      type 0: add photo,
      type 1: add keyword,
      type 2: search by keyword,
      type 3: delete photo,
      type 4: get photo name by id,
      type 5: ??
      */

      switch(msg.type){

        case 0:
          nbytes = read(newsockfd, &file_size, 4); //read file
          if( nbytes < 0 ){
            perror("Read: ");
            free(wa);
            free(save_bytes);
            close(newsockfd);
            pthread_exit(NULL);
          }

          if(msg.update == 0){
            printf("file size from client: %d\n", file_size);
          }

          if(msg.update == 1){
            printf("file size from gateway: %d\n", file_size);
          }

          fs_aux = file_size;

          memset(save_bytes, 0, MAX_FILE_SIZE);
          file_bytes = save_bytes;
          while(fs_aux > 0){
            nbytes = read(newsockfd, file_bytes, fs_aux); //read file
            if(nbytes < 0){
              perror("Read: ");
              free(wa);
              free(save_bytes);
              close(newsockfd);
              pthread_exit(NULL);
            }
            fs_aux -= nbytes;
            file_bytes += nbytes;
          }

          pthread_rwlock_wrlock(&rwlock);
          ret = add_photo(&head, msg.payload, msg.identifier, msg.update, save_bytes, file_size);
          if(ret > 0){
            numbPhotos++;
          }
          pthread_rwlock_unlock(&rwlock);
          if(msg.update == 0){
            nbytes = write(newsockfd, &ret, sizeof(int)); //send return signal to client
            if( nbytes < 0 ){
              perror("Write ret: ");
              free(wa);
              free(save_bytes);
              close(newsockfd);
              pthread_exit(NULL);
            }
          }
          pthread_rwlock_rdlock(&rwlock);
          print_list(head);
          pthread_rwlock_unlock(&rwlock);
          break;

        case 1:
          pthread_rwlock_wrlock(&rwlock);
          ret = add_keyword(head, msg.identifier, msg.payload);
          pthread_rwlock_unlock(&rwlock);
          if(msg.update == 0){
            nbytes = write(newsockfd, &ret, sizeof(int)); //send return signal to client
            if( nbytes < 0 ){
              perror("Write ret: ");
              free(wa);
              free(save_bytes);
              close(newsockfd);
              pthread_exit(NULL);
            }

          }
          pthread_rwlock_rdlock(&rwlock);
          print_list(head);
          pthread_rwlock_unlock(&rwlock);
          break;

        case 2:
          pthread_rwlock_rdlock(&rwlock);
          ret = get_photo_by_keyword(head, &ids, msg.payload);
          pthread_rwlock_unlock(&rwlock);
          nbytes = write(newsockfd, &ret, sizeof(int)); //send number of found photos to client
          if( nbytes < 0 ){
            perror("Write ret type 2(1): ");
            free(wa);
            free(save_bytes);
            close(newsockfd);
            pthread_exit(NULL);
          }
          for(aux_id = rm = ids; aux_id != NULL; rm = aux_id, aux_id = aux_id->next){
            nbytes = write(newsockfd, &aux_id->id, sizeof(int)); //send return signal to client
            if( nbytes < 0 ){
              perror("Write ret type 2(2): ");
              free(wa);
              free(save_bytes);
              close(newsockfd);
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
          pthread_rwlock_wrlock(&rwlock);
          ret = delete_photo(&head, msg.identifier);
          pthread_rwlock_unlock(&rwlock);
          if(msg.update == 0){
            nbytes = write(newsockfd, &ret, sizeof(int)); //send return signal to client
            if( nbytes < 0 ){
              perror("Write ret: ");
              free(wa);
              free(save_bytes);
              close(newsockfd);
              pthread_exit(NULL);
            }
          }
          pthread_rwlock_rdlock(&rwlock);
          print_list(head);
          pthread_rwlock_unlock(&rwlock);
          break;

        case 4:
          pthread_rwlock_rdlock(&rwlock);
          ret = gallery_get_photo_name(head, msg.identifier, file_name);
          pthread_rwlock_unlock(&rwlock);
          nbytes = write(newsockfd, &ret, sizeof(int)); //send return signal to client
          if( nbytes < 0 ){
            perror("Write ret 4: ");
            free(wa);
            free(save_bytes);
            close(newsockfd);
            pthread_exit(NULL);
          }

          if(ret == 1){
            nbytes = write(newsockfd, file_name, MAX_WORD_SIZE); //send return signal to client
            if( nbytes < 0 ){
              perror("Write type 4 ");
              free(wa);
              free(save_bytes);
              close(newsockfd);
              pthread_exit(NULL);
            }
            memset(file_name, 0, MAX_WORD_SIZE);
          }
          break;

        case 5:
          pthread_rwlock_rdlock(&rwlock);
          ret = gallery_get_photo(head, msg.identifier, save_bytes, &file_size);
          pthread_rwlock_unlock(&rwlock);
          nbytes = write(newsockfd, &ret, sizeof(int)); //send return signal to client
          if( nbytes < 0 ){
            perror("Write ret 4: ");
            free(wa);
            free(save_bytes);
            close(newsockfd);
            pthread_exit(NULL);
          }

          if(ret == 1){
            nbytes = write(newsockfd, &file_size, sizeof(file_size)); //send return signal to client
            if( nbytes < 0 ){
              perror("Write type 5 ");
              free(wa);
              free(save_bytes);
              close(newsockfd);
              pthread_exit(NULL);
            }

            nbytes = write(newsockfd, save_bytes, file_size); //send return signal to client
            if( nbytes < 0 ){
              perror("Write type 5 ");
              free(wa);
              free(save_bytes);
              close(newsockfd);
              pthread_exit(NULL);
            }
          }

          break;

        case 6:
          pthread_rwlock_rdlock(&rwlock);
          ret = send_database(newsockfd, head, numbPhotos);
          pthread_rwlock_unlock(&rwlock);
          break;

        default:
          printf("ERROR: received message type matched by default\n");
          break;
      }


      if(msg.type > 6) break; //exit while

      if((ret > 0) && (msg.type == 0 || msg.type == 1 || msg.type == 3) && msg.update == 0){

          /* create socket  */
          sock_gate_peer = socket(AF_INET, SOCK_STREAM,0);
          if(sock_gate_peer == -1){
            perror("socket: ");
            free(save_bytes);
            close(newsockfd);
            pthread_exit(NULL);
          }

          if(connect(sock_gate_peer, (struct sockaddr *) &gateway_addr_sync, sizeof(gateway_addr_sync)) < 0){
            perror("Connect gateway: ");
            free(save_bytes);
            close(newsockfd);
            close(sock_gate_peer);
            pthread_exit(NULL);
          }

          msg.update = 1;
          if(msg.type == 0){
            msg.identifier = ret;
          }

          nbytes = write(sock_gate_peer, &msg, sizeof(msg));
          if(nbytes < 0){
            perror("Write: ");
            free(save_bytes);
            close(newsockfd);
            close(sock_gate_peer);
            pthread_exit(NULL);
          }

          if(msg.type == 0){
            nbytes = write(sock_gate_peer, &file_size, 4); //send file size
            if(nbytes < 0){
              perror("Write: ");
              free(save_bytes);
              close(newsockfd);
              close(sock_gate_peer);
              pthread_exit(NULL);
            }

            nbytes = write(sock_gate_peer, save_bytes, file_size);
            if(nbytes< 0){
              perror("Write: ");
              free(save_bytes);
              close(newsockfd);
              close(sock_gate_peer);
              pthread_exit(NULL);
            }
          }
          close(sock_gate_peer);
      }
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

  free(wa);
  pthread_exit(NULL);
}

int main(int argc, char const *argv[]) {

    //signals
    struct sigaction sa;
    sa.sa_handler = &exit_handler;
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
    pthread_t thread_id, sync_thread, thread_id_update;
    struct workerArgs *wa;

    int nbytes, newsockfd, updateSocket, portno;
    unsigned int clilen;

    if (argc < 5) {
         fprintf(stderr,"Usage: serveraddr serverport gatewayaddress gatewayport\n");
         exit(1);
     }

    /* create socket  */
    sock_fd = socket(AF_INET, SOCK_STREAM,0);

    if(sock_fd == -1){
      perror("socket: ");
      exit(-1);
    }

    /* create socket  */
    sock_gt = socket(AF_INET, SOCK_DGRAM,0);
    if(sock_fd == -1){
      perror("socket: ");
      exit(-1);
    }

    //contact with gateway
    bzero((char *) &gateway_addr, sizeof(gateway_addr));
    gateway_addr.sin_family = AF_INET;
    gateway_addr.sin_addr.s_addr = inet_addr(argv[3]);
    gateway_addr.sin_port = htons(atoi(argv[4]));

    bzero((char *) &gateway_addr_sync, sizeof(gateway_addr_sync));
    gateway_addr_sync.sin_family = AF_INET;
    gateway_addr_sync.sin_addr.s_addr = inet_addr(argv[3]);
    gateway_addr_sync.sin_port = htons(atoi(argv[4])+1);

    //server
    bzero((char *) &local_addr, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = inet_addr(argv[1]);
    local_addr.sin_port = htons(atoi(argv[2]));

    strcpy(peer_addr, argv[1]);
    peer_port =  atoi(argv[2]);

    int err = bind(sock_fd, (struct sockaddr *)&local_addr, sizeof(local_addr));
    if(err == -1){
      perror("bind: ");
      close(sock_gt);
      close(sock_fd);
      close(sock_gate_peer);
      exit(-1);
    }

    //process message to gateway (register)
    auxm.type = 1;
    strcpy(auxm.address, argv[1]);
    auxm.port = atoi(argv[2]);

    //send checkin message to gateway
    nbytes = sendto(sock_gt, &auxm, sizeof( message_gw),0, (const struct sockaddr *) &gateway_addr, sizeof(gateway_addr));
    if( nbytes < 0) perror("Sending to gateway: ");

    nbytes = recv(sock_gt, &auxm, sizeof(message_gw),0);
    if( nbytes < 0) perror("Recv from gateway: ");
    if(auxm.type != 0){
      //contact with gateway pc
      bzero((char *) &gate_serv_addr, sizeof(gate_serv_addr));
      gate_serv_addr.sin_family = AF_INET;
      gate_serv_addr.sin_addr.s_addr = inet_addr(auxm.address);
      gate_serv_addr.sin_port = htons(auxm.port);

      updateSocket = socket(AF_INET, SOCK_STREAM,0);
      if(updateSocket == -1){
        perror("update socket: ");
        exit(-1);
      }

      if(connect(updateSocket,( struct sockaddr *) &gate_serv_addr, sizeof(gate_serv_addr)) < 0){
        perror("Connect update socket: ");
        close(updateSocket);
        return(0); //No peer available
      }

      if((numbPhotos = update_database(updateSocket, &head)) == -1){
        printf("ERROR: database update\n");
        //send message to remove from gateway type 5
        auxm.type = 5;
        strcpy(auxm.address, wa->address);
        auxm.port = atoi(wa->port);
        nbytes = sendto(sock_gt, &auxm, sizeof( struct message_gw),0, (const struct sockaddr *) &gateway_addr, sizeof(gateway_addr));
        if( nbytes < 0 ) perror("Sending to gateway: ");
        gallery_clean_list(head);
        pthread_rwlock_destroy(&rwlock);
        close(sock_gt);
        close(sock_fd);
        close(updateSocket);
        exit(0);
      }

      close(updateSocket);
    }

    //heartbeat thread
    if(pthread_create(&thread_id_update, NULL, handle_hearbeat, NULL) != 0){
      perror("Could not create heartbeathread");
      close(sock_gt);
      close(sock_fd);
      exit(1);
    }

    listen(sock_fd,5);
    clilen = sizeof(client_addr);
    while(1){
        newsockfd = accept(sock_fd, (struct sockaddr *) &client_addr, &clilen);
        if(newsockfd < 0){
          close(newsockfd);
          perror("Accept: ");
        }else{
          wa = malloc(sizeof(struct workerArgs));
          wa->gatesock = sock_gt;
          wa->clisock = newsockfd;
          strcpy(wa->address, argv[1]);
          strcpy(wa->port, argv[2]);

          if(pthread_create(&thread_id, NULL, handle_client, wa) != 0){
            perror("Could not create thread");
            close(newsockfd);
            free(wa);
          }
        }
    }
    // never goes here
    return 0;
}
