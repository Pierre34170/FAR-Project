#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <ctype.h>
#include <dirent.h> 

#define LONGEUR_MAX_PSEUDO 20
#define TAILLE_BUFFER 1024
#define TAILLE_RCV_BUFFER 10000
#define SIZE_SEND_BUFFER 10000
#define SIZE_BUFFER 1024

// déclaration des variables
    int clientSocket;
    char buffer[1024];
    struct sockaddr_in serverAddr;
    socklen_t lgA;

    struct peer_info {
        char port[10];
        char ip[20];
    
    };

    struct file_transfert_info {
        int clientSocket;
        char file[TAILLE_BUFFER];
        int file_size;
    };

// Info sur le client après qu'il se soit connecté au serveur
struct sockaddr_in local_address,cli;
int addr_size = sizeof(local_address);
socklen_t lgA = sizeof(struct sockaddr_in);



// pour la transmission de fichier, nous avons besoin de savoir
// quand le client arette de transmettre le fichier
// on décide que c'est tant qu'il n'a pas envoyé un autre fichier
// on a donc besoin d'une variable qui va terminer le thread

int transmission_fichier= 1;

// fonction qui permet de délimiter le mot saisie par le client (gràce a fgets()) 
// en remplacant le "\n" par un "\O" à la fin du mot dans le buffer
void convert(char* buff, int len) {
  int i;
  for (i = 0; i < len; i++) {
    if (buff[i] == '\n') {
      buff[i] = '\0';
      break;
    }
  }
}

int sizefile(char* name){
    FILE *file;
    int size=0;

    file=fopen(name,"rb");

    if (file){
        fseek(file, 0, SEEK_END);
        size=ftell(file);
        fclose(file);
    }
    return size;
}


void strToSocket(char * buffer,struct peer_info * peer_socket){
    // ip#port#
    sscanf(buffer,"%100[^#]%*c %30[^#]%*c"
    ,peer_socket->ip
    ,peer_socket->port);
}

// fonction qui permet d'afficher les fichiers présents dans un dossier dont le chemin est passé en paramètre 
void show_dir_content(char * path){
    // on ouvre le dossier
    DIR * d = opendir(path);

    // si il y a une erreur dans l'ouverture du dossier
    if(d==NULL) {
        printf("Le dossier n'existe pas !\n");
    }

    struct dirent * dir;

    // on lit le contenu du dossier et on l'affiche
    while ((dir = readdir(d)) != NULL) 
    {
        if ((strcmp(dir->d_name,".")!=0) && (strcmp(dir->d_name,"..")!=0)){
            printf("%s\n", dir->d_name);
        }
    }
    // on ferme le dossier
    closedir(d);
}

void *envoi_fichier_unique(struct file_transfert_info * peer_file_info){
    FILE *f;
    char filename[TAILLE_BUFFER];
    strcpy(filename,(char*)(peer_file_info->file));
    int file_size=peer_file_info->file_size;
    int read_size;
    int packet_index=0;
    char send_buffer[TAILLE_RCV_BUFFER];
    int envoi;
    int peerSocket = peer_file_info->clientSocket;
    char buffer[SIZE_BUFFER];
    strcpy(buffer,filename);
    packet_index =0;
    char path[200];

    sleep(1);

    // on envoi le nom du fichier
    if(send(peerSocket,buffer,strlen(buffer)+1,0)==-1){
        perror("Erreur d'envoi, arrêt du serveur\n");
        exit(1);
    }
    sleep(1);

    sprintf(buffer, "%d", file_size);

    // On envoi la taille du fichier
    if(send(peerSocket,buffer,strlen(buffer)+1,0)==-1){
        perror("Erreur d'envoi, arrêt du serveur\n");
        exit(1);
    }
    sleep(1);

    
    printf("*** Envoi du fichier...\n");
    // On construit le chemin du fichier
    snprintf(path, sizeof(path), "./filetosend/%s", filename);
    f = fopen(path, "rb");

    while(!feof(f)){

        // on lit le fichier et on met dans notre buffer d'envoi
        read_size = fread(send_buffer, 1, sizeof(send_buffer), f);

        //on envoi les données (si il y a eu une erreur avec write on renvois)
        do{
            envoi = send(peerSocket, send_buffer, read_size, 0);  
        }while (envoi < 0);

        packet_index ++;

        // on remet à 0 tous les octets du buffer d'envoi
        bzero(send_buffer, sizeof(send_buffer));
        sleep(1);

    }
        printf("*** Envoi au client réussit\n");
        close(peerSocket);
    }


