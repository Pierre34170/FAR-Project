#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/time.h>


#define NB_MAX_CLIENTS 3
#define LONGEUR_MAX_PSEUDO 20
#define TAILLE_BUFFER 1024
#define TAILLE_RCV_BUFFER 10000

// déclaration des variables serveur et clients
int dS;
struct sockaddr_in serverAddr,cli;
socklen_t lgA;
char buffer[TAILLE_BUFFER];
int clients[NB_MAX_CLIENTS] = {0}; // tableau contenant les sockets des clients connectés
int fileok = 1; // variable qui va permettre de voir si on va pouvoir supprimer le fichier après l'envoi au client (le fichier doit être supprimer seulement une fois que tous les clients ont reçu correctement le fichier)


// Structure qui contient l'indice pour i dans le tableau clients
// Ce struct servira à être transmit au thread
struct client_info
{
    int index_socket;
};
struct client_info client_struct;


pthread_mutex_t mutex_client = PTHREAD_MUTEX_INITIALIZER;

// fonction qui cherche une place libre dans le tableau des sockets
// clients[i] == 0 => place i est libre
// retourne l'index de la socket dans le tableau si il y a une place
// retourne -1 si il n'y a pas de place
int chercher_place(){
    int i = 0;
    while(i<NB_MAX_CLIENTS){
        // Si place trouvée
        if(clients[i]==0){
            return i;
        }
        i++;
    }

    // Place non trouvée => serveur plein
    return -1;
}

// fonction qui permet de compter le nombre de client connecté
int count_client(){

    int nb_client=0;
    int i = 0;
    while(i<NB_MAX_CLIENTS){

        if(clients[i]!=0){
            nb_client +=1;
        }
        i++;
    }
    return nb_client;

}

void error_file(int nb){
    pthread_mutex_lock(&mutex_client);
    if (nb == 1){
        fileok = 1;
    } 
    else if (nb == 0){
        fileok = 0;
    }
    pthread_mutex_unlock(&mutex_client);
}


