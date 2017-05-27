#include "serverutils.h"

//insert at the end of the list, 0-> error, integer ->sucesssfull
uint32_t add_photo(photo **head, char *file_name, char *file_bytes, int file_size){

  //generate random number for photo identifier
  int exists;
  photo *aux, *new_photo = (photo *) malloc(sizeof(photo));
  char file_id[100]; //CHANGE TO APPROPRIATE VALUE

  srand((int) time(NULL)*(int) pthread_self());
  int random_number, photo_id;

  //generate an exclusive photo id; must be different for all photos
  do{
    exists = 0;
    random_number = rand() % 501;
    photo_id = ((int)pthread_self()*random_number)/((int)(time(NULL))*random_number*random_number);

    for(aux = *head; aux != NULL; aux = aux->next){
      if(aux->identifier == photo_id){
        exists = 1;
        break;
      }
    }
  } while(exists);

  //generate file on disk to save photo data
  sprintf(file_id, "%d", photo_id);

  FILE *new_file;
  new_file = fopen(file_id, "w+");
  if(new_file == NULL){
    perror("ADD PHOTO: ");
    return -1;
  }

  fwrite(file_bytes, file_size, 1, new_file);
  fclose(new_file);

  new_photo->identifier = photo_id;
  strcpy(new_photo->name, file_name);
  new_photo->key_header = NULL;
  new_photo->next = NULL;

  //head = new_photo;
  aux = *head;
  if(*head == NULL){
    *head = new_photo;
  }else{
    //1 server on list
    if(aux->next == NULL){
      aux->next = new_photo;
    }else{
      while(aux->next != NULL){
        aux = aux->next;
      }
      aux->next = new_photo;
    }
  }

  return(photo_id);
}

// -1 -> error, 0 ->no photo 1->sucess
int add_keyword(photo* head, uint32_t identifier, char *keyword_input){

  //first search for photo
  if(head == NULL){
    printf("Empty list\n");
    return(0);
  }
  photo* aux = head;

  while(aux != NULL){

    //photo found add keyword
    if(identifier == aux->identifier){

      keyword* new_keyword;
      new_keyword = (keyword*)malloc(sizeof(keyword));
      if(new_keyword == NULL){
        printf("Error creating keyword\n");
        return(-1);
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
            return(-1);
          }
          aux_keyword = aux_keyword->next;
        }

        aux_keyword->next = new_keyword;
      }
      return(1);
    }
    aux = aux->next;
  }
  printf("Photo not found!\n");
  return(0);
}

// -1 -> error, 0 ->no photos integer->number of photos count
int search_by_keyword(photo* head, uint32_t** id_photos, char *keyword_input){

  //create id_photos to send to do..??

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
int delete_photo(photo** head, uint32_t identifier){

  if(*head == NULL){
    printf("Empty list\n");
    return(0);
  }
  photo *aux1 = *head;
  photo *aux2 = *head;

  if(((*head)->next == NULL) && (identifier == aux1->identifier)){
    *head = NULL;
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
int gallery_get_photo_name(photo* head, uint32_t id_photo, photo* photo_aux){

  if(head==NULL){
    printf("Empty list\n");
    return(0);
  }

  photo * aux = head;

  while(aux!= NULL){
    if(aux->identifier== id_photo){
      strcpy(photo_aux->name,aux->name);
      return(1);
    }
  aux = aux->next;
  }
  return(0);
}

// 0 ->no photo 1->sucesssfull, change return or parameters to send photo data to do...
//talvez o melhor seja receber a socket e enviar dentro da func
int gallery_get_photo(photo* head,uint32_t id_photo, photo* photo_aux){

  if(head==NULL){
    printf("Empty list\n");
    return(0);
  }

  photo * aux = head;

  while(aux!=NULL){
    if(aux->identifier== id_photo){
      strcpy(photo_aux->name,aux->name);   //exemplo to modify TO DO
      return(1);
    }
    aux = aux->next;
  }
  return(0);

}

//debug
void print_list(photo *head){

  if(head == NULL){
    printf("Empty list!\n");
    return;
  }

  printf("photo list: ");
  photo* aux = head;
  keyword *aux2 = NULL;
  while (aux!=NULL) {
    printf("iden-> %d name-> %s keyword's:",aux->identifier,aux->name);
    aux2 = aux->key_header;
    if(aux2 != NULL){
      while(aux2 != NULL){
        printf(" %s", aux2->name);
        aux2=aux2->next;
      }
    }
    printf("\n");
    aux = aux->next;
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

void gallery_clean_list(photo* head){
  photo* aux = head, *aux1;
  while(aux !=NULL){
    aux1 = aux;
    aux = aux->next;
    keyword_clean_list(aux1->key_header);
    free(aux1);
  }
}