// thread pour envoyer un fichier aux clients qui se connectent
void *envoi_fichier(char *file){

    FILE *f;
    char filename[TAILLE_BUFFER];
    strcpy(filename,(char*)file);
    int file_size=0;
    int peerSocket;
    char path[200];

    int socketServeur = socket(PF_INET, SOCK_STREAM, 0);
    local_address.sin_port=htons((int) ntohs(local_address.sin_port)+1000);    

    if(bind(socketServeur, (struct sockaddr *) &local_address, sizeof(local_address))==-1){
        perror("Erreur du bind, arrêt du peer\n");
        exit(1);
        
    }
    printf("*** Passage en mode serveur\n");

    // Serveur en mode écoute
    if (listen(socketServeur,5)==-1){
        printf("Erreur du listen, arrêt du peer\n");
        exit(1);
    }
    printf("*** Écoute des receveurs...\n");

    // Saisie du fichier
    snprintf(path, sizeof(path), "./filetosend/%s", filename);
    file_size = sizefile(path);

    f = fopen(path, "rb");

    if(f == NULL){
        perror("Error IN Opening File .. \n");
        exit(1);
    }

    sleep(1);
    
    while(1){
        peerSocket=accept(socketServeur,(struct sockaddr *) &cli,&lgA);
        if(peerSocket==-1)
        {
            printf("Erreur de l'accept, arrêt du peer");
            exit(1);
        } else {
             struct file_transfert_info peer_file_info;
        strcpy(peer_file_info.file,(char*)file);
        peer_file_info.file_size=file_size;
        peer_file_info.clientSocket=peerSocket;
        pthread_t sendUniqueFile;
        pthread_create(&sendUniqueFile, NULL, envoi_fichier_unique, &peer_file_info);
        sleep(1);

        }
           }

        
}

void *reception_fichier(struct peer_info *peer_socket){
    int peerSocket = socket(PF_INET, SOCK_STREAM, 0);
    struct sockaddr_in peerAddr;
    //paramètrage socket
    peerAddr.sin_family = AF_INET;
    peerAddr.sin_port = htons((int) atoi(peer_socket->port));
    peerAddr.sin_addr.s_addr = inet_addr(peer_socket->ip);
    lgA = sizeof peerAddr;

    // sert à suspendre le thread d'envoi de fichier entre 2 envois
    struct timespec tim1, tim2;
    tim1.tv_sec = 0;
    tim1.tv_nsec = 250000000L;

    nanosleep(&tim1, &tim2);

    FILE *fp;
    char filename[SIZE_BUFFER];
    int size_file,read_size,write_size;
    int recv_size=0;
    int packet_index=0;
    char rcv_buffer[SIZE_SEND_BUFFER];
    char buffer[SIZE_BUFFER];
    char path[200]; // variable dans laquel on stocke le chemin du fichier

    

    //connection serveur
    while(connect(peerSocket, (struct sockaddr *) &peerAddr, lgA)==-1){
        perror("Erreur connexion au peer");
        peerAddr.sin_family = AF_INET;
        peerAddr.sin_port = htons((int) atoi(peer_socket->port));
        peerAddr.sin_addr.s_addr = inet_addr(peer_socket->ip);
        nanosleep(&tim1, &tim2);
    }   
    printf("*** Connexion établie au peer...\n");

    // Tant qu'on a pa reçu le nom du fichier on attend 
    while(recv(peerSocket, buffer, sizeof(buffer), 0)==-1){

    }
    printf("*** Nom du fichier reçu : ");
    puts(buffer);
    strcpy(filename,buffer);
    memset(&buffer[0], 0, sizeof(buffer));

    // Tant que l'on a pas reçu la taille du fichier on attend
    while(recv(peerSocket, buffer, sizeof(buffer), 0)==-1){

    }
    size_file=atoi(buffer);
    printf("*** Taille du fichier : %d octets\n", size_file);

    // on construit le chemin du fichier dans lequel on va écrire
    snprintf(path, sizeof(path), "./filereceived/%s", filename);

    // on ouvre en écriture le fichier
    fp = fopen(path, "wb");

    if(fp == NULL){
        perror("*** Le fichier ne peut pas être ouvert\n");
        exit(1);
    }

    printf("*** Réception du fichier...\n");

    while(recv_size < size_file){
        do {
            read_size =recv(peerSocket, rcv_buffer, sizeof(rcv_buffer),0);
        }while (read_size<0);

        // on ecrit les données lu dans le fichier
        write_size = fwrite(rcv_buffer, 1, read_size, fp);
        if (read_size != write_size){
            printf("erreur les donnéees reçues n'ont pas été enregistrés correctement\n");
        }
        recv_size = recv_size + read_size;
        packet_index ++;
    }

    fclose(fp);
    printf("*** Fichier reçu avec succès\n");

    pthread_exit(NULL);
}

