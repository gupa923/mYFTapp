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

MUTEX_LIST_RECORD *first_list_record = NULL; //puntatore al primo elemento della lista linkata di mutex

pthread_mutex_t list_mutex = PTHREAD_MUTEX_INITIALIZER; //mutex associato alla lista di mutex

pthread_mutex_t *get_file_lock(char *file_name){
    pthread_mutex_lock(&list_mutex);

    //scorro la lista per trovare il mutex associato al file, se non è presente lo creo
    MUTEX_LIST_RECORD *current_node = first_list_record;
    MUTEX_LIST_RECORD *prev = NULL; //puntatore al nodo precedente, se rimane null la lista è vuota
    while(current_node){
        if (strcmp(current_node->file_path, file_name) == 0){
            pthread_mutex_unlock(&list_mutex);
            return &current_node->file_mutex;
        }
        prev = current_node;
        current_node = current_node->next;
    }

    //se il mutex non esiste lo creo e lo aggiungo alla lista
    MUTEX_LIST_RECORD *new_node = (MUTEX_LIST_RECORD *)malloc(sizeof(MUTEX_LIST_RECORD));
    /*
    new_node->file_path = (char *)malloc((strlen(file_name) + 1)*sizeof(char));
    strcpy(new_node->file_path, file_name);
    */
    new_node->file_path = strdup(file_name);
    pthread_mutex_init(&new_node->file_mutex, NULL);
    new_node->next = NULL;

    //controllo se il nodo creato era il primo
    if (prev == NULL){
        first_list_record = new_node;
    }else{
        prev->next = new_node;
    }

    pthread_mutex_unlock(&list_mutex);
    return &new_node->file_mutex;
}

void free_mutex_list(){
    pthread_mutex_lock(&list_mutex);

    MUTEX_LIST_RECORD *current_node = first_list_record;
    while(current_node){
        MUTEX_LIST_RECORD *temp = current_node;
        current_node = temp->next;
        pthread_mutex_destroy(&temp->file_mutex);
        free(temp->file_path);
        free(temp);
    }

    first_list_record = NULL;
    pthread_mutex_unlock(&list_mutex);
}

void get_write_header(write_header *header, char *h_string){
    char *local_copy = malloc(sizeof(char)*(strlen(h_string)+1));
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
    //verifico se il path esiste
    char *temp = (char *)malloc((strlen(request->o_path) + 1) * sizeof(char));
    strcpy(temp, request->o_path);
    sprintf(request->o_path, "%s%s", FT_ARGS.root_directory, temp);
    int flag = verify_wpath(request->o_path);
    free(temp);
    //invio messaggio per indicare se procedere o meno con la richiesta
    char sflag[2];
    sprintf(sflag, "%d", flag);
    if (write(*client_fd, sflag, sizeof(sflag)) < 0){
        perror("impossibile inviare dati");
        return;
    }
    //se flag == 0 allora il percorso di output non è vaildo e chiudo la connessione
    if (flag == 0){
        perror("percorso non valido");
        return;
    }

    int bytes_recived;
    char buffer[1024] = ""; //buffer dove salvare temporaneamente le linee del file letto
    char *content = (char *)malloc(header.content_size); //conterrà l'intero contenuto del file da scrivere
    if (content == NULL){
        perror("impossibile allocare la memoria");
        return;
    }
    memset(content, 0, header.content_size * sizeof(char)); //imposta tutti i byte di content a 0

    pthread_mutex_t *file_lock = get_file_lock(request->o_path);

    pthread_mutex_lock(file_lock);
    FILE *fp = fopen(request->o_path, "w+"); //apro il file in lettura, se il file non esiste lo creo
    if (fp == NULL){
        perror("impossibile aprire il file");
        return;
    }
    while(bytes_recived = read(*client_fd, buffer, sizeof(buffer)) > 0){ //leggo riga per riga le stringhe inviate dal client
        strcat(content, buffer);
    }
    //scrivo il contenuto nel file d'output
    char response[] = "1";
    if (fputs(content, fp) == EOF){
        perror("errore di scrittura");
        response[0] = '0';
        return;
    }
    free(content);
    fclose(fp);
    pthread_mutex_unlock(file_lock);
    //invio un messaggio al client che conferma che la richiesta è stata completata
    if (write(*client_fd, response, sizeof(response)) < 0){
        perror("impossibile inviare una risposta al client");
        return;
    }

    if (response[0] == '0'){ //se non è stato possibile scrivere sul file termino la connessione
        perror("impossibile completare la scrittura");
        return;
    }
    //termino la connessione
    printf("richiesta completata con successo\n");
}

int file_exists(char *path){
    struct stat file_stat;
    //controllo se il path esiste
    if (stat(path, &file_stat) != 0){
        perror("non è un file");
        return 0; //non è un path valido
    }

    if (!S_ISREG(file_stat.st_mode)){
        perror("non è un file regolare");
        return 0; //non è un file regolare
    }

    //controllo sia il path ad un file .txt
    char *ext = strrchr(path, '.'); //restituisce il puntatore all'ultima occorrenza di ".", così facendo ricavo l'estensione del file
    if(ext != NULL && strcmp(ext, ".txt") == 0){
        return 1; //è un file txt
    }
    perror("non è un file txt");
    return 0;
}

