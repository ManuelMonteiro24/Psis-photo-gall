#include "clientapi.h"

//arg -- gatewayaddress gatewayport

int main(int argc, char *argv[]){

    char buffer[BUFFERSIZE], aux[MAX_WORD_SIZE], cmd;
    char *photo_name;
    uint32_t *photos_id;
    int ret_aux, it, leave = 0;

    if (argc < 3) {
       fprintf(stderr,"usage gatewayhostname gatewayport\n");
       exit(1);
    }

    int sock_fd = gallery_connect(argv[1], atoi(argv[2]));
    if(sock_fd == 0){
      fprintf(stderr,"No peers available\n");
      exit(1);
    }
    if(sock_fd == -1){
      fprintf(stderr,"Gateway cannot be accessed\n");
      exit(1);
    }

    do{
      printf("\nCommands:\nd download\nu upload\ni get name by id\ns search by keyword\nk add keyword\nr remove photo\n");
      fgets(buffer, BUFFERSIZE, stdin);
      if(sscanf(buffer, "%c", &cmd) != 1){
        cmd = 'x'; //enter default
      }

      switch(cmd){
        case 'u':
          printf("Insert name of photo file to add to gallery: \n");
          fgets(buffer, BUFFERSIZE, stdin);

          uint32_t photo_id = gallery_add_photo(sock_fd,buffer);
          if(photo_id == 0){
            close(sock_fd);
            fprintf(stderr,"Photo insertion as failed!\n");
            exit(1);
          }else{
            printf("Photo_id from the added photo: %u\n", photo_id);
          }
          break;

        case 'd':
          printf("Insert name of file to receive the downloaded photo and photo id to download: \n");
          fgets(buffer, BUFFERSIZE, stdin);

          while(2 != sscanf(buffer, "%s %u", aux, (uint32_t*) &photo_id)){
            printf("ERROR: invalid input\n");
            fgets(buffer, BUFFERSIZE, stdin);
          }

          ret_aux = gallery_get_photo(sock_fd, photo_id, aux);
          if(ret_aux == -1){
            close(sock_fd);
            fprintf(stderr,"Error Ocurred (invalid arguments, network problem or memory problem)\n");
            exit(1);
          }else if(ret_aux == 0){
            printf("No photo in the server with that identifier\n");
          }else{
            printf("Download sucesssfull photo saved in file: %s\n", buffer);
          }
          break;

        case 'i':
          printf("Insert id of photo to get name: \n");
          fgets(buffer, BUFFERSIZE, stdin);
          if(1 != sscanf(buffer, "%u", (uint32_t*) &photo_id)){
            printf("ERROR: invalid input\n");
          }
          ret_aux = gallery_get_photo_name(sock_fd, photo_id, &photo_name);
          if(ret_aux == -1){
            close(sock_fd);
            fprintf(stderr,"Error Ocurred (invalid arguments, network problem or memory problem)\n");
            exit(1);
          }else if(ret_aux == 0){
            printf("No photo in the server with that identifier\n");
          }else{
            printf("Name of photo found: %s\n", photo_name);
          }
          break;

        case 's': //search photo by keyword
          printf("Search by keyword: \n");
          fgets(buffer, BUFFERSIZE, stdin);
          sscanf(buffer, "%s", aux);
          int photo_count = gallery_search_photo(sock_fd, aux, &photos_id);
          if(photo_count == -1){
            close(sock_fd);
            fprintf(stderr,"ERROR: invalid arguments, network problem or memory problem\n");
            exit(1);
          }else if(photo_count == 0){
            printf("No photo in the server with that keyword\n");
          }else{
            printf("Number of photos with that keyword: %d\n", photo_count);
            printf("Photo identifiers:\n");
            //print photo_id TO CHECK...
            for(it = 0;it < photo_count; it++){
              printf("%u\n", photos_id[it]);
            }
          }
          break;

        case 'k': //add keyword
          printf("Insert photo_id and keyword to insert to the photo: \n");
          fgets(buffer, BUFFERSIZE, stdin);

          while(2 != sscanf(buffer, "%u %s", (uint32_t*) &photo_id, aux)){
            printf("ERROR: invalid input\n");
            fgets(buffer, BUFFERSIZE, stdin);
          }

          ret_aux =  gallery_add_keyword(sock_fd, photo_id, aux);
          if(ret_aux == -1){
            close(sock_fd);
            fprintf(stderr,"ERROR: duplicates or memory\n");
            exit(1);
          }else if(ret_aux == 0){
            printf("ERROR: photo not found in server\n");
          }else{
            printf("Keyword added sucesssfully\n");
          }
          break;

        case 'r': //remove photo
          printf("Insert id of photo to delete: \n");
          fgets(buffer, BUFFERSIZE, stdin);
          if(1 != sscanf(buffer, "%u", (uint32_t*) &photo_id)){
            printf("ERROR: invalid input\n");
          } else {
            //gallery_delete_photo example
            ret_aux = gallery_delete_photo(sock_fd, photo_id);
            if(ret_aux == 0){
              printf("Photo to delete not found\n");
            }else{
              printf("Remove sucessfull\n");
            }
          }
          break;

        case 'q':
          leave = 1;
          break;
        default:
          printf("invalid command\n");
          break;
      }

    } while(leave != 1);


    if(gallery_disconnect(sock_fd) != 0){
      perror("On socket close ");
    }

  return 0;
}
