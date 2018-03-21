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
#include <signal.h>

#include "server.h"
#include "handler.h"

static int listenfd = -1;
static int connfd = -1;
static char *current;
static char recv_buffer[BUFF_SIZE];
static char send_buffer[BUFF_SIZE];
static int readlen = 0;

static char client_ip[INET_ADDRSTRLEN];
static int client_port = 0;

static volatile int keep_running = 1;

static void server_put(int num_files){

  int num_retry = 0;
  char fname[BUFF_SIZE];
  int file_size;
  int file_exists;

  for(int i = 0; i<num_files; i++){

    memset(fname, '\0', sizeof(fname));
    if((file_size = recv_file_metadata(connfd, fname)) <= 0){
      printf("\nClient unable to open file: %s\n", fname);
      continue;
    }
    
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

  int num_retry = 0;
  char fname[BUFF_SIZE];
  int file_on_server = 0;

  int fd;
  int file_name_size;
  int file_size;
  int conflict_choice = 0;
  
  for(int i = 0; i<num_files; i++){

    conflict_choice = get_int_from_conn(connfd);
    if(conflict_choice <= 0){
      printf("\nSkipping file.\n");
      continue;
    }

    file_name_size = get_int_from_conn(connfd);
    current = recv_buffer;
    readlen = read(connfd, recv_buffer, file_name_size);

    if(readlen < file_name_size){
      printf("\nError: Invalid protocol\n");
      return;
    }

    memcpy(fname, recv_buffer, file_name_size);
    fname[file_name_size] = '\0';
    printf("\nRecd file name: %s.\n", fname);
    
    if(access(fname, F_OK) != -1)
      file_on_server = 1;
    else
      file_on_server = 0;
    
    send_int_to_conn(connfd, file_on_server);
    if(file_on_server == 0){
      continue;
    }

    fd = open(fname, O_RDONLY);
    if ((file_size = send_file_metadata(fd, fname, connfd)) <= 0){
      printf("\nSkipping file: %s\n", fname);
      continue;
    }

    send_file(fd, fname, file_size, connfd, &i, &num_retry);
    close(fd);
  }
}

static void server_mget(void){

  int regex_length = get_int_from_conn(connfd);
  readlen = read(connfd, recv_buffer, regex_length);
  current = recv_buffer;

  char regex[BUFF_SIZE];
  memcpy(regex, recv_buffer, regex_length);
  regex[regex_length] = '\0';

  int num_files = 0;
  char *file_names[BUFF_SIZE];
  char cmd[BUFF_SIZE];
  sprintf(cmd, "find . -maxdepth 1 -type f -name \"%s\" -printf '%%P\n'",
          regex);

  //Get Files
  FILE *fp = popen(cmd, "r");
  if (fp == NULL) {
    printf("\nError: Failed to run find command.\n" );
    return;
  }

  char *buff = (char *) malloc(sizeof(char) * BUFF_SIZE);
  while(fscanf(fp, "%[^\n]%*c", buff) > 0){
    file_names[num_files++] = buff;
    buff = (char *) malloc(sizeof(char) * BUFF_SIZE);
  }
  pclose(fp);

  send_int_to_conn(connfd, num_files);

  //Send Files
  int fd;
  int file_size;
  
  int num_retry;
  char *fname = NULL;
  for(int i = 0; i<num_files; i++){
    fname = file_names[i];
    current = send_buffer;

    fd = open(fname, O_RDONLY);
    if ((file_size = send_file_metadata(fd, fname, connfd)) <= 0){
      printf("\nSkipping file: %s\n", fname);
      continue;
    }
    
    int conflict_choice = get_int_from_conn(connfd);
    if(conflict_choice <= 0){
      continue;
    }
    send_file(fd, fname, file_size, connfd, &i, &num_retry);
    close(fd);
  }
}

static void sigint_handler(int dummy) {
  keep_running = 0;
  printf("\nSIGINT invoked: Terminating the server.\n");

  if(connfd >= 0){
    printf("\nShutting down and closing connection.\n");
    shutdown(connfd, 2);
    close(connfd);
  }

  if(listenfd >= 0){
    int t = 1;
    printf("\nReleasing socket.\n");
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEPORT, &t, sizeof(int)) < 0){
      printf("\nUnable to gracefully terminate.\n");
    }
    shutdown(listenfd, 2);
    close(listenfd);
    printf("\nClosing socket.\n");
  }
}

int main(int argc, char *argv[]){
  if (argc != 2)
  {
    printf("\nUsage: server <ServerPort>\n");
    return -1;
  }

  signal(SIGINT, sigint_handler);

  char curr_dir[BUFF_SIZE];
  getcwd(curr_dir, sizeof(curr_dir));
  printf("\nWorking dir of server:\n\"%s\".\n", curr_dir);

  const int PORT = atoi(argv[1]);
  const int MAX_BACKLOG = 10;
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
    close(listenfd);
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
  
  while(keep_running){
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
    case MGET: server_mget(); break;
    default: printf("\nError: Invalid protocol.\n"); break;
    }

    shutdown(connfd, 2);
    close(connfd);
  }
  close(listenfd);
  return 0;
}
