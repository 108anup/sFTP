#include <stdio.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include "handler.h"

void send_int_to_conn(int sock_desc, int val){
  send(sock_desc, &val, sizeof(int), 0);
  printf("\nMessage sent: %d\n", val);
}

int get_int_from_conn(int sock_desc){
  int val = -1;
  int readlen = read(sock_desc, &val, sizeof(int));
  if(readlen < sizeof(int)){
    printf("\nError: Party not responding or protocol not being followed.\n");
  }
  printf("\nGot value: %d\n", val);
  return val;
}