// le thread envoi permet de saisir un message et de l'envoyer au serveur
void *envoi(void *arg){
    FILE *f;
    char path[200];
    int EXIT = 0;
    char filename[SIZE_BUFFER];

    while(EXIT == 0){
        //on vide le buffer
        memset(&buffer[0], 0, sizeof(buffer));

        //saisie du message
        fgets(buffer, sizeof(buffer), stdin);

        char *chfin = strchr(buffer, '\n');
        *chfin = '\0';

        // on regarde si le message envoyé est "fin"
        if(strcmp(buffer,"fin")==0){
            EXIT = 1;
        } 
        else if (strcmp(buffer,"file")==0)

        {
        // On entre un fichier tant que le nom n'est pas valide
        do {
            printf("------------- Fichiers à envoyer -------------\n");
            show_dir_content("./filetosend");
            printf("\n");

            printf("*** Saisissez le fichier à envoyer : \n");
            fgets(filename, sizeof(filename), stdin);
            convert(filename, strlen(filename));

            // On construit le chemin du fichier
            snprintf(path, sizeof(path), "./filetosend/%s", filename);
            f = fopen(path, "rb");
            if(f == NULL){
                printf("*** ERREUR : Ce fichier n'existe pas veuillez entrer un nom correct !\n");
            }
        }   while(f == NULL);

        printf("*** Début d'envoi de fichiers\n");
        pthread_t sendfile;
        pthread_create(&sendfile, NULL, envoi_fichier, &filename);

        }

        // on envoie le message
        if(send(clientSocket,buffer,strlen(buffer)+1,0)==-1){
            perror("Erreur du send\n");
            exit(1);
        }

        
        
    }
    pthread_exit(NULL);
}

// le thread reception permet de recevoir un message du serveur et de l'afficher
void *reception(void *arg){
    int EXIT = 0;
    struct peer_info peer_socket;
    while(EXIT == 0){
        // on vide le buffer
        memset(&buffer[0], 0, sizeof(buffer));

        // on attend de recevoir un message
        int rcv = recv(clientSocket, buffer, sizeof(buffer)-1, 0);
        if(rcv == -1){
            perror("Erreur rcv\n");
            exit(1);
        }
        else{ 
            // si rcv n'est pas égal a 1 alors un message a été reçu
            if (rcv != 1) {
                // On a reçu les infos d'un client pour recevoir un fichier
                if (strncmp("1",buffer,1)==0){
                    // On deserialise les informations du buffer pour les mettre dans le struct de la socket
                    strToSocket(buffer,&peer_socket);
                    printf("*** Début de réception du fichier\n");
                    pthread_t recvfile;
                    pthread_create(&recvfile, NULL, reception_fichier, &peer_socket);


                    // Création du thread de connexion au client emetteur
                } else
                // Si le message reçu n'est pas fin
                if(strcmp(buffer,"fin")!=0){
                    // on affiche le message
                    puts(buffer);

                    // on vide le buffer
                    memset(&buffer[0], 0, sizeof(buffer));
                }
                // On a reçu un fichier 
                else
                {
                    printf("Fin de la communication\n");
                    EXIT = 1;
                }
            }
        }
    }
    pthread_exit(NULL);
}




int main() {

    //paramètrage socket
    clientSocket = socket(PF_INET, SOCK_STREAM, 0);
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8081);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    lgA = sizeof serverAddr;

    //connection serveur
    if(connect(clientSocket, (struct sockaddr *) &serverAddr, lgA)==-1){
        printf("Erreur connexion serveur\n");
        exit(1);
    }   
    printf("Connexion établie ...\n");

    // On charge les données de la socket après la connexion serveur dans
    // les variables pré-définies
    getsockname(clientSocket, &local_address, &addr_size);

    // on reçoit le message de début de conversation du serveur
    int rcv = recv(clientSocket, buffer, sizeof(buffer)-1, 0);
    if(rcv == -1){
        perror("Erreur rcv\n");
        exit(1);
    }
    printf("%s\n",buffer);

    // on initialise les deux threads
    pthread_t sendmsg;
    pthread_t receivemsg;

    // on lance les deux threads
    pthread_create(&sendmsg, NULL, envoi, NULL);
    pthread_create(&receivemsg, NULL, reception, NULL);

    // on attend que le thread concernant la reception du message se termine (dans le cas ou le client reçoit "fin")
    pthread_join(receivemsg, NULL);

    close(clientSocket);
    return 0;
}


