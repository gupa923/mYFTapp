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


client_args THIS_ARGS;

typedef struct write_request{
    int flag; //indica se la richiesta può procedere (1 se può procedere, 0 altrimenti)
    long content_size; //indicare ladimensione del contenuto del file in modo che il server può allocare le risorse necessarie
} write_request;

write_request W_REQUEST;

char *NO_O_PATH = "____";

void do_write(int client_fd);

void parse_client_input(int argc, char **argv);

void parse_client_input(int argc, char **argv);