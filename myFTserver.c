#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>


int main(int argc, char *argv[]){
    //fare funzione per gestione input
    int port_num = atoi(argv[2]);
    char *ip_addr = argv[1];
    int server_flag, client_flag;
    struct sockaddr_in server_address, client_address;
    socklen_t client_len = sizeof(client_address);

    //creazione del server
    if ((server_flag = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        perror("errore nella creazione del server");
        exit(1);
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
        exit(1);
    }else{
        printf("binding completato\n");
    }

    //server inizia l'ascolto
    if (listen(server_flag, 16) == -1){ //il valore del numero massimo di connessioni è impostato a 16 poichè è il numero di core logici della macchina
        perror("errore nell'ascolto");
        close(server_flag);
        exit(1);
    }else{
        printf("server in ascolto\n");
    }

    while(1){
        //accetto la connessione del server
        client_flag = accept(server_flag, (struct sockaddr *)&client_address, &client_len);
        if (client_flag == -1){
            perror("errore accettazione del client");
            exit(1);
        }
        printf("client accettato\n");
        break;
    }

    close(server_flag);
    return 0;
}