#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "client.h"
#include "handler.h"

int main(int argc, char *argv[]){
  if (argc != 3)
  {
    printf("\nUsage: client <ServerIP> <ServerPort>\n");
    return -1;
  }

  const int PORT = atoi(argv[2]);
  const char *ADDRESS = argv[1];

  char *msg = "Hello from Client";
  
  int sockfd = 0;
  struct sockaddr_in serv_addr;
  char buffer[1024];

  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    printf("\nError: Unable to create socket.\n");
    return -1;
  }
  
  memset(&serv_addr, '0', sizeof(serv_addr));
  memset(buffer, '\0', sizeof(buffer));
  
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(PORT);
      
  if (inet_pton(AF_INET, ADDRESS, &serv_addr.sin_addr)<=0) 
  {
    printf("\nInvalid IP Address.\n");
    return -1;
  }
  
  if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
  {
    printf("\nUnable to connect to server.\n");
    return -1;
  }
  
  send(sockfd, msg, strlen(msg), 0);
  printf("\nMessage sent to server\n");
  
  int readlen = read(sockfd, buffer, 1024);
  printf("\nGot from sever: %*s\n", readlen, buffer);
  
  return 0;
}
