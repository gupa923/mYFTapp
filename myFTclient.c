#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <getopt.h>
#include "myClient.h"
#include "myFTstruct.h"

client_args THIS_ARGS;


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
            THIS_ARGS.o_path = NULL;
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
    
    close(client_fd);
    return 0;
}