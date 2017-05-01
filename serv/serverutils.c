#include "serverutils.h"


void * handle_client(void * arg){

  // get arguments
  struct workerArgs *wa;
  wa = (struct workerArgs*) arg;

  message_gw auxm;
  message m;
  int nbytes, sock_gt, newsockfd;
  sock_gt = wa->gatesock;
  newsockfd = wa->clisock;

  //necessario?
  pthread_detach(pthread_self());

  //if accept was sucesssfull communicate to gateway to change state
  auxm.type = 3;
  strcpy(auxm.address, wa->address);
  auxm.port = atoi(wa->port);
  nbytes = sendto(sock_gt, &auxm, sizeof( struct message_gw),0, (const struct sockaddr *) &(wa->gateway_addr), sizeof(wa->gateway_addr));
  if(nbytes< 0){
    perror("Sending to gateway: ");
    free(wa);
    pthread_exit(NULL);
  }

  // read message
  nbytes = read(newsockfd,m.buffer,MESSAGE_LEN);
  if( nbytes< 0){
    perror("Read: ");
    free(wa);
    pthread_exit(NULL);
  }
  printf("received %d bytes message:%s\n", nbytes, m.buffer);

  //process message...

  //send answer (echo)
  nbytes = send(newsockfd, m.buffer, MESSAGE_LEN, 0);
  if( nbytes< 0){
    perror("Write: ");
    free(wa);
    pthread_exit(NULL);
  }
  printf("replying %d bytes message:%s\n", nbytes, m.buffer);

  // communicate to gateway to change state
  //change to a more secure way to do...
  auxm.type = 4;
  strcpy(auxm.address, wa->address);
  auxm.port = atoi(wa->port);
  nbytes = sendto(sock_gt, &auxm, sizeof( struct message_gw),0, (const struct sockaddr *) &(wa->gateway_addr), sizeof(wa->gateway_addr));
  if( nbytes< 0){
    perror("Sending to gateway: ");
    free(wa);
    pthread_exit(NULL);
  }

  free(wa);
  pthread_exit(NULL);
}