// thread_client(i) permet de recevoir le message du client i et de l'envoyer aux clients connectés
void *thread_client(void *arg){

    struct timespec tim, tim2;
    tim.tv_sec = 0;
    tim.tv_nsec = 250000000L; // la fréquence d'envoie des paquets

    int EXIT = 0;
    // Réceptacle pour l'index i du tableau client correspondant 
    // au client concerné par le thread
    struct client_info *client=(void *)arg;

    // Variables d'info sur les clients
    char pseudo[LONGEUR_MAX_PSEUDO];
    int socket;
    int index;
    char buffer[TAILLE_BUFFER];     // Contient les contenus de message
    char message[1024];             // Représente le message envoyé, càd ce qui va s'affiche chez le client (avec le pseudo)

    index = client->index_socket;
    socket = clients[index];

    // Demande du pseudo au client
    strcpy(buffer,"Veuillez entrer votre pseudo : ");
    if(send(socket,buffer,sizeof(buffer),0)==-1){
        printf("Erreur send, arrêt du serveur \n");
        exit(1);
    }
    memset(&buffer[0], 0, sizeof(buffer));

    // Tant qu'on a pa reçu le pseudo on attend 
    while(recv(socket, buffer, 1024, 0)==-1){
            
    }

    if (strcmp(buffer, "fin")!=0){
        printf("Pseudo du client %d : %s \n",index,buffer);
    }

    // On stock le pseudo reçu dans pseudo
    strcpy(pseudo,buffer);

    // SI le client choisi "fin" comme pseudo il est directement deconnecté
    if (strcmp(pseudo,"fin")==0)
    {
        EXIT = 1;
        clients[index]=0;
        strcpy(buffer,"fin");
        if(send(socket,buffer,sizeof(buffer),0)==-1){
            printf("Erreur send, arrêt du serveur \n");
            exit(1);
        }
        
    }
    else {
        // On confirme au client qu'il peut commencer à entrer des messages
        strcpy(buffer,"Vous pouvez discuter ");
        if(send(socket,buffer,sizeof(buffer),0)==-1){
            printf("Erreur send, arrêt du serveur \n");
            exit(1);
        }
    }

    

    // on vide le buffer
    memset(&buffer[0], 0, sizeof(buffer));

    // Boucle qui attend les messages du client, on sort de la boucle si le client envoi fin
    // cela le deconnecte et libère une place dans le tableau
    while(EXIT==0){
        while(recv(socket, buffer, 1024, 0)==-1){
            
        }
            strcpy(message,"");
            strcat(message,pseudo);
            strcat(message," : ");
            strcat(message,buffer);

            FILE *fp;
            char filename[TAILLE_BUFFER]; // nom du fichier reçu
            int size_file; // taille du fichier reçu
            int recv_size=0; // le nombre d'octets total que l'on a reçu
            int read_size; // le nombre d'octets du paquet que l'on reçoit 
            int packet_index=0; // indexe du paquet reçu
            int write_size; // le nombre d'octet que l'on écrit dans le fichier 
            char rcv_buffer[TAILLE_RCV_BUFFER]; // buffer dans lequel on stocke les données reçues
            char send_buffer[TAILLE_RCV_BUFFER]; // contient les données que l'on va transmettre aux clients
            int envoi; // le nombre d'octets envoyés au client
            int total_send = 0; // le nombre total d'octets envoyés
            char path[200]; // contient le chemin du fichier

            int i = 0; // index des clients dans le tableau

            // le cas ou le client envoie un fichier
            if (strcmp(buffer, "/file")==0){

                memset(&buffer[0], 0, sizeof(buffer));


                // Tant qu'on a pas reçu le nom du fichier on attend 
                while(recv(socket, buffer, sizeof(buffer), 0)==-1){

                }

                printf("nom du fichier reçu : ");
                puts(buffer);
                printf("\n");
                strcpy(filename,buffer);

                memset(&buffer[0], 0, sizeof(buffer));

                // Tant que l'on a pas reçu la taille du fichier on attend
                while(recv(socket, buffer, sizeof(buffer), 0)==-1){

                }
                size_file=atoi(buffer);
                printf("taille du fichier reçu : %d\n", size_file);
                printf("\n");
                
                // on construit le chemin du fichier à envoyer
                snprintf(path, sizeof(path), "./filetosubmit/%s", filename);

                fp = fopen(path, "wb");

                if(fp == NULL){
                    perror("Le fichier ne peut pas être ouvert\n");
                    exit(1);
                }

                printf("Reception du fichier...\n");

                struct timeval timeout;
                fd_set fds;
                int buffer_fd;

                // on attend pendant 1 secondes max
                timeout.tv_sec = 1;
                timeout.tv_usec = 0;

                while(recv_size < size_file){
                    FD_ZERO(&fds);
                    FD_SET(socket, &fds);

                    buffer_fd = select(FD_SETSIZE, &fds, NULL, NULL, &timeout);


                    if (buffer_fd<0){
                        perror("erreur");
                        exit(1);
                    }

                    // le cas ou le delais du time out à expiré et que l'on a toujours rien reçu
                    if (buffer_fd==0){
                        perror("timeout expired");
                        exit(1);
                    }

                    if (buffer_fd>0){

                        // on reçoit les paquets de données du client
                        do {
                            read_size =recv(socket, rcv_buffer, sizeof(rcv_buffer),0);
                        }while (read_size<0);

                        printf("Paquet reçu numéro %d\n", packet_index);
                        printf("%d octets reçus\n", read_size);
                        printf("\n");

                        // on ecrit les données lu dans le fichier et on recommence si jamais il y a une erreur dans l'enregistrement des données
                        // c'est à dire si on enregistre pas le même nombre de données que l'on a reçu
                        do {
                            write_size = fwrite(rcv_buffer, 1, read_size, fp);
                            printf("données enregistrés : %d octets\n", write_size);
                            printf("\n");
                        }while (read_size != write_size);

                        recv_size = recv_size + read_size;
                        packet_index ++;
                    }
                }


                fclose(fp);

                memset(&buffer[0], 0, sizeof(buffer));

                printf("Fichier bien enregistré\n");
                printf("\n");

                printf("On transmet maintenant aux clients\n");
                printf("\n");


                while(i<sizeof(clients)){
                    packet_index =0;
                    if (clients[i]>0 && i!=index){

                        strcpy(buffer, "/file");

                        // on envoie le mot-clès qui permettra de déclanché le protocole chez le client
                        if(send(clients[i],buffer,strlen(buffer)+1,0)==-1){
                            printf("Erreur d'envoi, arrêt du serveur\n");
                            exit(1);
                        }
                        nanosleep(&tim, &tim2);

                        // on envoi le nom du fichier
                        if(send(clients[i],filename,strlen(filename)+1,0)==-1){
                            printf("Erreur d'envoi, arrêt du serveur\n");
                            exit(1);
                        }
                        nanosleep(&tim, &tim2);

                        sprintf(buffer, "%d", size_file);

                        // on envoie la taille
                        if(send(clients[i],buffer,strlen(buffer)+1,0)==-1){
                            printf("Erreur d'envoi, arrêt du serveur\n");
                            exit(1);
                        }

                        nanosleep(&tim, &tim2);

                        printf("Envoi du fichier...\n");

                        fp = fopen(path, "rb");

                        total_send=0;

                        while(!feof(fp)){

                            // on lit le fichier et on met dans notre buffer d'envoi
                            read_size = fread(send_buffer, 1, sizeof(send_buffer), fp);


                            //on envoie les données (si il y a eu une erreur avec send on renvoie)
                            do{
                                envoi = send(clients[i], send_buffer, read_size, 0);  
                            }while (envoi < 0);

                            total_send += envoi;

                            printf("paquet numéro %d\n",packet_index);
                            printf("%d octets envoyés\n", read_size);
                            printf("\n");
                            packet_index ++;

                            // on remet à 0 tous les octets du buffer d'envoi
                            bzero(send_buffer, sizeof(send_buffer));
                            nanosleep(&tim, &tim2);

                        }

                        if (total_send == size_file ){
                            printf("Envoi au client %d réussis\n", i);
                            printf("\n");
                        }
                    }
                    i++;
                }

                sleep(1);

                // tant que le fichier n'est pas prêt à être supprimer on attend
                while (fileok==0){

                }

                int delete_status = unlink(path);
                if(delete_status!=0) {
                    printf("Erreur, le fichier ne peut pas être supprimé !\n");
                }
                else {
                    printf("Le fichier %s a été correctement supprimé !\n", filename);
                }

                memset(&buffer[0], 0, sizeof(buffer));
            }

            else if (strcmp(buffer, "ERROR_FILE")==0){

                // on change la variable fileok à 0 car il y à eu une erreur donc le fichier doit être renvoyé et il faut donc bloqué la suppression du fichier
                error_file(0);
                printf("%d\n", fileok);

                memset(&buffer[0], 0, sizeof(buffer));

                // on remet à 0 tous les octets du buffer d'envoi
                bzero(send_buffer, sizeof(send_buffer));

                // Tant qu'on a pas reçu le nom du fichier qui doit être renvoyé on attend 
                while(recv(socket, buffer, sizeof(buffer), 0)==-1){

                }

                strcpy(filename,buffer);

                snprintf(path, sizeof(path), "./filetosubmit/%s", filename);



                    fp = fopen(path, "rb");

                    nanosleep(&tim, &tim2);

                    while(!feof(fp)){

                        // on lit le fichier et on met dans notre buffer d'envoi
                        read_size = fread(send_buffer, 1, sizeof(send_buffer), fp);


                        //on envoie les données (si il y a eu une erreur avec send on renvoie)
                        do{
                            envoi = send(socket, send_buffer, read_size, 0);  
                        }while (envoi < 0);

                        total_send += envoi;

                        printf("paquet numéro %d\n",packet_index);
                        printf("%d octets envoyés\n", read_size);
                        printf("\n");
                        packet_index ++;

                        // on remet à 0 tous les octets du buffer d'envoi
                        bzero(send_buffer, sizeof(send_buffer));
                        nanosleep(&tim, &tim2);


                    }
                    // on change la variable fileok à 1 pour dire que le fichier est prêt à être supprimé
                    error_file(1);
                    printf("%d\n", fileok);
            }
    
            // Sortir de la boucle si le client a envoyé "fin"
            else if (strcmp(buffer, "fin")==0)
            {   
                printf("\n");
                printf("*** Déconnexion du client : %s ***\n",pseudo);
                // on renvoi le message "fin" au client qui l'a envoyé pour le terminer
                if(send(socket,buffer,sizeof(buffer),0)==-1){
                    printf("Erreur d'envoi, arrêt du serveur\n");
                    exit(1);
                }
                EXIT = 1;
                // le client passe en status déconnecté, la place est libérée
                clients[index]=0;
            }
            //sinon
            else if (strcmp(buffer, "/filename")!=0){
                int i = 0;
                // le renvoyer aux clients
                while(i<sizeof(clients)){
                    if(clients[i]>0 && i!=index){
                        if(send(clients[i],message,sizeof(message),0)==-1){
                            printf("Erreur d'envoi, arrêt du serveur\n");
                            exit(1);
                        }
                        printf("\n");
                        printf("Envoi du message : %s\n",buffer);
                        printf("Emetteur : client %d -- Destinataire : client %d\n",index,i );
                    }
                    i++;
                }

                // on vide le buffer
                memset(&buffer[0], 0, sizeof(buffer));
            }
        }
    pthread_exit(NULL);
}



