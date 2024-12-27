#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <arpa/inet.h>
#include <pthread.h>

int MAX_CONCURRENT_CONNECTIONS= 8;

typedef struct MUTEX_LIST_RECORD{
    char *file_path;
    pthread_mutex_t file_mutex;
    struct MUTEX_LIST_RECORD *next;
}MUTEX_LIST_RECORD;

pthread_mutex_t *get_file_lock(char* file_name);

void free_mutex_list();

/**
 * Struct contenete gli argomenti in inputdel server
 */
typedef struct server_args {
    char *address; //indirizzo IP del server
    char *port; //numero di porta del server
    char *root_directory; //root directory del server
}server_args;

/**
 * Struct che contiene i campi necessari per eseguire la richiesta del client
 */
typedef struct client_request{
    char op_tag; //codice dell'operazione da eseguire
    char *f_path; 
    char *o_path;
}client_request;

/**
 * Contiene informazioni sul file che il client vuole scrivere sul server
 */
typedef struct write_header{
    int flag;  //1 se si può procedere con la scrittura
    long content_size; //contiene la dimensione in byte del file da scrivere
} write_header;

server_args FT_ARGS;


char *NO_O_PATH = "____";

/**
 * Funzione che ricava i campi della struct write_reader, passata in input, partendo dalla stringa passata in inut
 */
void get_write_header(write_header *header, char *h_string);

/**
 * verifica che il percorso su cui andare a scrivere il file in un'operazione di scrittura sia valido.
 * Se è il percorso ad un file .txt che non esiste allora il file viene creato.
 * @return restituisce 1 se il percorso è valido, 0 altrimenti
 */
int verify_wpath(char *o_path);

/**
 * Funzione che gestisce l'esecuzione dell'opzione -w lato server
 */
void do_write(int *client_fd, client_request *request);

/**
 * Questa funzione controlla se il file richiesto dall'operazione -r esiste
 * @return restituisce 1 se il file esiste ed è un file .txt, restituisce 0 altrimenti
 */
int file_exists(char *path);

/**
 * Gestisce l'esecuzione dell'operazione -r lato server
 */
void do_read(int *client_fd, client_request *request);

/**
 * Gestisce l'esecuzione dell'opzione -l lato server
 */
void do_list(int *client_fd, client_request *request);

/**
 * Data una stringa inviata dal client, ricava i campi della struct client_request per procedere con la corretta esecuzione della richiesta
 */
void get_client_request(char *content, client_request *request);

/**
 * Funzione passata come argomento a pthread_create. Riceve il primo messaggio del client contenente la richiesta sotto forma di stringa.
 * Analizza la richiesta chiamando la funzione get_client_request e poi gestisce l'esecuzione della richiesta
 */
void *accettazione_client(void *args);

/**
 * Funzione che controlla se il path inserito come root directory del server è valido. Controlla poi se tale path esiste e in caso contrario lo crea
 */
void valida_root(char *root_dir);

/**
 * Funzione che processa parametri in ingresso del programma
 */
void parse_input(int argc, char **argv);