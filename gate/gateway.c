#include "gatewayutils.h"

servernode* head = NULL;

int sock_client, sock_peers, sock_peers_sync;

pthread_mutex_t mutex;

void exit_handler(int sig){
    close(sock_client);
    close(sock_peers);
    close(sock_peers_sync);
    clean_server_list(head);
    pthread_mutex_destroy(&mutex);
    exit(0);
}

void alarm_handler(int sig){
  alarm(25);
  check_heartbeat(&head);
  printf("Check heartbeat done\n");
  print_server_list(head);
}

void * sync_peers(void * arg){

  //get arguments
  struct workerArgs *wa;
  wa = (struct workerArgs*) arg;

  int sock_sync_peers_accepted = wa->sock_fd, file_size;
  struct sockaddr_in sender_addr = wa-> sender_addr;
  servernode *aux = head;
  struct sockaddr_in peer_addr;
  int nbytes, sock_sync;
  char file_bytes[MAX_FILE_SIZE];

  Message msg;
  memset(&msg, -1, sizeof(msg));

  pthread_detach(pthread_self());

  if(head == NULL){
    pthread_exit(NULL);
  }

  nbytes = read(sock_sync_peers_accepted, &msg, sizeof(msg));
  if(nbytes < 0){
    perror("Received message: ");
    free(wa);
    close(sock_sync_peers_accepted);
    pthread_exit(NULL);
  } else if(nbytes == 0){
    printf("Connection closed by the peer..\n");
    free(wa);
    close(sock_sync_peers_accepted);
    pthread_exit(NULL);
  }

  if(msg.type == 0){
    file_size = read(sock_sync_peers_accepted, file_bytes, MAX_FILE_SIZE); //read file
    printf("received photo with %d bytes\n", nbytes);
    if( nbytes < 0 ){
      perror("Read: ");
      free(wa);
      close(sock_sync_peers_accepted);
      pthread_exit(NULL);
    }
  }

  while(aux != NULL){
    printf("sending to peer addr %s port %d\nsender addr %s port %d", aux->address, aux->port, inet_ntoa(sender_addr.sin_addr), sender_addr.sin_port);
    peer_addr.sin_family = AF_INET;
    peer_addr.sin_addr.s_addr = inet_addr(aux->address);
    peer_addr.sin_port = htons(aux->port);
    sock_sync = socket(AF_INET, SOCK_STREAM, 0);
    if(sock_sync == -1){
      perror("socket: ");
      close(sock_sync_peers_accepted);
      pthread_exit(NULL);
    }

    if(connect(sock_sync,(struct sockaddr *) &peer_addr, sizeof(peer_addr)) < 0){
      perror("Connect: ");
      close(sock_sync);
      close(sock_sync_peers_accepted);
      pthread_exit(NULL);
    }
    nbytes = write(sock_sync, &msg, sizeof(msg));
    if(nbytes< 0){
      perror("Write: ");
      close(sock_sync);
      close(sock_sync_peers_accepted);
      pthread_exit(NULL);
    }

    if(msg.type == 0){
      nbytes = write(sock_sync, file_bytes, file_size);
      if(nbytes< 0){
        perror("Write: ");
        close(sock_sync);
        close(sock_sync_peers_accepted);
        pthread_exit(NULL);
      }
    }
    aux = aux->next;
    close(sock_sync);
  }
  close(sock_sync_peers_accepted);
  pthread_exit(NULL);
}

void * peers_server_sync(void * arg){

  struct sockaddr_in peer_addr;
  int newsockfd;
  workerArgs *wa;
  pthread_t thread_id;

  listen(sock_peers_sync, 5);
  socklen_t peer_len = sizeof(peer_addr);
  while(1){

      printf("accept sync\n");
      newsockfd = accept(sock_peers_sync, (struct sockaddr *) &peer_addr, &peer_len);
      if(newsockfd < 0){
        close(newsockfd);
        perror("Accept: ");
      }

      wa = malloc(sizeof(struct workerArgs));
      wa->sock_fd = newsockfd;
      wa->sender_addr = peer_addr;

      if(pthread_create(&thread_id, NULL, sync_peers, wa) != 0){
        perror("Could not create thread");
        close(newsockfd);
      }
  }
  pthread_exit(NULL);
}

void * client_server(void * arg){

  message_gw auxm;
  struct sockaddr_in client_addr;
  servernode *auxpointer = NULL;
  socklen_t size_addr;
  int nbytes,flagaux =0;

  while(1){
    //read
    size_addr = sizeof(struct sockaddr_in);
    nbytes = recvfrom(sock_client,&auxm,sizeof(struct message_gw),0,(struct sockaddr *) & client_addr, &size_addr);
    if( nbytes< 0) perror("Read: ");
    printf("received: %d %d %s %d\n", nbytes, auxm.type, auxm.address, auxm.port);

    //process message, process for server avalbility to do...
    if(auxm.type ==0){
      pthread_mutex_lock(&mutex);
      find_server(head,&auxm);
      pthread_mutex_unlock(&mutex);
    }
      //send answer for clients
      nbytes = sendto(sock_client, &auxm, sizeof(struct message_gw), 0, ( struct sockaddr *) &client_addr, sizeof(client_addr));
      if( nbytes< 0) perror("Write: ");
       printf("replying %d bytes with address %s and port %d\n", nbytes , auxm.address, auxm.port);
    }

  pthread_exit(NULL);
}

