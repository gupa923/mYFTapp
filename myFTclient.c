#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <getopt.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "myClient.h"

void read_txt_file(char *path, int client_fd){
    int nb_read;
    char w_header[256];
    FILE *file = fopen(path, "r");
    if (file == NULL){
        W_REQUEST.content_size = 0;
        sprintf(w_header, "%d:%ld", W_REQUEST.flag, W_REQUEST.content_size);
        printf("%s\n", w_header);
        if(write(client_fd, &w_header, sizeof(w_header)) < 0){
            perror("write error");
            return;
        }
        return;
        //invia messaggio di impossibilità di leggere
    }
    //calcolo dimensione del file in byte
    fseek(file, 0, SEEK_END);
    W_REQUEST.content_size = ftell(file);
    rewind(file);

    
    sprintf(w_header, "%d:%ld", W_REQUEST.flag, W_REQUEST.content_size);
    printf("%s\n", w_header);
    if(write(client_fd, &w_header, sizeof(w_header)) < 0){
        perror("write error");
        return;
    }

    char flag[2];
    read(client_fd, flag, sizeof(flag));

    if (flag[0] == '0'){
        perror("path inserito non valido");
        return;
    }
    //procedo a leggere il contenuto del file
    //char *content = (char *)malloc(((W_REQUEST.content_size+1) * sizeof(char)));
    char buffer[1024] = "";
    while ((nb_read = fread(buffer, 1, sizeof(buffer), file)) > 0){
        printf("%s", buffer);
        if (send(client_fd, buffer, nb_read, 0) == -1){
            perror("send non andata a buon fine");
        }
    }
    fclose(file);
    printf("ciao\n");

    printf("scrittura completa con successo\n");
}

//è sbagliat, non va bene
int check_path(char *path){
    //assumo che vengano inseriti o path relativi alla directory attuale o path assoluti

    //prima di tutto vedo se il path esiste
    struct stat file_stat;

    if (stat(path, &file_stat) != 0){
        perror("non è un file");
        return 0; //non è un path valido
    }

    if (!S_ISREG(file_stat.st_mode)){
        perror("non è un file regolare");
        return 0; //non è un file regolare
    }

    //controllo sia un file .txt
    char *ext = strrchr(path, '.'); //restituisce il puntatore all'ultima occorrenza di ".", così facendo ricavo l'estensione del file
    if(ext != NULL && strcmp(ext, ".txt") == 0){
        return 1; //è un file txt
    }
    perror("non è un file txt");
    return 0;
}

void do_write(int client_fd){
    /**
     * @todo controllare che il path indicato in f_path esista, in caso contrario invia messaggio al server e interrompi connessione
     * @todo leggere il file se esiste e inviare il contenuto al server
     * @todo attendere la risposta del server
     * @todo verificare se la richiesta ha avuto successo o meno
     * viene ammessa solo la lettura di file .txt
     */

    /**
     * L'idea è quella di creare una struct in cui si mettono i dati della richiesta, 
     * che vengono impostati ad un valore specifico se la richiesta deve essere abortita
     */
    
    W_REQUEST.flag = check_path(THIS_ARGS.f_path);

    
    read_txt_file(THIS_ARGS.f_path, client_fd);

}

void parse_client_input(int argc, char **argv){
    int op_flag = 0;
    int a_flag = 0, p_flag = 0, f_flag = 0, o_flag = 0;
    int opt;

    while ((opt = getopt(argc, argv, "wrla:p:f:o:")) != -1){
        switch (opt){
            case 'w':{
                op_flag++;
                THIS_ARGS.operation = 'w';
                break;
            }case 'r':{
                op_flag++;
                THIS_ARGS.operation = 'r';
                break;
            }case 'l':{
                op_flag++;
                THIS_ARGS.operation = 'l';
                break;
            }case 'a':{
                a_flag++;
                THIS_ARGS.server_address = optarg;
                break;
            }case 'p':{
                p_flag++;
                THIS_ARGS.server_port = optarg;
                break;
            }case 'f':{
                f_flag++;
                THIS_ARGS.f_path = optarg;
                break;
            }case 'o':{
                o_flag++;
                THIS_ARGS.o_path = optarg;
                break;
            }case '?':{
                perror("invalid operation or missing arguments");
                exit(EXIT_FAILURE);
            }
        }
    }

    if (op_flag != 1){
        perror("operation missing");
        exit(EXIT_FAILURE);
    }
    if (o_flag == 0){
        if (THIS_ARGS.operation == 'w' || THIS_ARGS.operation == 'r'){
            THIS_ARGS.o_path = THIS_ARGS.f_path;
        }else{
            //NO_O_PATH viene inserito solo in caso l'opzione sia la l
            THIS_ARGS.o_path = NO_O_PATH;
        }
    }
    if (a_flag != 1 || p_flag != 1 || f_flag != 1){
        perror("missing operation");
        exit(EXIT_FAILURE);
    }

}

int main(int argc, char *argv[]){
    parse_client_input(argc, argv);
    char *server_ip = THIS_ARGS.server_address;
    int server_port = atoi(THIS_ARGS.server_port);
    int client_fd; 
    struct sockaddr_in server_address;
    char first_message[256];
    sprintf(first_message,"%c:%s:%s",THIS_ARGS.operation, THIS_ARGS.f_path, THIS_ARGS.o_path);

    //creazione del client
    if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        perror("errore creazione del client");
        exit(-1);
    }else{
        printf("client creato correttamente\n");
    }

    //inserimento dati del server
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr(server_ip);
    server_address.sin_port = htons(server_port);

    //connessione al server
    if (connect(client_fd, (struct sockaddr * )&server_address, sizeof(server_address))== -1){
        perror("connessione fallita");
        close(client_fd);
        exit(-1);
    }else{
        printf("connessione al server riuscita\n");
    }

    if(write(client_fd, &first_message, sizeof(first_message)) < 0){
        perror("errore invio messaggio");
        exit(EXIT_FAILURE);
    }

    switch(THIS_ARGS.operation){
        case 'w':{
            do_write(client_fd);
            break;
        }case 'r':{
            //manage r
            break;
        }case 'l':{
            //manage list
            break;
        }
    }
    printf("connessione terminata\n");
    close(client_fd);
    return 0;
}