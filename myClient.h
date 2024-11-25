#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <getopt.h>

typedef struct client_args{
    char operation;
    char *server_address;
    char *server_port;
    char *f_path;
    char *o_path;
}client_args;