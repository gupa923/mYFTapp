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


server_args FT_ARGS;

//LA CONCORRENZA IN TEORIA FUNZIONA MA SAREBBE MEGLIO MODIFICARLA LO FARO? ALLA FINE
int MAX_CONCURRENT_CONNECTIONS= 3;
pthread_t threads[3];


void *accettazione_client(void *args);

void valida_root(char *root_dir);

void parse_input(int argc, char **argv);