void do_read(int *client_fd, client_request *request){
    //vedo se il file richiesto esiste
    char *temp = (char *)malloc((strlen(request->f_path) + 1) * sizeof(char));
    strcpy(temp, request->f_path);
    sprintf(request->f_path, "%s%s", FT_ARGS.root_directory, temp);
    int flag = file_exists(request->f_path);
    free(temp);

    pthread_mutex_t *file_lock = get_file_lock(request->f_path);
    pthread_mutex_lock(file_lock);
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
    //invio al client il risultato dei precedenti controlli per indicare se procedere con la richiesta o interrompere la connessione
    sprintf(header, "%d:%ld", flag, size);
    if (write(*client_fd, header, sizeof(header)) < 0){
        perror("errore invio dati al client");
        return;
    } 
    //se flag == 0 allora il percorso non esiste oppure non è un path ad un file .txt quindi devo terminare la connessione
    if (flag == 0){
        perror("file non esistenete");
        return;
    }

    //attendo che il client invii il segnale per iniziare ad inviare i dati
    char send_signal[2];
    if (read(*client_fd, send_signal, sizeof(send_signal)) < 0){
        perror("erorre nella read");
        return;
    }
    //se il segnale inviato è 0 allora il client non può ricevere il file e devo quindi interrompere la connessione
    if (send_signal[0] == 0){  
        perror("path di destinazione non valido");
        return;
    }
    printf("path di destinazione valido\n");

    //invio i dati al client
    char buffer[1024];  //buffer temporaneo
    int bytes_read; //numero di byte letti dal file
    while((bytes_read = fread(buffer, 1, sizeof(buffer), fp)) > 0){ //leggo il file riga per riga
        if (write(*client_fd, buffer, bytes_read) == -1){ //invio al client la riga appena letta
            perror("write non andata a buon fine");
        }
    }
    fclose(fp);
    pthread_mutex_unlock(file_lock);
    //invio segnale per interrompere la lettura del client
    shutdown(*client_fd, SHUT_WR);

    //attendo che il client mandi una conferma dell'avvenuta ricezione
    char response[2];
    if (read(*client_fd, response, sizeof(response)) < 0){
        perror("impossibile ricevere risposta dal client");
        return;
    }
    //chiudo la connessione
    if (response[0] == '0'){
        perror("impossibile completare la lettura");
        return;
    }
    printf("lettura completa con successo\n");
}

void do_list(int *client_fd, client_request *request){
    //controllo se la directory è valida
    char *temp = (char *)malloc((strlen(request->f_path) + 1) * sizeof(char));
    strcpy(temp, request->f_path);
    sprintf(request->f_path, "%s%s", FT_ARGS.root_directory, temp);
    free(temp);

    //apro la directory
    int flag = 1;
    struct dirent *element;
    DIR *f_dir = opendir(request->f_path);

    //se l'apertura fallisce imposto il flag a 0 
    if (f_dir == NULL){
        printf("directory non esiste\n");
        flag = 0;
    }

    //invio il flag di controllo per dire al clien se si può procedere o meno con l'invio dei dati
    char send_flag[2];
    sprintf(send_flag, "%d", flag);
    if (write(*client_fd, send_flag, sizeof(send_flag)) < 0){
        perror("errore nella write");
        return;
    }

    if (flag == 0){ //se il flag è 0 allora la directory non esiste quindi bisogna terminare la connessione
        printf("percorso inserito non valido o non esistente\n");
        return;
    }

    //prendo e invio i dati al client
    while ((element = readdir(f_dir)) != NULL){
        struct stat info; //serve per controllare se l'elemento è un file o una directory
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

        if (write(*client_fd, send_buffer, sizeof(send_buffer))<0){  //invio la stringa con i dati al client
            perror("write non andata a buon fine");
        }
    }

    //chiudu la directory e mando un segnale per terminare la letturadel client. poi termino la connessione
    closedir(f_dir);
    shutdown(*client_fd, SHUT_WR);
    printf("richiesta di listing completata con successo\n");
    return;
}

void get_client_request(char *content, client_request *request){
    char *local_copy = malloc(sizeof(char)*(strlen(content) +1));
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
    //A questo punto ho la garanzia da parte del client che non manchino argomenti
}

void *accettazione_client(void *args){
    printf("client accettato\n");
    char message[256];
    int *client_fd = (int *)args;
    client_request request;

    //ricevo il messaggio contenente la richiesta del client
    if(read(*client_fd, &message, sizeof(message)) < 0){
        perror("impossibi leleggere la richiesta");
        close(*client_fd);
        pthread_exit(NULL);
    }

    //analizzo la richiesta del client
    get_client_request(message, &request);
    printf("lettura della richiesta avvenuta correttamente\n");

    //scelgo la funzione corretta in base all'operazione inserita nella richiesta
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

    pthread_t threads[MAX_CONCURRENT_CONNECTIONS];
    int thread_num = 0;

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

        int *temp_client = malloc(sizeof(int));
        *temp_client = client_fd;
        pthread_create(&threads[thread_num], NULL, accettazione_client,(void *) temp_client);
        thread_num++;

        if (thread_num >= MAX_CONCURRENT_CONNECTIONS){
            for (int i = 0; i < MAX_CONCURRENT_CONNECTIONS; i++){
                pthread_join(threads[i], NULL);
            }
            thread_num = 0;
        }
    }
    free_mutex_list();
    close(server_flag);
    return 0;
}