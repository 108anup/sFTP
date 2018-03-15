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

#include "server.h"
#include "handler.h"

static int connfd = 0;
static char recv_buffer[BUFF_SIZE];
static int readlen = 0;

static char client_ip[INET_ADDRSTRLEN];
static int client_port = 0;

static void server_put(int num_files){

  int num_retry = 0;
  char fname[BUFF_SIZE];
  int file_size;
  int file_exists;

  for(int i = 0; i<num_files; i++){

    memset(fname, '\0', sizeof(fname));
    file_size = recv_file_metadata(connfd, fname);
    if(access(fname, F_OK) != -1)
      file_exists = 1;
    else
      file_exists = 0;
    
    send_int_to_conn(connfd, file_exists);
    if(file_exists == 1){
      int choice = get_int_from_conn(connfd);
      if(choice <= 0)
        continue;
    }

    recv_file(fname, file_size, connfd, &i, &num_retry);
  }
}

static void server_get(int num_files){
}

int main(int argc, char *argv[]){
  if (argc != 2)
  {
    printf("\nUsage: server <ServerPort>\n");
    return -1;
  }

  char curr_dir[BUFF_SIZE];
  getcwd(curr_dir, sizeof(curr_dir));
  printf("\nWorking dir of server:\n\"%s\".\n", curr_dir);

  const int PORT = atoi(argv[1]);
  const int MAX_BACKLOG = 10;
  int listenfd = 0;
  struct sockaddr_in serv_addr;
  memset(&serv_addr, '0', sizeof(serv_addr));
  memset(recv_buffer, '\0', sizeof(recv_buffer));
  
  if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    printf("\nError: Unable to create socket.\n");
    return -1;
  }
  
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons(PORT); 

  if (bind(listenfd, (struct sockaddr*) &serv_addr,
           (socklen_t) sizeof(serv_addr)) < 0)
  {
    printf("\nError: Bind Failed, descriptors "
           "incorrect or address/port already in use.\n");
    return -1;
  }
  
  if (listen(listenfd, MAX_BACKLOG) < 0)
  {
    printf("\nError: Listen Failed, descriptors "
           "incorrect or address/port already in use.\n");
    return -1;
  }

  struct sockaddr_in client_address;
  socklen_t addrlen = sizeof((struct sockaddr *) &client_address);
  
  while(true){
    if ((connfd = accept(listenfd, (struct sockaddr *) &client_address,
                         &addrlen)) < 0)
    {
      printf("\nError: Unable to accept connection, errcode: %d.\n", errno);
      continue;
    }

    memset(recv_buffer, '\0', sizeof(recv_buffer));
    readlen = read(connfd, recv_buffer, sizeof(int) * 2);

    if(readlen < sizeof(int) * 2){
      printf("\nError: Client not following protocol\n");
      continue;
    }
    
    int protocol;
    int num_files;
    char *current = recv_buffer;
    
    memcpy(&protocol, current, sizeof(int));
    current += sizeof(int);
    memcpy(&num_files, current, sizeof(int));

    inet_ntop(AF_INET, &(client_address.sin_addr), client_ip, INET_ADDRSTRLEN);
    client_port = ntohs(client_address.sin_port);
    printf("\nGot %d bytes from %s:%d as: (%d, %d)\n", readlen, client_ip,
           client_port, protocol, num_files);

    //Send ACK
    send_int_to_conn(connfd, 1);

    switch(protocol){
    case PUT: server_put(num_files); break;
    case GET: server_get(num_files); break;
    default: printf("\nError: Invalid protocol.\n"); break;
    }
    
    close(connfd);
  }
  
  return 0;
}
