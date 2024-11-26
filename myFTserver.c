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
#include "myServer.h"

void get_client_request(char *content, client_request *request){
    char *loca_copy = malloc(sizeof(char)*strlen(content));
    strcpy(loca_copy, content);
    char **save_ptr;
    char *token = __strtok_r(loca_copy, ":", &loca_copy);
    int i = 0;
    while(token != NULL){
        if (i == 0){
            request->op_tag = token[0];
            token = __strtok_r(NULL, ":", &loca_copy);
            i++;
        }else if (i == 1){
            strcpy(request->f_path, token);
            token = __strtok_r(NULL, ":", &loca_copy);
            i++;
        }else{
            strcpy(request->o_path, token);
            token = __strtok_r(NULL, ":", &loca_copy);
            i++;
        }
    }
    //NON HO BISOGNO DI CONTROLLARE SE LA LETTURA E' aANDATA A BUON FINE POICHÈ HO LA GARANZIA CHE IL CLIENT NON SI CONNETTA SE LA RICHIESTA E' SBAGLIATA
}

void *accettazione_client(void *args){
    printf("client accettato\n");
    char message[256];
    int *client_fd = (int *)args;
    client_request request;

    if(read(*client_fd, &message, sizeof(message)) < 0){
        perror("impossibi leleggere il messaggio");
        close(*client_fd);
        pthread_exit(NULL);
    }

    
    get_client_request(message, &request);
    printf("lettura avenuta correttamente\n");
    printf("gli argomenti sono: %c, %s, %s\n", request.op_tag, request.f_path, request.o_path);

    switch(request.op_tag){
        case 'w':{
            //manage write //nella scrittura devo controllare o_path se esiste 
            break;
        }case 'r':{
            //manage read //nella lettura devo controllare f_path se esiste
            break;
        }case 'l':{
            //manage list //nel list devo controllare se esiste f_path
            break;
        }
    }


    printf("connessione terminata\n");
    close(*client_fd);
    pthread_exit(NULL);
}

void valida_root(char *root_dir){  //verifica se esiste root directory, altrimenti la crea. assumo che il path può essere o assoluto oppure relativo alla cartella contenente il server
    struct stat root_stat;

    if (stat(root_dir, &root_stat) == 0){  //verifico se il path esiste
        if (S_ISDIR(root_stat.st_mode)){ //verifico che il path sia una directory e non un file
            printf("cartella sorgente localizzata correttamente\n");
        }else{
            perror("il path inserito è quello di un file");
            exit(EXIT_FAILURE);
        }
    }else{
        if(mkdir(root_dir, 0777) == 0){
            printf("cartella sorgente creata correttamente\n");
        }else{
            perror("errore nella creazione della root directory");
            exit(EXIT_FAILURE);
        }
    }
    printf("root directory inizializzata correttamente\n");
}

void parse_input(int argc, char **argv){
    int a_flag = 0, p_flag = 0, ft_flag = 0, opt;

    while((opt = getopt(argc, argv, "a:p:d:")) != -1){
        switch(opt){
            case 'a':{
                a_flag++;
                FT_ARGS.address = optarg;
                break;
            }case 'p':{
                p_flag++;
                FT_ARGS.port = optarg;
                break;
            }case 'd':{
                ft_flag++;
                FT_ARGS.root_directory = optarg;
                break;
            }case '?':{
                perror("Invalid operation or missing argument");
                exit(EXIT_FAILURE);
            }
        }
    }

    if (a_flag != 1 || p_flag != 1 || ft_flag != 1){
        perror("missing operation");
        exit(EXIT_FAILURE);
    }

    valida_root(FT_ARGS.root_directory);
}

int main(int argc, char *argv[]){
    parse_input(argc, argv);
    //fare funzione per gestione input
    int port_num = atoi(FT_ARGS.port);
    char *ip_addr = FT_ARGS.address;
    int server_flag, client_fd;
    struct sockaddr_in server_address, client_address;
    socklen_t client_len = sizeof(client_address);

    //creazione del server
    if ((server_flag = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        perror("errore nella creazione del server");
        exit(EXIT_FAILURE);
    }else{
        printf("server creato\n");
    }

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr(ip_addr);
    server_address.sin_port = htons(port_num);

    //binding
    if ((bind(server_flag, (struct sockaddr *)&server_address, sizeof(server_address)) == -1)){
        perror("binding del server fallito");
        close(server_flag);
        exit(EXIT_FAILURE);
    }else{
        printf("binding completato\n");
    }

    //server inizia l'ascolto
    if (listen(server_flag, MAX_CONCURRENT_CONNECTIONS) == -1){ //il valore del numero massimo di connessioni è impostato a 16 poichè è il numero di core logici della macchina
        perror("errore nell'ascolto");
        close(server_flag);
        exit(EXIT_FAILURE);
    }else{
        printf("server in ascolto\n");
    }

    while(1){
        //accetto la connessione del server
        client_fd = accept(server_flag, (struct sockaddr *)&client_address, &client_len);
        if (client_fd == -1){
            perror("errore accettazione del client");
        }

        int *temp_client = &client_fd;
        pthread_create(&threads[MAX_CONCURRENT_CONNECTIONS], NULL, accettazione_client,(void *) temp_client);
        MAX_CONCURRENT_CONNECTIONS++;

        if (MAX_CONCURRENT_CONNECTIONS >= 3){
            for (int i = 0; i < MAX_CONCURRENT_CONNECTIONS; i++){
                pthread_join(threads[MAX_CONCURRENT_CONNECTIONS], NULL);
            }
            MAX_CONCURRENT_CONNECTIONS = 0;
        }
    }

    close(server_flag);
    return 0;
}