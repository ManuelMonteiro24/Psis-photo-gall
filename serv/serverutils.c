#include "serverutils.h"

//insert at the end of the list, 0-> error, integer ->sucesssfull
uint32_t add_photo(photo **head, char *file_name, char *file_bytes, int file_size){

  //generate random number for photo identifier
  int exists;
  photo *aux, *new_photo = (photo *) malloc(sizeof(photo));
  char file_id[10]; //CHANGE TO APPROPRIATE VALUE

  time_t rawtime;
  struct tm *time_info;

  time(&rawtime);
  time_info = localtime(&rawtime);
  srand((int) time_info->tm_sec*(int) pthread_self());
  int random_number, photo_id;

  //generate an exclusive photo id; must be different for all photos
  do{
    exists = 0;
    random_number = rand() % 501;
    time_info = localtime(&rawtime);
    photo_id = ((int)pthread_self()*random_number)/(time_info->tm_sec*random_number*random_number);

    for(aux = *head; aux != NULL; aux = aux->next){
      if(aux->identifier == photo_id){
        exists = 1;
        break;
      }
    }
  } while(exists && photo_id == 0);

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
  new_photo->next = *head;

  *head = new_photo; //insert in the beginning of the list
  return photo_id;
}

// -1 -> error, 0 ->no photo 1->sucess
int add_keyword(photo *head, uint32_t identifier, char keyword_input[20]){

  //first search for photo
  if(head == NULL){
    printf("Empty list\n");
    return(0);
  }
  photo *aux = head;

  while(aux != NULL){
    //photo found; add keyword
    if(identifier == aux->identifier){

      keyword* new_keyword;
      new_keyword = (keyword*) malloc(sizeof(keyword));

      if(new_keyword == NULL){
        printf("Error creating keyword\n");
        return(-1);
      }

      strcpy(new_keyword->word, keyword_input);
      printf("adad %s\n", new_keyword->word);
      new_keyword->next = NULL;

      if(aux->key_header == NULL){
        aux->key_header = new_keyword;

      } else{
        keyword *aux_keyword = aux->key_header;
        while(aux_keyword != NULL){
          //keyword duplicates not allowed
          if(strcmp(new_keyword->word, aux_keyword->word) == 0){
            printf("keyword already in photo\n");
            free(new_keyword);
            return(-1);
          }
          if(aux_keyword->next == NULL){
            break;
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
int get_photo_by_keyword(photo *head, struct identifier **ids, char keyword_input[MAX_WORD_SIZE]){

  if(head == NULL){
    printf("Empty list\n");
    return(0);
  }
  photo *aux = head;
  keyword *keyword_aux = NULL;
  struct identifier *aux_id = NULL;
  int photo_count = 0;

  while(aux != NULL){
    keyword_aux = aux->key_header;
    while(keyword_aux != NULL){
      if(strcmp(keyword_input, keyword_aux->word) == 0){

        photo_count++;
        aux_id = (struct identifier *) calloc(1, sizeof(struct identifier));
        aux_id->id = aux->identifier;

        if((*ids) == NULL){
          aux_id->next = NULL;
        } else{
          aux_id->next = *ids;
        }

        *ids = aux_id;
        break;
      }
      keyword_aux = keyword_aux->next;
    }
    aux = aux->next;
  }

  return photo_count;
}

//0 ->no photo 1->sucesssfull
int delete_photo(photo** head, uint32_t identifier){

  photo *aux1, *aux2 = *head;
  char file_name[10];
  int found = 0;

  if(*head == NULL){
    printf("Empty list\n");
    return(0);
  }

  if(identifier == (*head)->identifier){ //only case where *head needs to be updated
    *head = (*head)->next;
    sprintf(file_name, "%d", (int) aux2->identifier);
    if(unlink(file_name) == -1){ //delete file from file system
      perror("Deleting file ");
    }
    free(aux2);
    return 1;
  }

  for(aux1 = (*head)->next; aux1 != NULL; aux2 = aux1, aux1 = aux1->next){
    if(identifier == aux1->identifier){
      aux2->next = aux1->next;
      sprintf(file_name, "%d", (int) aux1->identifier);
      if(unlink(file_name)  == -1){ //delete file from file system
        perror("Deleting file ");
      }
      return 1;
    }
  }

  return 0;
}

// 0 ->no photo 1->sucesssfull
int gallery_get_photo_name(photo *head, uint32_t id_photo, char file_name[MAX_WORD_SIZE]){

  if(head == NULL){
    printf("Empty list\n");
    return(0);
  }

  photo *aux = head;

  while(aux != NULL){
    if(aux->identifier == id_photo){
      strcpy(file_name, aux->name);
      return(1);
    }
    aux = aux->next;
  }
  return(0);
}

// -1-> error opening file 0->no photo 1->sucesssfull, change return or parameters to send photo
int gallery_get_photo(photo* head, uint32_t id_photo, char *file_bytes, long *file_size){

  char file_name[MAX_WORD_SIZE];
  Message msg;

  if(head==NULL){
    printf("Empty list\n");
    return(0);
  }

  photo * aux = head;

  while(aux!=NULL){
    if(aux->identifier == id_photo){

      sprintf(file_name, "%d", (int)id_photo);

      FILE *fd = fopen(file_name,"r+");
      if(fd == NULL){
        perror("Photo file ");
        return (-1);
      }

      fseek(fd, 0, SEEK_END); //set stream pointer @fd to end-of-file
      *file_size = ftell(fd); //get fd current possition in stream
      memset(file_bytes,0,MAX_FILE_SIZE);

      msg.type = 0;
      strcpy(msg.payload, file_name);
      rewind(fd); //start reading file from the beginning
      fread(file_bytes, *file_size, 1, fd);
      fclose(fd);
      printf("found\n");
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

  printf("\nPhoto list: \n");
  photo* aux = head;
  keyword *aux2 = NULL;
  while (aux!=NULL) {
    printf("iden-> %d name-> %s keyword's:",aux->identifier,aux->name);
    aux2 = aux->key_header;
    if(aux2 != NULL){
      while(aux2 != NULL){
        printf(" %s", aux2->word);
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