int main(int argc, char * argv[]) {

    // Paramétrage du serveur
    dS = socket(PF_INET, SOCK_STREAM, 0);

    if(dS==-1){
        printf("Erreur de la création de la socket, arrêt du serveur");
        exit(1);
    }
    printf("Réussite de la création de la socket\n");

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8081);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.01");

    if(bind(dS, (struct sockaddr *) &serverAddr, sizeof(serverAddr))==-1){
        printf("Erreur du bind, arrêt du serveur\n");
        exit(1);
    }
    printf("Réussite du bind\n");

    // Serveur en mode écoute
    if (listen(dS,5)==-1){
        printf("Erreur du listen, arrêt du serveur\n");
        exit(1);
    }
    printf("Écoute...\n");

    // Liaison entre serveur et les deux clients
    lgA = sizeof cli;
    pthread_t thread_clients[NB_MAX_CLIENTS];
    int socket,index;

    while(1){
        socket = accept(dS, (struct sockaddr *) &cli, &lgA);
        if(socket==-1)
        {
            printf("Erreur du accept, arrêt du serveur \n");
            exit(1);
        }

        // on stock le descripteur de socket dans le tableau
        // on entre dans une section critique donc on bloque l'accès aux autres threads si un threads se trouve dans cette section
        pthread_mutex_lock(&mutex_client);

        index = chercher_place();
        if (index>-1){
            clients[index]=socket;
            client_struct.index_socket=index;
            printf("\n");
            printf("Client %d connecté !\n",index);
            //on lance le thread client
            pthread_create(&thread_clients[index], NULL, thread_client, &client_struct);

            printf("juste après thread clients[%d] = %d\n",index,clients[index]);

        } else {
            // Le serveur est plein on refuse le client
            printf("\n");    
            printf("Refus du client\n");
            strcpy(buffer,"Serveur plein");
            if(send(socket,buffer,sizeof(buffer),0)==-1){
                    printf("Erreur d'envoi, arrêt du serveur\n");
                    exit(1);
                }
            memset(&buffer[0], 0, sizeof(buffer));
            strcpy(buffer,"fin");

            sleep(1);

            if(send(socket,buffer,sizeof(buffer),0)==-1){
                    printf("Erreur d'envoi, arrêt du serveur\n");
                    exit(1);
                }
            memset(&buffer[0], 0, sizeof(buffer));
        }

        pthread_mutex_unlock(&mutex_client);
        
    }
    return 0;
}