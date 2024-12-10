#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <getopt.h>

/**
 * Questa struct contiene gli argomenti passati in input al client quando viene eseguito
 * @param operation: l'operazione da eseguire
 * @param server_address: l'indirizzo del server a cui connettersi
 * @param server_port: la porta del server a cui connettersi
 * @param f_path: il percorso indicato come argomento dell'opzione -f
 * @param o_path: il percorso indicato come argomento dell'opzione -o se presente
 */
typedef struct client_args{
    char operation; 
    char *server_address;
    char *server_port;
    char *f_path;
    char *o_path;
}client_args;

client_args THIS_ARGS;

/**
 * Questa struct contiene le informazioni da mandare al server prima di effettuare il trasferimento dei dati
 * @param flag: intero che quando è settato indica cheil client è pronto a inviare i dati
 * @param content_size: indica la dimensione in byte del file in modo che il server possa controllare se è effettivamente possibile scrivere il contenuto del file
 */
typedef struct write_request{
    int flag; //indica se la richiesta può procedere (1 se può procedere, 0 altrimenti)
    long content_size; //indicare la dimensione del contenuto del file in modo che il server può allocare le risorse necessarie
} write_request;

write_request W_REQUEST;

/**
 * Questa struct contiene le informazioni mandate dal server prima di inviare i dati in un operazione di read
 * @param flag: intero che se settato indica che il server è pronto a inviare dati
 * @param file_size: indica la dimensione in byte del file richiesto
 */
typedef struct read_header{
    int flag;
    long file_size;
} read_header;

read_header R_HEADER;

/**
 * Constante che viene messa nel campo o_path della struct client_args nell'opzione -l
 */
char *NO_O_PATH = "____";

/**
 * La funzione ha il compito di leggere e inviare il file che si vuole scrivere sul server
 * controlla che il file esista e vede quanto spazio occupa
 * aspetta che il server sia pronto a ricevere
 * invia i dati e aspetta una conferma di lettura, da parte del server
 * @param path: il percorso del file da inviare
 * @param client_fd: la socket del client
 */
void read_txt_file(char *path, int client_fd);

/**
 * La funzione controlla se il percorso inserito è il percorso di un file .txt
 * @param path: il percorso da analizzare
 * @return: 1 se è un path ad un file .txt 0 altrimenti
 */
int check_path(char *path);

/**
 * Funzione che gestisce l'esecuzione dell'opzione -w lato client
 */
void do_write(int client_fd);

/**
 * ricava dalla stringa in input i campi della struct read_header
 */
void get_read_header(char *header);

/**
 * Controlla se il path di destinazione dell'opzione -r è un percorso ad un file .txt. 
 * Se è un percorso ad un file .txt che ancora non esiste crea il file e le eventuali directory non esistenti
 * @return: 1 se il percorso è quello di un file .txt, 0 altrimenti
 */
int check_dest_path(char *path);

/**
 * Funzione che gestisce l'esecuzione dell'opzione -r lato client
 */
void do_read(int client_fd);

/**
 * Funzione che gestisce l'esecuzione dell'opzione -l lato client
 */
void do_list(int client_fd);

/**
 * Funzione che estrae gli argomenti in input e di coneguenza riempie i campi della struct THIS_ARGS e interrompe l'esecuzione nel caso in cui i parametri non siano corretti
 */
void parse_client_input(int argc, char **argv);