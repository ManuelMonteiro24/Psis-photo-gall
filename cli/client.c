#include "clientapi.h"

//arg -- gatewayaddress gatewayport

int main(int argc, char *argv[]){

    char buffer[BUFFERSIZE];
    int ret_aux;

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

    printf("Insert name of photo file to add to gallery: \n");
    fgets(buffer, BUFFERSIZE, stdin);

    uint32_t photo_id = gallery_add_photo(sock_fd,buffer);
    if(photo_id == 0){
      close(sock_fd);
      fprintf(stderr,"Photo insertion as failed!\n");
      exit(1);
    }

    //gallery_add_keyword example

    printf("Insert keyword to insert to the photo just inserted: \n");
    fgets(buffer, BUFFERSIZE, stdin);

    ret_aux =  gallery_add_keyword(sock_fd, photo_id, buffer);
    if(ret_aux == -1){
      close(sock_fd);
      fprintf(stderr,"Error duplicates or memory\n");
      exit(1);
    }else if(ret_aux == 0){
      printf("Photo to add the keyword not found\n");
    }else{
      printf("Keyword adding sucesssfull\n");
    }


    //gallery_search_photo example

    uint32_t *id_photos;

    printf("Insert keyword to receive the photos_id with that tag: \n");
    fgets(buffer, BUFFERSIZE, stdin);

    int photo_count = gallery_search_photo(sock_fd, buffer, &id_photos);
    if(photo_count == -1){
      close(sock_fd);
      fprintf(stderr,"Error Ocurred (invalid arguments, network problem or memory problem)\n");
      exit(1);
    }else if(photo_count == 0){
      printf("No photo in the server with that keyword\n");
    }else{
      printf("Number of photos with that keyword: %d\n", photo_count);
      printf("Photos identifiers: \n");

      //print photo_id TO CHECK...
      /*for(int i=0;i<photo_count;i++){
        printf("%d \n", id_photos[i]);
      }
      */
    }

    // gallery_get_photo_name example

    char * photo_name;

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

    //gallery_delete_photo example
    ret_aux = gallery_delete_photo(sock_fd, photo_id);
    if(ret_aux == 0){
      printf("Photo to delete not found\n");
    }else{
      printf("Remove sucesssfull\n");
    }

    // gallery_get_photo example

    printf("Insert name of file to receive the downloaded photo: \n");
    fgets(buffer, BUFFERSIZE, stdin);

    ret_aux = gallery_get_photo(sock_fd, photo_id, buffer);
    if(ret_aux == -1){
      close(sock_fd);
      fprintf(stderr,"Error Ocurred (invalid arguments, network problem or memory problem)\n");
      exit(1);
    }else if(ret_aux == 0){
      printf("No photo in the server with that identifier\n");
    }else{
      printf("Download sucesssfull photo saved in file: %s\n", buffer);
    }

    //try to disconnect
    while(1){

      //client doesnt leave rigth away dunno why?
      if(gallery_disconnect(sock_fd)==0){
        close(sock_fd);
        exit(0);
      }else{
        printf("Error disconnect\n");
      }
  }
}
