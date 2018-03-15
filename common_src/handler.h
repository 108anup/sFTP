#ifndef HANDLER_H
#define HANDLER_H

enum {PUT, GET};
#define FILE_CHUNK_BUFF_SIZE 1024
#define MAX_RETRIES 3
#define BUFF_SIZE 1024

void send_int_to_conn(int, int);
int get_int_from_conn(int);

#endif
