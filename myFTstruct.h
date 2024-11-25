#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

typedef struct server_args {
    char *address;
    char *port;
    char *root_directory;
}server_args;

typedef struct client_args{
    char operation;
    char *server_address;
    char *server_port;
    char *f_path;
    char *o_path;
}client_args;

typedef struct client_first_message{
    char op_tag;
    char *f_path;
    char *o_path;
}client_first_message;
