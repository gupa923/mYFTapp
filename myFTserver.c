#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "myServer.h"

server_args FT_ARGS;

//LA CONCORRENZA IN TEORIA FUNZIONA MA SAREBBE MEGLIO MODIFICARLA LO FARO? ALLA FINE
int MAX_CONCURRENT_CONNECTIONS= 3;
pthread_t threads[3];

void *accettazione_client(void *args){
    printf("client accettato\n");
    int *client = (int *)args;
    sleep(5);
    printf("connessione terminata");
    close(*client);
    pthread_exit(NULL);
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

    //validation root directory
}

int main(int argc, char *argv[]){
    parse_input(argc, argv);
    //fare funzione per gestione input
    int port_num = atoi(FT_ARGS.port);
    char *ip_addr = FT_ARGS.address;
    int server_flag, client_flag;
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
        client_flag = accept(server_flag, (struct sockaddr *)&client_address, &client_len);
        if (client_flag == -1){
            perror("errore accettazione del client");
            exit(EXIT_FAILURE);
        }

        int *temp_client = &client_flag;
        pthread_create(&threads[MAX_CONCURRENT_CONNECTIONS], NULL, accettazione_client, temp_client);
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