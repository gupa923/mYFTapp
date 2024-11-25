#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <arpa/inet.h>
#include <pthread.h>


typedef struct server_args {
    char *address;
    char *port;
    char *root_directory;
}server_args;

