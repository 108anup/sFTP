#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <unistd.h>
#include <sys/socket.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "handler.h"

static char send_buffer[BUFF_SIZE];
static char *current = send_buffer;
static int readlen = 0;
static char recv_buffer[BUFF_SIZE];

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

/*
 * Packet Format:
 * Protcol, num files
*/
int send_protocol_header(int protocol, int sock_desc, int num_files){
  current = send_buffer;
  memcpy(current, &protocol, sizeof(int));
  current += sizeof(int);
  memcpy(current, &num_files, sizeof(int));

  send(sock_desc, send_buffer, sizeof(int) * 2, 0);
  printf("\nMessage (%d, %d) sent to server\n", protocol, num_files);

  int ack = get_int_from_conn(sock_desc);
  return ack;
}

/* Packet Format:
 * First sizeof(int) bytes: file name length
 * Next sizeof(int) bytes: file size
 * Next (file name length) bytes: file name
 */
int send_file_metadata(int fd, char *fname, int sock_desc){
  struct stat file_stat;

  if(fstat((int) fd, &file_stat) < 0)
  {
    printf("\nError: Unable to get file stats for: %s, errno: %d\n",
           fname, errno);
    return -1;
  }

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
  send(sock_desc, send_buffer, sizeof(int) * 2 + file_name_size, 0);
  printf("\nSending file metadata as:"
         " %s (%d) of size: %d\n", fname, file_name_size,
         (int) file_stat.st_size);
  return file_size;
}

void send_file(int fd, char *fname,
               int file_size, int sock_desc, int *i, int *num_retry){
  printf("\nSending file: %s of %d bytes.\n", fname, file_size);
  int read_bytes = 0;
  char file_buffer[FILE_CHUNK_BUFF_SIZE];
  int current_read = 0;
  while(read_bytes < file_size){
    current_read = read(fd, file_buffer, FILE_CHUNK_BUFF_SIZE);
    send(sock_desc, file_buffer, current_read, 0);
    read_bytes += current_read;
  }

  int got_full_file = get_int_from_conn(sock_desc);
  if(got_full_file <= 0){
    printf("\nUnable to send full file, retry attempt num: %d\n", *num_retry + 1);
    if(*num_retry < MAX_RETRIES){
      *num_retry++;
      *i--; //Retry
    }
  }
  else{
    printf("\nFile \"%s\" sent.\n", fname);
    *num_retry = 0;
  }
}

int recv_file_metadata(int sock_desc, char *fname){
  readlen = read(sock_desc, recv_buffer, sizeof(int) * 2);
  if(readlen < sizeof(int) * 2){
    printf("\nError: Invalid protocol\n");
    return -1;
  }

  int file_name_size;
  int file_size;

  current = recv_buffer;
  memcpy(&file_name_size, current, sizeof(int));
  current += sizeof(int);
  memcpy(&file_size, current, sizeof(int));

  readlen = read(sock_desc, recv_buffer, file_name_size);

  if(readlen < file_name_size){
    printf("\nError: Invalid protocol\n");
    return -1;
  }

  memcpy(fname, recv_buffer, file_name_size);
  printf("\nRecd file metadata: %s, %d.\n", fname, file_size);
  return file_size;
}

void recv_file(char *fname,
               int file_size, int sock_desc, int *i, int *num_retry){
  int fd = open(fname, O_WRONLY | O_CREAT);
  int read_bytes = 0;
  char file_buffer[FILE_CHUNK_BUFF_SIZE];
  int current_read = 0;
  while(read_bytes < file_size){
    current_read = read(sock_desc, file_buffer, FILE_CHUNK_BUFF_SIZE);

    if(current_read < 0){
      printf("\nError: Recieving file failed\n");
      break;
    }

    write(fd, file_buffer, current_read);
    read_bytes += current_read;
  }
  close(fd);

  printf("\nRecieved %d bytes of file %s.\n", read_bytes, fname);
    
  int got_full_file = 0;
  if(read_bytes == file_size)
  {
    *num_retry = 0;
    got_full_file = 1;
  }
  else
  {
    got_full_file = 0;
    if(*num_retry < MAX_RETRIES){
      *num_retry++;
      *i--; //Retry
    }
  }
  send_int_to_conn(sock_desc, got_full_file);
}
