#include "serverutils.h"

//create empty list
photo* create_photo_list(char *name){
  return NULL;
}

//insert at the end of the list, 1 -> error, 0 ->sucesssfull
int add_photo(photo* head,char *name){

  int photo_count = 0;
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
    return(1);
  }

  new_photo->identifier = photo_count;
  strcpy(new_photo->name, name);
  new_photo->key_header = NULL;
  new_photo-> next = NULL;

  if(photo_count==0)
    *head = new_photo;
  else
    aux->next = new_photo;

  return 0;
}

// 1 -> error, 0 ->sucesssfull
int add_keyword(photo* head,uint32_t identifier, char *keyword_input){

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
int search_by_keyword(photo* head, uint32_t** id_photos, char *keyword_input){

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
int delete_photo(photo* head, uint32_t identifier){

  if(head == NULL){
    printf("Empty list\n");
    return(0);
  }

  photo *aux1 = head;
  photo *aux2 = head;

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
int gallery_get_photo_name(photo* head, uint32_t id_photo,char **photo_name){

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
int gallery_get_photo(photo* head, uint32_t id_photo){

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
void print_list(photo * head){

  photo* aux = head;
  keyword *aux2 = NULL;
  while (aux!=NULL) {
    printf("photo: iden %d name %s keyword's:",aux->identifier,aux->name);
    aux2 = aux->key_header;
    if(aux2 != NULL){
      while(aux2 != NULL){
        printf(" %s", aux2->name);
        aux2=aux2->next;
      }
    }
    aux = aux->next;
  }
}

void gallery_clean_list(photo * head){
  photo* aux = head, *aux1;
  while(aux !=NULL){
    aux1 = aux;
    aux = aux->next;
    keyword_clean_list(aux1->key_header);
    free(aux1);
  }
}

void keyword_clean_list(keyword * head){
  keyword* aux = head, *aux1;
  while(aux !=NULL){
    aux1 = aux;
    aux = aux->next;
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
