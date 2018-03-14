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
  }  
  
  char *msg = "Hello from Client";
  char buffer[1024];
  memset(buffer, '\0', sizeof(buffer));
  
  send(sockfd, msg, strlen(msg), 0);
  printf("\nMessage sent to server\n");
  
  int readlen = read(sockfd, buffer, 1024);
  printf("\nGot from sever: %*s\n", readlen, buffer);

  close(sockfd);
}

void mput(int argc, char *argv[]){
  char *msg = "Hello from Client";
  char buffer[1024];
  memset(buffer, '\0', sizeof(buffer));
  
  send(sockfd, msg, strlen(msg), 0);
  printf("\nMessage sent to server\n");
  
  int readlen = read(sockfd, buffer, 1024);
  printf("\nGot from sever: %*s\n", readlen, buffer);
}

void get(int argc, char *argv[]){
  char *msg = "Hello from Client";
  char buffer[1024];
  memset(buffer, '\0', sizeof(buffer));
  
  send(sockfd, msg, strlen(msg), 0);
  printf("\nMessage sent to server\n");
  
  int readlen = read(sockfd, buffer, 1024);
  printf("\nGot from sever: %*s\n", readlen, buffer);
}

void mget(int argc, char *argv[]){
  char *msg = "Hello from Client";
  char buffer[1024];
  memset(buffer, '\0', sizeof(buffer));
  
  send(sockfd, msg, strlen(msg), 0);
  printf("\nMessage sent to server\n");
  
  int readlen = read(sockfd, buffer, 1024);
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
