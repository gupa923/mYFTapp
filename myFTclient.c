#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <getopt.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/statvfs.h>
#include "myClient.h"

int is_free_mem(char *file, long size){
    struct statvfs system_info;

    if (statvfs(file, &system_info) != 0){
        perror("errore in statvfs");
        return 0;
    }

    long long available_space = system_info.f_bavail * system_info.f_frsize;

    if (available_space - size <= 0){
        return 0;
    }
    return 1;
}

void read_txt_file(char *path, int client_fd){
    int nb_read;

    //vedo se il file esiste e in tal caso calcolo in peso in byte
    char w_header[256];
    FILE *file = fopen(path, "r");
    if (file == NULL){
        W_REQUEST.content_size = 0;
        sprintf(w_header, "%d:%ld", W_REQUEST.flag, W_REQUEST.content_size);
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

    //invio il risultato al server
    sprintf(w_header, "%d:%ld", W_REQUEST.flag, W_REQUEST.content_size);
    if(write(client_fd, &w_header, sizeof(w_header)) < 0){
        perror("write error");
        return;
    }

    //aspetto cheil server sia pronto a ricevere i dati
    char flag[2];
    if (read(client_fd, flag, sizeof(flag)) < 0){
        perror("impossibile ricevere i dati");
        return;
    }
    //se il flag è 0 allora il server non può ricevere i dati e la connessione va terminata
    if (flag[0] == '0'){
        perror("path inserito non valido");
        return;
    }
    //se invece il server è pronto a ricevere(flag == 1), procedo a leggere il contenuto del file
    char buffer[1024] = ""; //buffer temporaneo dove salvare il contenuto del file
    while ((nb_read = fread(buffer, 1, sizeof(buffer), file)) > 0){ //leggo il file riga per riga 
        if (write(client_fd, buffer, nb_read) == -1){ //invio ogni riga al server
            perror("send non andata a buon fine");
        }
    }
    fclose(file);

    //mando un segnale di interruzione delle scrittura
    shutdown(client_fd, SHUT_WR);

    //aspetto il messaggio di conferma della ricezione da parte del server
    char response[2];
    if (read(client_fd, response, sizeof(response)) < 0){
        perror("impossibile ricevere risposta dal server");
        return;
    }

    //chiudo la connessione
    if (response[0] == '0'){
        perror("impossibile completare la scrittura");
        return;
    }
    printf("scrittura completa con successo\n");
}

int check_path(char *path){
    //assumo che vengano inseriti o path relativi alla directory attuale o path assoluti

    //vedo se il path esiste
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
     * L'idea è quella di creare una struct in cui si mettono i dati della richiesta, 
     * che vengono impostati ad un valore specifico se la richiesta deve essere abortita
     */
    
    W_REQUEST.flag = check_path(THIS_ARGS.f_path);

    read_txt_file(THIS_ARGS.f_path, client_fd);

}

void get_read_header(char *header){
    char *local_copy = malloc(sizeof(char)*strlen(header));
    strcpy(local_copy, header);
    char *save_ptr;
    char *token = strtok_r(local_copy, ":", &save_ptr);
    int i = 0;
    while(token != NULL){
        if (i == 0){
            R_HEADER.flag = atoi(token);
            token = strtok_r(NULL, ":", &save_ptr);
            i++;
        }else if (i == 1){
            R_HEADER.file_size = strtol(token, NULL, 10);
            token = strtok_r(NULL, ":", &save_ptr);
            i++;
        }
    }
}

int check_dest_path(char *path){
    //prima vedo se il path fornito è quello di un file txt
    char *ext = strrchr(path, '.');
    if (ext == NULL || strcmp(ext, ".txt") != 0){
        return 0; //il path fornito non è quello di un file
    }

    //visto che il path è quello di un file .txt vedo se le cartelle precedenti esistono e in caso contrario le creo
    char *temp = (char *)malloc((strlen(path) + 1) * sizeof(char));
    if (temp == NULL){
        perror("impossibile allocare la memoria");
        return 0;
    }
    strcpy(temp, path);
    char *save_ptr;
    char *token = strtok_r(temp, "/", &save_ptr);
    char current_dir[1024] = "";
    while (token != NULL){
        //assemblo la directori da analizzare con quelle già analizzateper costruire il path corretto
        strcat(current_dir, token);
        strcat(current_dir, "/");
        //controllo se mi trovo all'ultimo token, ovvero quello che corrisponde al nome del file. Se è l'ultimo allora next_token sarà NULL
        char *next_token = strtok_r(NULL, "/", &save_ptr);
        if (next_token != NULL){
            if (access(current_dir, F_OK) != 0){ //verifico se la directory corrente esiste ( == 0). Se non esiste la creo
                if (mkdir(current_dir, 0777) != 0){ //creo la directory
                    perror("errore nella creazione della directory di destinazione");
                    return 0;
                }else{
                    //se entra qui vuol dire che la directory è stata creata con successo
                }
            }else{
                //se entra qui vuol dire che la directory già esisteva
            }
        }
        token = next_token;
    }
    free(temp);
    return 1;
}

void do_read(int client_fd){

    //ricavo l'header dai dati inviati dal server
    char header[256];
    if (read(client_fd, header, sizeof(header)) < 0){
        perror("impossibile ricevere i dati dal client");
        return;
    }
    get_read_header(header);
    //controllare l'o_path ovvero il percorso di destinazione
    int flag = check_dest_path(THIS_ARGS.o_path);

    //invio al server un segnale per indicare che il client è pronto o meno a ricevere i dati
    char send_signal[2];
    int is_space = is_free_mem(THIS_ARGS.o_path, R_HEADER.file_size);
    if (flag != 1 || is_space != 1){
        flag = 0;
    }
    sprintf(send_signal, "%d", flag);
    if(write(client_fd, send_signal, sizeof(send_signal)) < 0){
        perror("errore nella write");
        return;
    }
    //se il percorso di destinazione non è valido allora il client interrompe la connessione
    if (flag == 0){
        perror("path di destinazione non valido");
        return;
    }
    printf("path di destinazione valido\n");

    //ricevo i dati inviati dal server
    int bytes_recived;
    char buffer[1024] = "";  //buffer temporaneo
    char *content = (char *)malloc(R_HEADER.file_size);  //stringa che conterra l'intero contenuto del file ricevuto
    if (content == NULL){
        perror("impossibile allocare la memoria");
        return;
    }
    memset(content, 0, R_HEADER.file_size * sizeof(char));

    FILE *fp = fopen(THIS_ARGS.o_path, "w+");
    if (fp == NULL){
        perror("impossibile aprire il file");
        return;
    }
    while((bytes_recived = read(client_fd, buffer, sizeof(buffer))) > 0){ //ricevo i dati sul buffer temporaneo
        strcat(content, buffer); //concateno il buffer con content 
    }

    //scrivo content sul file di output
    char response[] = "1"; //flag  che viene impostato a 0 se la scrittura su file fallisce
    if (fputs(content, fp) == EOF){
        perror("errore di scrittura");
        response[0] = '0';
        return;
    }
    free(content);
    fclose(fp);

    //invio al server un messaggio di conferma per indicare selarichiesta è stata completata correttamente
    if (write(client_fd, response, sizeof(response)) < 0){
        perror("impossibile inviare una risposta al server");
        return;
    }

    //chiudo la connessione
    if (response[0] == '0'){
        perror("impossibile completare la lettura");
        return;
    }
    printf("richiesta di lettura completata\n");
    return;
}

void do_list(int client_fd){
    
    //attende che il server controlli se la directory richiesta esiste
    char flag[2];
    if (read(client_fd, flag, sizeof(flag)) < 0){
        perror("impossibile ricevere risposta dal client");
        return;
    }
     //se non esiste chiude la connessione
    if (flag[0] == '0'){
        printf("percorso inserito non valido\n");
        return;
    }

    //se esiste stampa ogni elemento presente nella directory che viene inviato dal server
    char buffer[1024];
    int bytes_recived;
    int counter = 0;
    while ((bytes_recived = read(client_fd, buffer, sizeof(buffer))) > 0){
        printf("%s\n", buffer);
        counter++;
    }

    //se non viene inviato niente allora la directory è vuota
    if (counter == 0){
        printf("directory vuota\n");
        return;
    }

    //la connessione viene chiusa
    printf("richiesta di listing completata con successo\n");
    return;
}

void parse_client_input(int argc, char **argv){
    int op_flag = 0; //contatore che indica se è stato inserito il coice dell'operazione da eseguire
    int a_flag = 0, p_flag = 0, f_flag = 0, o_flag = 0; //indicano se sono stati inseriti gli argomenti richiesti
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

    //se non è stata inserita un operazione oppure se ne sono state inserite troppe interrompe l'esecuzione
    if (op_flag != 1){
        perror("operation missing");
        exit(EXIT_FAILURE);
    }

    //se non viene inserito un o_path, se l'opzione è -w o -r allora o_path viene impostato uguale a f_pah, altrimenti assume un valore di default che verra trascurato
    if (o_flag == 0){
        if (THIS_ARGS.operation == 'w' || THIS_ARGS.operation == 'r'){
            THIS_ARGS.o_path = THIS_ARGS.f_path;
        }else{
            //NO_O_PATH viene inserito solo in caso l'opzione sia la l
            THIS_ARGS.o_path = NO_O_PATH;
        }
    }
    //se non ci sono le opzione -a, -p e -f allora l'esecuzione viene interrotta
    if (a_flag != 1 || p_flag != 1 || f_flag != 1){
        perror("missing operation");
        exit(EXIT_FAILURE);
    }

}

int main(int argc, char *argv[]){
    //controllo gli argomenti in input
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
            do_read(client_fd);
            //manage r
            break;
        }case 'l':{
            do_list(client_fd);
            //manage list
            break;
        }
    }
    printf("connessione terminata\n");
    close(client_fd);
    return 0;
}