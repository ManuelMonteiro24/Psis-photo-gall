  #include "serverutils.h"

//arg serveraddr(127.0.0.1) serverport(51717) gatewayaddress gatewayport
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
    struct sockaddr_in local_addr;
    struct sockaddr_in client_addr;
    struct sockaddr_in gateway_addr;
    socklen_t size_addr;

    //var for threads
    pthread_t thread_id;
    struct workerArgs *wa;

    int nbytes, clilen, newsockfd, portno;

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

    //process message to gateway
    auxm.type = 1;
    strcpy(auxm.address, argv[1]);
    auxm.port = atoi(argv[2]);

    //send checkin message to gateway
    nbytes = sendto(sock_gt, &auxm, sizeof( struct message_gw),0, (const struct sockaddr *) &gateway_addr, sizeof(gateway_addr));
    if( nbytes< 0) perror("Sending to gateway: ");

    listen(sock_fd,5);
    clilen = sizeof(client_addr);
    while(1){

        newsockfd = accept(sock_fd, (struct sockaddr *) &client_addr, &clilen);
        if(newsockfd < 0){
          close(newsockfd);
          perror("Accept: ");
        }

        //ctrl-c pressed!
          if(flag ==1){
            //send message to remove from gateway
            close(sock_gt);
            close(sock_fd);
            exit(0);
          }

          wa = malloc(sizeof(struct workerArgs));
          wa->gatesock = sock_gt;
          wa->clisock = newsockfd;
          wa->gateway_addr = gateway_addr;
          strcpy(wa->address, argv[1]);
          strcpy(wa->port, argv[2]);

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
