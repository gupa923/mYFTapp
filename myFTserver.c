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
#include <dirent.h>
#include "myServer.h"

void get_write_header(write_header *header, char *h_string){
    char *local_copy = malloc(sizeof(char)*strlen(h_string));
    strcpy(local_copy, h_string);
    char *save_ptr;
    char *token = strtok_r(local_copy, ":", &save_ptr);
    int i = 0;
    while(token != NULL){
        if (i == 0){
            header->flag = atoi(token);
            token = strtok_r(NULL, ":", &save_ptr);
            i++;
        }else if (i == 1){
            header->content_size = strtol(token, NULL, 10);
            token = strtok_r(NULL, ":", &save_ptr);
            i++;
        }
    }
}

int verify_wpath(char *o_path){
    //prima vedo se il path fornito è quello di un file txt
    char *ext = strrchr(o_path, '.');
    if (ext == NULL || strcmp(ext, ".txt") != 0){
        return 0; //il path fornito non è quello di un file
    }

    //visto che il path è quello di un file .txt vedo se le cartelle precedenti esistono e in caso contrario le creo
    char *temp = (char *)malloc((strlen(o_path) + 1) * sizeof(char));
    if (temp == NULL){
        perror("impossibile allocare la memoria");
        return 0;
    }
    strcpy(temp, o_path);
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

void do_write(int *client_fd, client_request *request){
    char h_string[256];
    write_header header;
    if (read(*client_fd, &h_string, sizeof(h_string)) < 0){
        perror("impossibile ricevere i dati dal client");
        return;
    }
    get_write_header(&header, h_string);
    if (header.flag == 0){
        perror("impossibile procedere con la scrittura");
        return;
    }
    char *temp = (char *)malloc(strlen(request->o_path) * sizeof(char));
    strcpy(temp, request->o_path);
    sprintf(request->o_path, "%s%s", FT_ARGS.root_directory, temp);
    int flag = verify_wpath(request->o_path);
    free(temp);
    //manca il messaggio al client per dirgli di procedere all'invio dei dati
    char sflag[2];
    sprintf(sflag, "%d", flag);
    if (write(*client_fd, sflag, sizeof(sflag)) < 0){
        perror("impossibile inviare dati");
        return;
    }

    if (flag == 0){
        perror("percorso non valido");
        return;
    }

    int bytes_recived;
    char buffer[1024] = "";
    char *content = (char *)malloc(header.content_size);
    if (content == NULL){
        perror("impossibile allocare la memoria");
        return;
    }
    memset(content, 0, header.content_size * sizeof(char));

    FILE *fp = fopen(request->o_path, "w");
    if (fp == NULL){
        perror("impossibile aprire il file");
        return;
    }
    while(bytes_recived = read(*client_fd, buffer, sizeof(buffer)) > 0){
        strcat(content, buffer);
    }

    char response[] = "1";
    if (fputs(content, fp) == EOF){
        perror("errore di scrittura");
        response[0] = '0';
        return;
    }
    free(content);
    fclose(fp);

    if (write(*client_fd, response, sizeof(response)) < 0){
        perror("impossibile inviare una risposta al client");
        return;
    }

    if (response[0] == '0'){
        perror("impossibile completare la scrittura");
        return;
    }
    //codice per indicare che la richiesta è andata a buon 
    printf("richiesta completata con successo\n");
}

int file_exists(char *path){
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

void do_read(int *client_fd, client_request *request){
    //primo compito vedere se il file esiste
    char *temp = (char *)malloc(strlen(request->f_path) * sizeof(char));
    strcpy(temp, request->f_path);
    sprintf(request->f_path, "%s%s", FT_ARGS.root_directory, temp);
    int flag = file_exists(request->f_path);
    free(temp);

    //ricavo la dimensione del file
    long size = 0;
    char header[256];
    FILE *fp = fopen(request->f_path, "r");
    if (fp == NULL){
        flag = 0;
        size = 0;
    }else{
        fseek(fp, 0, SEEK_END);
        size = ftell(fp);
        rewind(fp);
    }

    sprintf(header, "%d:%ld", flag, size);
    if (write(*client_fd, header, sizeof(header)) < 0){
        perror("errore invio dati al client");
        return;
    }
    if (flag == 0){
        perror("file non esistenete");
        return;
    }

    char send_signal[2];
    if (read(*client_fd, send_signal, sizeof(send_signal)) < 0){
        perror("erorre nella read");
        return;
    }
    if (send_signal[0] == 0){
        perror("path di destinazione non valido");
        return;
    }
    printf("path di destinazione valido\n");

    char buffer[1024];
    int bytes_read;
    while((bytes_read = fread(buffer, 1, sizeof(buffer), fp)) > 0){
        if (write(*client_fd, buffer, bytes_read) == -1){
            perror("write non andata a buon fine");
        }
    }
    fclose(fp);

    shutdown(*client_fd, SHUT_WR);

    char response[2];
    if (read(*client_fd, response, sizeof(response)) < 0){
        perror("impossibile ricevere risposta dal client");
        return;
    }

    if (response[0] == '0'){
        perror("impossibile completare la lettura");
        return;
    }
    printf("lettura completa con successo\n");
}

void do_list(int *client_fd, client_request *request){
    /**
     * @todo verificare la correttezza di f_path
     * @todo inviare un messaggio al client per indicare che la richiesta può essere eseguita
     * @todo inviare il contenuto della directory un'entry alla volta
     * @todo aspettare che il client mandi un messaggio di conferma della ricezione
     */

    //controllo se la directory è valida
    char *temp = (char *)malloc(strlen(request->f_path) * sizeof(char));
    strcpy(temp, request->f_path);
    sprintf(request->f_path, "%s%s", FT_ARGS.root_directory, temp);
    free(temp);

    int flag = 1;
    struct dirent *element;
    DIR *f_dir = opendir(request->f_path);

    if (f_dir == NULL){
        printf("directory non esiste\n");
        flag = 0;
    }

    char send_flag[2];
    sprintf(send_flag, "%d", flag);
    if (write(*client_fd, send_flag, sizeof(send_flag)) < 0){
        perror("errore nella write");
        return;
    }

    if (flag == 0){
        printf("percorso inserito non valido o non esistente\n");
        return;
    }

    //prendo e invio i dati al client
    while ((element = readdir(f_dir)) != NULL){
        struct stat info;
        char temp_path[1024];
        char send_buffer[1024];

        //ignoro le directory . e .. e i file nascosti
        if (strncmp(element->d_name, ".", 1) == 0 || strncmp(element->d_name, "..", 2) == 0){
            continue;
        }

        sprintf(send_buffer, "%s", element->d_name);
        //controllo se l'elemento analizzato è una directory
        sprintf(temp_path, "%s/%s", request->f_path, element->d_name);
        if (stat(temp_path, &info) == 0 && S_ISDIR(info.st_mode)){
            sprintf(send_buffer, "<DIRECTORY> %s", element->d_name);
        }

        if (write(*client_fd, send_buffer, sizeof(send_buffer))<0){
            perror("write non andata a buon fine");
        }
    }

    closedir(f_dir);

    shutdown(*client_fd, SHUT_WR);

    printf("richiesta di listing completata con successo\n");
    return;
}

void get_client_request(char *content, client_request *request){
    char *local_copy = malloc(sizeof(char)*strlen(content));
    char *save_ptr;
    strcpy(local_copy, content);
    char *token = strtok_r(local_copy, ":", &save_ptr);
    int i = 0;
    while(token != NULL){
        if (i == 0){
            request->op_tag = token[0];
            token = strtok_r(NULL, ":", &save_ptr);
            i++;
        }else if (i == 1){
            strcpy(request->f_path, token);
            token = strtok_r(NULL, ":", &save_ptr);
            i++;
        }else{
            strcpy(request->o_path, token);
            token = strtok_r(NULL, ":", &save_ptr);
            i++;
        }
    }
    free(local_copy);
    //NON HO BISOGNO DI CONTROLLARE SE LA LETTURA E' aANDATA A BUON FINE POICHÈ HO LA GARANZIA CHE IL CLIENT NON SI CONNETTA SE LA RICHIESTA E' SBAGLIATA
}

void *accettazione_client(void *args){
    printf("client accettato\n");
    char message[256];
    int *client_fd = (int *)args;
    client_request request;

    if(read(*client_fd, &message, sizeof(message)) < 0){
        perror("impossibi leleggere la richiesta");
        close(*client_fd);
        pthread_exit(NULL);
    }

    
    get_client_request(message, &request);
    printf("lettura della richiesta avvenuta correttamente\n");

    switch(request.op_tag){
        case 'w':{
            do_write(client_fd, &request);
            break;
        }case 'r':{
            do_read(client_fd, &request);
            //manage read //nella lettura devo controllare f_path se esiste
            break;
        }case 'l':{
            do_list(client_fd, &request);
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
            continue;
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