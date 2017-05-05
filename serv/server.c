  #include "serverutils.h"

photo* head = NULL;

//insert at the end of the list, 0-> error, integer ->sucesssfull
int add_photo(char *name){

  int photo_count = 1;
  photo* aux = head;

  if(head != NULL){
    while(aux != NULL){
      aux = aux->next;
      photo_count++;
    }
  }
  photo* new_photo;
  new_photo = (photo*)malloc(sizeof(photo));
  if(new_photo == NULL){
    printf("Error creating photo\n");
    return(0);
  }

  new_photo->identifier = photo_count;
  strcpy(new_photo->name, name);
  new_photo->key_header = NULL;
  new_photo-> next = NULL;

  if(photo_count==1)
    head = new_photo;
  else
    aux->next = new_photo;

  return(photo_count);
}

// 1 -> error, 0 ->sucesssfull
int add_keyword(uint32_t identifier, char *keyword_input){

  //first search for photo
  if(head == NULL){
    printf("Empty list\n");
    return(1);

  }
  photo* aux = head;

  while(aux != NULL){

    //photo found add keyword
    if(identifier == aux->identifier){

      keyword* new_keyword;
      new_keyword = (keyword*)malloc(sizeof(keyword));
      if(new_keyword == NULL){
        printf("Error creating keyword\n");
        return(1);
      }
      strcpy(new_keyword->name, keyword_input);
      new_keyword->next = NULL;

      if(aux->key_header == NULL){
        aux->key_header = new_keyword;
      }else{
        keyword *aux_keyword = aux->key_header;
        while(aux_keyword!= NULL){

          //keyword duplicates not allowed
          if(strcmp(keyword_input, aux_keyword->name) == 0){
            printf("keyword already in photo\n");
            free(new_keyword);
            return(1);
          }
          aux_keyword = aux_keyword->next;
        }

        aux_keyword->next = new_keyword;
      }
      return(0);
    }
    aux = aux->next;
  }
  printf("Photo not found!\n");
  return(1);
}

// -1 -> error, 0 ->no photos integer->number of photos count
int search_by_keyword( uint32_t** id_photos, char *keyword_input){

  //create id_photos to do..??

  if(head == NULL){
    printf("Empty list\n");
    return(0);
  }

  photo* aux = head;
  keyword* keyword_aux = NULL;
  int photo_count = 0;

  while(aux != NULL){
    keyword_aux = aux->key_header;
    while(keyword_aux != NULL){
      if(strcmp(keyword_input, keyword_aux->name) == 0){
        photo_count++;
        // add to the id_photos
      }
      keyword_aux = keyword_aux->next;
    }
    aux = aux->next;
  }

  return(photo_count);
}

// 0 ->no photo 1->sucesssfull
int delete_photo(uint32_t identifier){

  if(head == NULL){
    printf("Empty list\n");
    return(0);
  }
  photo *aux1 = head;
  photo *aux2 = head;

  if((head->next == NULL) && (identifier == aux1->identifier)){
    head = NULL;
    return(1);
  }

  while(aux1!= NULL){
    if(identifier == aux1->identifier){
      aux2->next = aux1->next;
      return(1);
    }
    aux2 = aux1;
    aux1 = aux1->next;
  }
  return(0);
}

// 0 ->no photo 1->sucesssfull
int gallery_get_photo_name( uint32_t id_photo,char **photo_name){

  if(head==NULL){
    printf("Empty list\n");
    return(0);
  }

  photo * aux = head;

  while(aux!= NULL){
    if(aux->identifier== id_photo){
      //get photo name to photo_name var to return
      return(1);
    }
  aux = aux->next;
  }
  return(0);
}

// 0 ->no photo 1->sucesssfull, change return or parameters to send photo data to do...
//talvez o melhor seja receber a socket e enviar dentro da func
int gallery_get_photo( uint32_t id_photo){

  if(head==NULL){
    printf("Empty list\n");
    return(0);
  }

  photo * aux = head;

  while(aux!=NULL){
    if(aux->identifier== id_photo){
      //get photo data and sent to client
      return(1);
    }
    aux = aux->next;
  }
  return(0);

}

//debug
void print_list(){

  photo* aux = head;
  keyword *aux2 = NULL;
  while (aux!=NULL) {
    printf("photo list: iden-> %d name-> %s keyword's:",aux->identifier,aux->name);
    aux2 = aux->key_header;
    if(aux2 != NULL){
      while(aux2 != NULL){
        printf(" %s", aux2->name);
        aux2=aux2->next;
      }
    }
    aux = aux->next;
  }
  printf("\n");

}

void keyword_clean_list(keyword * head){
  keyword* aux = head, *aux1;
  while(aux !=NULL){
    aux1 = aux;
    aux = aux->next;
    free(aux1);
  }
}

void gallery_clean_list(){
  photo* aux = head, *aux1;
  while(aux !=NULL){
    aux1 = aux;
    aux = aux->next;
    keyword_clean_list(aux1->key_header);
    free(aux1);
  }
}

void * handle_client(void * arg){

  // get arguments
  struct workerArgs *wa;
  wa = (struct workerArgs*) arg;

  message_gw auxm;
  message m;
  int nbytes, sock_gt, newsockfd;
  sock_gt = wa->gatesock;
  newsockfd = wa->clisock;

  struct photo photo_aux;

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
  //need to put this in loop to do...

  // read message
  nbytes = read(newsockfd,&photo_aux,sizeof(photo_aux));
  if( nbytes< 0){
    perror("Read: ");
    free(wa);
    pthread_exit(NULL);
  }
  //process message
  if(photo_aux.type == 0){
    photo_aux.type = add_photo(photo_aux.name);
    print_list();
  }else if(photo_aux.type ==1){
    photo_aux.type = add_keyword(photo_aux.identifier, photo_aux.name); //keyword por agora vai no name
    print_list();
  }else if(photo_aux.type ==2){
    //function to do...
    //photo_aux.type = search_by_keyword();
  }else if(photo_aux.type ==3){
    photo_aux.type = delete_photo(photo_aux.identifier);
    print_list();
  }else if(photo_aux.type ==4){
    //function to do...
    //photo_aux.type = gallery_get_photo_name();
  }else{
    photo_aux.type = gallery_get_photo(photo_aux.identifier);
    print_list();
  }

  //send answer (echo)
  nbytes = send(newsockfd, &photo_aux, sizeof(photo_aux), 0);
  if( nbytes< 0){
    perror("Write: ");
    free(wa);
    pthread_exit(NULL);
  }
  printf("replying %d bytes message:%s\n", nbytes, m.buffer);

  // communicate to gateway to change state
  //do this by reciving a signal to terminate to do...
  auxm.type = 4;
  strcpy(auxm.address, wa->address);
  auxm.port = atoi(wa->port);
  nbytes = sendto(sock_gt, &auxm, sizeof( struct message_gw),0, (const struct sockaddr *) &(wa->gateway_addr), sizeof(wa->gateway_addr));
  if( nbytes< 0){
    perror("Sending to gateway: ");
    free(wa);
    pthread_exit(NULL);
  }

  printf("Exiting thread\n");
  free(wa);
  pthread_exit(NULL);
}

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
            auxm.type = 5;
            strcpy(auxm.address, argv[1]);
            auxm.port = atoi(argv[2]);
            nbytes = sendto(sock_gt, &auxm, sizeof( struct message_gw),0, (const struct sockaddr *) &gateway_addr, sizeof(gateway_addr));
            if( nbytes< 0) perror("Sending to gateway: ");
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
