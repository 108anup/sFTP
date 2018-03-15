#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "client.h"
#include "handler.h"
#include "cli.h"

static int sockfd = 0;
static struct sockaddr_in serv_addr;

static char send_buffer[BUFF_SIZE];
static char *current = send_buffer;
  

static int PORT = 0;
static char *ADDRESS = "";

static int init_sockets(void){
  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    printf("\nError: Unable to create socket.\n");
    return -1;
  }
  
  memset(&serv_addr, '0', sizeof(serv_addr));
  
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(PORT);
      
  if (inet_pton(AF_INET, ADDRESS, &serv_addr.sin_addr) <= 0) 
  {
    printf("\nInvalid IP Address.\n");
    return -1;
  }
  
  if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
  {
    printf("\nUnable to connect to server.\n");
    return -1;
  }
  
  return 0;
}

static int handle_conflict(int sock_desc){
  int choice = 0;
  printf("\nFile with same name already exists (0: Skip, 1: Overwrite)\n");
  scanf("%d", &choice);

  send(sock_desc, &choice, sizeof(int), 0);
  return choice;
}

/*
 * Protocol Header
 * For each file:
 * * Send file name size, file size, file name
 * * Resolve conflicts if any
 * * Send/Recv file
 */
void put(int argc, char *argv[]){
  if (init_sockets() < 0){
    printf("\nError: unable to initialize sockets.\n");
    return;
  }  
  if (send_protocol_header(PUT, sockfd, argc - 1) != 1)
    return;
  
  int fd;
  int num_retry = 0;
  int file_size = 0;

  char *fname = NULL;
  for(int i = 1; i<argc; i++){
    fname = argv[i];
    current = send_buffer;

    fd = open(fname, O_RDONLY);
    if(fd < 0)
    {
      printf("\nError: Unable to open file: %s\n", fname);
      /* file_name_size = 0; */
      /* memcpy(current, &file_name_size, sizeof(int)); */
      /* current += sizeof(int); */
      /* memcpy(current, &file_name_size, sizeof(int)); */

      /* send(sock_desc, send_buffer, sizeof(int) * 2, 0); */
      /* printf("\nSkipping file: %s\n", fname); */
      continue;
    }
    if ((file_size = send_file_metadata(fd, fname, sockfd)) < 0)
      continue;
    
    int file_exists;
    if((file_exists = get_int_from_conn(sockfd)) == -1){
      return;
    }
    if(file_exists && (handle_conflict(sockfd) == 0)){
      continue;
    }
    send_file(fd, fname, file_size, sockfd, &i, &num_retry);
    close(fd);
  }
  
  close(sockfd);
}

void mput(int argc, char *argv[]){
  int num_files = 1;
  char *file_names[SIZE];
  char cmd[BUFF_SIZE];
  sprintf(cmd, "find . -maxdepth 1 -type f -name \"%s\" -printf '%%P\n'",
          argv[1]);

  FILE *fp = popen(cmd, "r");
  if (fp == NULL) {
    printf("\nError: Failed to run find command.\n" );
    return;
  }

  char *buff = (char *) malloc(sizeof(char)*SIZE);
  while(fscanf(fp, "%[^\n]%*c", buff) > 0){
    file_names[num_files++] = buff;
    buff = (char *) malloc(sizeof(char)*SIZE);
  }
  pclose(fp);
  put(num_files, file_names);
}

void get(int argc, char *argv[]){
  if (init_sockets() < 0){
    printf("\nError: unable to initialize sockets.\n");
    return;
  }  
  if (send_protocol_header(GET, sockfd, argc - 1) != 1)
    return;
  
  int num_retry = 0;
  int file_size = 0;
  char *fname = NULL;
  
  char temp[BUFF_SIZE];

  int file_exists = 0;
  int file_on_server = 0;
  int file_name_size = 0;
  
  for(int i = 1; i<argc; i++){
    fname = argv[i];
    current = send_buffer;
    
    if(access(fname, F_OK) != -1)
      file_exists = 1;
    else
      file_exists = 0;
    
    if(file_exists && (handle_conflict(sockfd) == 0)){
      printf("\nSkipping file: %s\n", fname);
      continue;
    }
    else if(!file_exists){
      send_int_to_conn(sockfd, 1);
    }
    
    // Sending file name
    file_name_size = strlen(fname);
    memcpy(current, &file_name_size, sizeof(int));
    current += sizeof(int);
    memcpy(current, fname, file_name_size);
    send(sockfd, send_buffer, sizeof(int) + file_name_size, 0);
    printf("\nSending file name as:"
           " %s (%d)\n", fname, file_name_size);
    
    if((file_on_server = get_int_from_conn(sockfd)) == -1){
      return;
    }
    if(!file_on_server){
      printf("\nError: File: %s does not exist on server\n", fname);
      continue;
    }

    file_size = recv_file_metadata(sockfd, temp);
    recv_file(fname, file_size, sockfd, &i, &num_retry);
  }
  close(sockfd);
}

void mget(int argc, char *argv[]){
  if (init_sockets() < 0){
    printf("\nError: unable to initialize sockets.\n");
    return;
  }  
  if (send_protocol_header(MGET, sockfd, argc - 1) != 1)
    return;
  
}

int main(int argc, char *argv[]){
  if (argc != 3)
  {
    printf("\nUsage: client <ServerIP> <ServerPort>\n");
    return -1;
  }

  PORT = atoi(argv[2]);
  ADDRESS = argv[1];

  init_cli();
  cli();
  
  return 0;
}
