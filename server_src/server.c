#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "server.h"
#include "handler.h"

int main(int argc, char *argv[]){
  if (argc != 2)
  {
    printf("\nUsage: server <ServerPort>\n");
    return -1;
  }

  const int PORT = atoi(argv[1]);
  const int MAX_BACKLOG = 10;
  int listenfd = 0;
  struct sockaddr_in serv_addr;
  memset(&serv_addr, '0', sizeof(serv_addr));

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

  int connfd = 0;
  struct sockaddr_in client_address;
  socklen_t addrlen;
  char buffer[1024];
  memset(buffer, '\0', sizeof(buffer));
  int readlen = 0;
  char client_ip[INET_ADDRSTRLEN];
  int client_port = 0;
  
  char *msg = "Hello from Server";
  
  while(true){
    if ((connfd = accept(listenfd, (struct sockaddr *) &client_address,
                         &addrlen)) < 0)
    {
      printf("\nError: Unable to accept connection.\n");
      continue;
    }
    readlen = read(connfd, buffer, 1024);
    inet_ntop(AF_INET, &(client_address.sin_addr), client_ip, INET_ADDRSTRLEN);
    client_port = ntohs(client_address.sin_port);
    printf("\nGot %d bytes from %s:%d as: %*s\n", readlen, client_ip,
           client_port, readlen, buffer);
    
    send(connfd, msg, strlen(msg) , 0);
    printf("\nMessage sent to %s:%d\n", client_ip, client_port);
  }
  
  return 0;
}
