#ifndef HANDLER_H
#define HANDLER_H

enum {PUT, GET};
#define FILE_CHUNK_BUFF_SIZE 1024
#define MAX_RETRIES 3
#define BUFF_SIZE 1024

void send_int_to_conn(int, int);
int get_int_from_conn(int);
int send_protocol_header(int, int, int);
int send_file_metadata(int, char *, int);
void send_file(int, char *, int, int, int *, int *);
int recv_file_metadata(int, char *);
void recv_file(char *, int, int, int *, int *);

#endif
