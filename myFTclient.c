#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>


int main(int argc, char *argv[]){

    char *server_ip = argv[1];
    int server_port = atoi(argv[2]);
    int client_flag; 
    struct sockaddr_in server_address;

    //creazione del client
    if ((client_flag = socket(AF_INET, SOCK_STREAM, 0)) == -1){
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
    if (connect(client_flag, (struct sockaddr * )&server_address, sizeof(server_address))== -1){
        perror("connessione fallita");
        close(client_flag);
        exit(-1);
    }else{
        printf("connessione al server riuscita\n");
    }
    close(client_flag);
    return 0;
}