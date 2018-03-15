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

void put(int argc, char *argv[]){

  if (init_sockets() < 0){
    printf("\nError: unable to initialize sockets.\n");
    return;
  }  

  /*
   * Protocol, Number of Files
   * For each file:
   * * Send file name size, file size, file name
   * * Resolve conflicts if any
   * * Send/Recv file
   */

  // PROTCOL HEADER
  char send_buffer[BUFF_SIZE];
  char *current = send_buffer;
  int protocol = PUT;
  int num_files = argc - 1;
  //(MAX_NUM_FILES = (2^32 -1))

  char buffer[BUFF_SIZE];
  
  memcpy(current, &protocol, sizeof(int));
  current += sizeof(int);
  memcpy(current, &num_files, sizeof(int));

  send(sockfd, send_buffer, sizeof(int) * 2, 0);
  printf("\nMessage \"%d, %d\" sent to server\n", protocol, num_files);

  int ack = get_int_from_conn(sockfd);
  if(ack != 1){
    return;
  }
  
  char *fname;
  int fd;
  struct stat file_stat;
  int num_retry = 0;
  
  for(int i = 1; i<argc; i++){
    fname = argv[i];
    fd = open(fname, O_RDONLY);

    if(fd < 0)
    {
      printf("\nError: Unable to open file: %s\n", fname);
      continue;
    }

    if(fstat((int) fd, &file_stat) < 0)
    {
      printf("\nError: Unable to get file stats for: %s, errno: %d\n",
             fname, errno);
      continue;
    }

    /* Packet Format:
     * First sizeof(int) bytes: file name length
     * Next sizeof(int) bytes: file size
     * Next (file name length) bytes: file name
     */

    //FILE META DATA
    current = send_buffer;
    int file_name_size = strlen(fname);
    int file_size = file_stat.st_size;
    
    memcpy(current, &file_name_size, sizeof(int));
    current += sizeof(int);
    memcpy(current, &file_size, sizeof(int));
    current += sizeof(int);
    memcpy(current, fname, file_name_size);

    // Sending file metadata
    send(sockfd, send_buffer, sizeof(int)*2 + file_name_size, 0);
    printf("\nSending file metadata as:"
           " %s (%d) of size: %d\n", fname, file_name_size,
           (int) file_stat.st_size);

    int file_exists;
    file_exists = get_int_from_conn(sockfd);

    if(file_exists == -1){
      return;
    }
    else if(file_exists == 1){
      int choice = 0;
      printf("\nFile with same name already exists (0: Skip, 1: Overwrite)\n");
      scanf("%d", &choice);

      send(sockfd, &choice, sizeof(int), 0);
      if(choice == 0){
        continue;
      }
    }
    
    // Send file
    int read_bytes = 0;
    char file_buffer[FILE_CHUNK_BUFF_SIZE];
    int current_read = 0;
    while(read_bytes < file_size){
      current_read = read(fd, file_buffer, FILE_CHUNK_BUFF_SIZE);
      send(sockfd, file_buffer, current_read, 0);
      read_bytes += current_read;
    }
    close(fd);
    printf("\nFile \"%s\" sent.\n", fname);

    int got_full_file = get_int_from_conn(sockfd);
    if(got_full_file <= 0){
      if(num_retry < MAX_RETRIES){
        num_retry++;
        i--; //Retry
      }
    }
    else
      num_retry = 0;
  }
  
  close(sockfd);
}

void mput(int argc, char *argv[]){
  char *msg = "Hello from Client";
  char buffer[BUFF_SIZE];
  memset(buffer, '\0', sizeof(buffer));
  
  send(sockfd, msg, strlen(msg), 0);
  printf("\nMessage sent to server\n");
  
  int readlen = read(sockfd, buffer, BUFF_SIZE);
  printf("\nGot from sever: %*s\n", readlen, buffer);
}

void get(int argc, char *argv[]){
  char *msg = "Hello from Client";
  char buffer[BUFF_SIZE];
  memset(buffer, '\0', sizeof(buffer));
  
  send(sockfd, msg, strlen(msg), 0);
  printf("\nMessage sent to server\n");
  
  int readlen = read(sockfd, buffer, BUFF_SIZE);
  printf("\nGot from sever: %*s\n", readlen, buffer);
}

void mget(int argc, char *argv[]){
  char *msg = "Hello from Client";
  char buffer[BUFF_SIZE];
  memset(buffer, '\0', sizeof(buffer));
  
  send(sockfd, msg, strlen(msg), 0);
  printf("\nMessage sent to server\n");
  
  int readlen = read(sockfd, buffer, BUFF_SIZE);
  printf("\nGot from sever: %*s\n", readlen, buffer);
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
