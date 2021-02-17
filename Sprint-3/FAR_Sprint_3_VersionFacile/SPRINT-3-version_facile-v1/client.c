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


#define SIZE_SEND_BUFFER 10000
#define SIZE_BUFFER 1024


    // déclaration des variables
    int clientSocket;
    char buffer[SIZE_BUFFER];
    struct sockaddr_in serverAddr;
    socklen_t lgA;


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

// fonction qui calcul la taille d'un fichier dont le chemin est passé en paramètre
int sizefile(char* name){
    FILE *file;
    int size;

    file=fopen(name,"rb");

    if (file){
        fseek(file, 0, SEEK_END);
        size=ftell(file);
        fclose(file);
    }
    return size;
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

// thread permettant de recevoir un fichier
void *recevoir_fichier(void *arg){

    // déclaration des variables
    FILE *fp;
    char filename[SIZE_BUFFER]; // nom du fichier reçu
    int size_file; // taille du ficheir reçu
    int recv_size=0; // le nombre d'octets total que l'on a reçu
    int read_size; // le nombre d'octets du paquet que l'on reçoit 
    int packet_index=0; // indexe du paquet reçu
    int write_size; // le nombre d'octet que l'on écrit dans le fichier 
    char rcv_buffer[SIZE_SEND_BUFFER]; // buffer dans lequel on stocke les données reçues
    char path[200]; // variable dans laquel on stocke le chemin du fichier

    // sert à suspendre le thread d'envoi de fichier entre 2 envois
    struct timespec tim1, tim2;
    tim1.tv_sec = 0;
    tim1.tv_nsec = 250000000L;

    // Tant qu'on a pa reçu le nom du fichier on attend 
    while(recv(clientSocket, buffer, sizeof(buffer), 0)==-1){

    }

    strcpy(filename,buffer);

    memset(&buffer[0], 0, sizeof(buffer));

    // Tant que l'on a pas reçu la taille du fichier on attend
    while(recv(clientSocket, buffer, sizeof(buffer), 0)==-1){

    }
    size_file=atoi(buffer);

    // on construit le chemin du fichier dans lequel on va écrire
    snprintf(path, sizeof(path), "./filereceive/%s", filename);

    // on ouvre en écriture le fichier
    fp = fopen(path, "wb");

    if(fp == NULL){
        perror("Le fichier ne peut pas être ouvert\n");
        exit(1);
    }

    printf("Reception du fichier...\n");

// cela va nous être utile dans le cas ou le client ne recoit pas le fichier en entier, c'est à dire ne recoit pas le nombre d'octet auquel il s'attend
// cela va donc impliquer que le programme sera bloquer dans la boucle while ci dessous
// on va utiliser cette structure pour faire en sorte que lorsque le client ne recoit pas tous les octets attendus, il ne reste pas bloqué indéfiniment dans la boucle
// le timer va permettre de lancer le protocole de renvoi du fichier dans ce cas 
    struct timeval timeout;
    fd_set fds;
    int buffer_fd;

    // on attend pendant 1 secondes max
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

// on peut simuler le fait que le client de reçoit pas le fichier en entier 
//    recv_size=-10;

    while((recv_size < size_file)){

        FD_ZERO(&fds);
        FD_SET(clientSocket, &fds);

        buffer_fd = select(FD_SETSIZE, &fds, NULL, NULL, &timeout);


        if (buffer_fd<0){
            perror("erreur");
            exit(1);
        }

        // le cas ou le delais du time out à expiré et que l'on a toujours rien reçu
        // le client n'a pas reçu le fichier correctement
        else if (buffer_fd==0){
            printf("Erreur dans la reception du fichier\n");

            // on demande au serveur de nous renvoyer le fichier
            strcpy(buffer, "ERROR_FILE");
            if(send(clientSocket,buffer,strlen(buffer)+1,0)==-1){
                perror("Erreur du send\n");
                exit(1);
            }
            nanosleep(&tim1, &tim2);
            // on envoi le nom du fichier
            if(send(clientSocket,filename,strlen(filename)+1,0)==-1){
                printf("Erreur d'envoi, arrêt du serveur\n");
                exit(1);
            }
            fp = fopen(path, "wb");
            nanosleep(&tim1, &tim2);

            // on remet les compteurs à zéros
            packet_index = 0;
            recv_size = 0;
        }

        else if (buffer_fd>0){
            // on reçoit les paquets envoyés par un autre client
            do {
                read_size =recv(clientSocket, rcv_buffer, sizeof(rcv_buffer),0);
            }while (read_size<0);

            // on ecrit les données reçues dans le fichier
            write_size = fwrite(rcv_buffer, 1, read_size, fp);

            if (read_size != write_size){
                printf("erreur les donnéees reçues n'ont pas été enregistrés correctement\n");
            }

            recv_size = recv_size + read_size;
            packet_index ++;
        }
    }

    printf("Fichier bien reçu !\n");
    printf("\n");

    fclose(fp);

    pthread_exit(NULL);

}


void *envoi_fichier(void *arg){

    FILE *f;
    char filename[SIZE_BUFFER]; 
    int file_size;
    int read_size;
    int envoi;
    int package_index = 0;
    char send_buffer[SIZE_SEND_BUFFER];
    char path[200];

    // sert à suspendre le thread d'envoi de fichier entre 2 envois
    struct timespec tim1, tim2;
    tim1.tv_sec = 0;
    tim1.tv_nsec = 250000000L;


    do {
        printf("saisissez le fichier à envoyer : ");
        fgets(filename, sizeof(filename), stdin);

        convert(filename, strlen(filename));

        // on construit le chemin du fichier
        snprintf(path, sizeof(path), "./filetosend/%s", filename);

        f = fopen(path, "rb");

        if(f == NULL){
            printf("Ce fichier n'existe pas veuillez entrer un nom correcte !\n");
        }
    }while(f == NULL);

    // on envoie le nom du fichier
    if(send(clientSocket,filename,strlen(filename)+1,0)==-1){
        perror("Erreur du send\n");
        exit(1);
    }
    nanosleep(&tim1, &tim2);

    // on calcule la taille du fichier à transmettre
    file_size = sizefile(path);


    sprintf(buffer, "%d", file_size);

    // on envoie la taille du fichier
    if(send(clientSocket,buffer,strlen(buffer)+1,0)==-1){
        perror("Erreur du send\n");
        exit(1);
    }

    nanosleep(&tim1, &tim2);
    printf("Envoi du fichier...\n");
    while(!feof(f)){

        // on lit le fichier et on met dans notre buffer d'envoi
        read_size = fread(send_buffer, 1, sizeof(send_buffer), f);

        // on envoi les données (si il y a eu une erreur avec send on renvoit)
        do{
            envoi = send(clientSocket, send_buffer, read_size, 0);  
        }while (envoi < 0);

        package_index ++;

        // on remet à 0 tous les octets du buffer d'envoi
        bzero(send_buffer, sizeof(send_buffer));
        nanosleep(&tim1, &tim2);

    }
    printf("\n");
    printf("Envoi fini.\n");
    printf("\n");

    fclose(f);

    pthread_exit(NULL);
}



// le thread envoi permet de saisir un message et de l'envoyer au serveur
void *envoi(void *arg){
    int EXIT = 0;

    while(EXIT == 0){
        //on vide le buffer
        memset(&buffer[0], 0, sizeof(buffer));

        //saisie du message
        fgets(buffer, sizeof(buffer), stdin);

        convert(buffer, strlen(buffer));

        // on envoie le message
        if(send(clientSocket,buffer,strlen(buffer)+1,0)==-1){
            perror("Erreur du send\n");
            exit(1);
        }
        

        if(strcmp(buffer,"/file")==0){

            pthread_t sendfile;

            pthread_create(&sendfile, NULL, envoi_fichier, NULL);

            pthread_join(sendfile, NULL);

        }
        else if (strcmp(buffer,"/filename")==0){

            printf("\n");
            printf("-------------FILES-------------\n");
            show_dir_content("./filetosend");
            printf("-------------FILES-------------\n");
            printf("\n");

        }
        else {

            // on regarde si le message envoyé est "fin"
            if(strcmp(buffer,"fin")==0){
                EXIT = 1;
            }
        }
    }
    pthread_exit(NULL);
}


// le thread reception permet de recevoir un message du serveur et de l'afficher
void *reception(void *arg){
    int EXIT = 0;

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
                // Si le message reçu n'est pas fin
                if(strcmp(buffer,"fin")!=0){

                    if(strcmp(buffer,"/file")==0){

                        pthread_t receivefile;

                        pthread_create(&receivefile, NULL, recevoir_fichier, NULL);

                        pthread_join(receivefile, NULL);
                    }
                    else{
                        // on affiche le message
                        puts(buffer);

                        // on vide le buffer
                        memset(&buffer[0], 0, sizeof(buffer));
                    }
                }
                else{
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