void * peers_server(void * arg){

  //alarm for heartbeat checking
	struct sigaction sa;
	sa.sa_handler = &alarm_handler;
	sa.sa_flags = 0;
	sigfillset(&sa.sa_mask);

	//set first alarm clock 30 seconds
	alarm(30);

  if(sigaction(SIGALRM, &sa, 0) == -1){
		perror("sigaction");
	}

  struct sockaddr_in peer_addr;
  socklen_t size_addr;

  message_gw auxm, auxm2;
  int nbytes, port_aux, ret_aux;
  char address_aux[20];

  while(1){
    //read
    size_addr = sizeof(struct sockaddr_in);
    nbytes = recvfrom(sock_peers,&auxm,sizeof(struct message_gw),0,(struct sockaddr *) & peer_addr, &size_addr);
    if( nbytes< 0) perror("Read: ");
    printf("received: %d %d %s %d\n", nbytes, auxm.type, auxm.address, auxm.port);

    //process message, process for server avalbility to do...
    pthread_mutex_lock(&mutex);
    if(auxm.type == 0){
      update_heartbeat(head,auxm.address, auxm.port);
    }else if(auxm.type == 1){
      //ADDRESS and port from the peer that just register
      strcpy(address_aux,auxm.address);
      port_aux = auxm.port;
      if(find_server(head,&auxm2) == -1){
        auxm2.type = 0; //first peer on list
      } else{
        auxm2.type = 1;
      }

      //auxm recv server after the register peer and auxm2 recv server before the register peer
      ret_aux = insert_server(&head, auxm.address, auxm.port);

      print_server_list(head);

      nbytes = sendto(sock_peers, &auxm2, sizeof(struct message_gw), 0, ( struct sockaddr *) &peer_addr, sizeof(peer_addr));
      if( nbytes< 0) perror("Write: ");
       printf("replying %d bytes with address %s and port %d\n", nbytes , auxm.address, auxm.port);

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
    sa.sa_handler = &exit_handler;
    sa.sa_flags = 0;
  	sigfillset(&sa.sa_mask);
    if( sigaction(SIGINT, &sa,0) == -1){
      perror("sigaction");
    }

    //var for sockets
    message_gw auxm;
    struct sockaddr_in local_addr_client, local_addr_peers, local_addr_peers_sync;
    socklen_t size_addr;
    int nbytes, clilen, newsockfd, portno;

    //threads
    pthread_t thread_client, thread_peers, thread_peers_sync;

    if (argc < 5) {
         fprintf(stderr,"Usage: clientserveraddr clientserverport peersserveraddr peersserverport\n");
         exit(1);
     }

    /* create client server socket  */
    sock_client = socket(AF_INET, SOCK_DGRAM,0);
    if(sock_client == -1){
      perror("socket: ");
      exit(-1);
    }

    //client server
    bzero((char *) &local_addr_client, sizeof(local_addr_client));
    portno = atoi(argv[2]);
    local_addr_client.sin_family = AF_INET;
    local_addr_client.sin_addr.s_addr = inet_addr(argv[1]);
    local_addr_client.sin_port = htons(portno);

    int err = bind(sock_client, (struct sockaddr *)&local_addr_client, sizeof(local_addr_client));
    if(err == -1){
      perror("bind: ");
      close(sock_client);
      exit(-1);
    }

    /* create peers server socket  */
    sock_peers = socket(AF_INET, SOCK_DGRAM,0);
    if(sock_peers == -1){
      perror("socket: ");
      close(sock_client);
      exit(-1);
    }

    //peers server
    bzero((char *) &local_addr_peers, sizeof(local_addr_peers));
    portno = atoi(argv[4]);
    local_addr_peers.sin_family = AF_INET;
    local_addr_peers.sin_addr.s_addr = inet_addr(argv[3]);
    local_addr_peers.sin_port = htons(portno);

    err = bind(sock_peers, (struct sockaddr *)&local_addr_peers, sizeof(local_addr_peers));
    if(err == -1){
      perror("bind: ");
      close(sock_client);
      close(sock_peers);
      exit(-1);
    }

    //Socket peers sync
    /* create socket  */
    sock_peers_sync = socket(AF_INET, SOCK_STREAM,0);
    if(sock_client == -1){
      perror("socket: ");
      exit(-1);
    }

    //server
    bzero((char *) &local_addr_peers_sync, sizeof(local_addr_peers_sync));
    local_addr_peers_sync.sin_family = AF_INET;
    local_addr_peers_sync.sin_addr.s_addr = inet_addr(argv[3]);
    local_addr_peers_sync.sin_port = htons(portno+1); //peers port + 1

    err = bind(sock_peers_sync, (struct sockaddr *)&local_addr_peers_sync, sizeof(local_addr_peers_sync));
    if(err == -1){
      perror("bind: ");
      close(sock_peers);
      close(sock_client);
      close(sock_peers_sync);
      exit(-1);
    }

    //Initiate mutex that protects list
    pthread_mutex_init(&mutex, NULL);

    if(pthread_create(&thread_client, NULL, client_server,NULL) != 0){
      perror("Could not create clients thread");
      close(sock_client);
      close(sock_peers);
      close(sock_peers_sync);
      exit(-1);
    }


    if(pthread_create(&thread_peers, NULL, peers_server,NULL) != 0){
      perror("Could not create peers thread");
      close(sock_client);
      close(sock_peers);
      close(sock_peers_sync);
      exit(-1);
    }

    if(pthread_create(&thread_peers_sync, NULL, peers_server_sync,NULL) != 0){
      perror("Could not create peers thread");
      close(sock_client);
      close(sock_peers);
      close(sock_peers_sync);
      exit(-1);
    }

    while(1){
      //wait till crtl-c pressed
    }
    // never goes here!
    return 0;
}
