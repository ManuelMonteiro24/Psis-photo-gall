#include "gatewayutils.h"

volatile sig_atomic_t flag = 0;
void my_handler(int sig){
  flag = 1;
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

    //erro a passar pointer
    struct workerArgs *wa;
    servernode* head = create_server_list();
    wa = malloc(sizeof(struct workerArgs));
    wa->sock = sock_fd;
    wa->list = head;

    if(pthread_create(&thread_id, NULL, client_server,wa) != 0){
      perror("Could not create clients thread");
      close(sock_fd);
      close(sock_peers);
      exit(-1);
    }

    struct workerArgs *wa1;
    wa1 = malloc(sizeof(struct workerArgs));
    wa1->sock = sock_peers;
    wa1->list = head;

    if(pthread_create(&thread_id0, NULL, peers_server,wa1) != 0){
      perror("Could not create peers thread");
      close(sock_fd);
      close(sock_peers);
      exit(-1);
    }


    while(1){

      //best way to shut down the threads
      //ctrl-c pressed!
        if(flag ==1){
          clean_server_list(head);
          close(sock_fd);
          close(sock_peers);
          int s = pthread_cancel(thread_id0);
          if(s != 0) perror("server thread cancel: ");
          s = pthread_cancel(thread_id);
          if(s != 0) perror("peers thread cancel: ");
          free(wa);
          free(wa1);
          exit(0);
          //clear list to do...
        }

    }
    // nunca vai para aqui
    return 0;

}
