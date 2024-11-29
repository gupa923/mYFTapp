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

typedef struct client_request{
    char op_tag;
    char *f_path;
    char *o_path;
}client_request;

typedef struct write_header{
    int flag;  //1 se si può procedere con la scrittura
    long content_size;
} write_header;

void get_client_request(char *content, client_request *request);
server_args FT_ARGS;

//LA CONCORRENZA IN TEORIA FUNZIONA MA SAREBBE MEGLIO MODIFICARLA LO FARO? ALLA FINE
int MAX_CONCURRENT_CONNECTIONS= 3;
pthread_t threads[3];

char *NO_O_PATH = "____";

void do_write(int *client_fd, client_request *request);

void *accettazione_client(void *args);

void valida_root(char *root_dir);

void parse_input(int argc, char **argv);