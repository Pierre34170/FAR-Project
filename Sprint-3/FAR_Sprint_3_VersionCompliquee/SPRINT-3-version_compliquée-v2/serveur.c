#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>


#define NB_MAX_CLIENTS 6
#define LONGEUR_MAX_PSEUDO 20
#define TAILLE_BUFFER 1024

// définition d'un format de transfert des données

// déclaration des variables serveur et clients
int dS;
struct sockaddr_in serverAddr,cli;
socklen_t lgA = sizeof(struct sockaddr_in);
char buffer[TAILLE_BUFFER];
int clients[NB_MAX_CLIENTS] = {0}; // tableau contenant les sockets des clients connectés
struct sockaddr_in sock[NB_MAX_CLIENTS];


// Structure qui contient l'indice pour i dans le tableau clients
// Ce struct servira à être transmit au thread
struct client_info
{
    int index_socket;
};
struct client_info client_struct;

// on initialise un mutex
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

// Fonction pour formatter la struct sockaddr_in pour la transmettre via le buffer

void sockaddr_inToStr(struct sockaddr_in *socket, char *buffer){
    // Le client devient serveur sur le port port_client + 1000
    snprintf(buffer,TAILLE_BUFFER,"%s#%d#",
    inet_ntoa(socket->sin_addr),
    (int) ntohs(socket->sin_port)+1000
    );
printf("L'adresse IP du client est : %s\n",inet_ntoa(cli.sin_addr));
printf("Son numéro de port est : %d\n",(int) ntohs(cli.sin_port));
}

// thread_client(i) permet de recevoir le message du client i et de l'envoyer au clients connectés
void *thread_client(void *arg){
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

    // On envoi au client le port auquel il a été e

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
            // message = "pseudo_reçu : message_buffer"
            strcpy(message," "); 
            strcat(message,pseudo);
            strcat(message," : ");
            strcat(message,buffer);
    
            // Sortir de la boucle si le client a envoyé "fin"
            if (strcmp(buffer, "fin")==0)
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

            } else if (strcmp(buffer, "file")==0){
                // Le client concerné par le thread veut envoyer un fichier, on envoi sa structure aux autre clients
                pthread_mutex_lock(&mutex_client);
                // On formatte les données sous la forme ip#port#
                sockaddr_inToStr(&sock[index],buffer);
                int i = 0;
                // le renvoyer aux clients
                while(i<sizeof(clients)){
                    if(clients[i]>0 && i!=index){
                        if(send(clients[i],buffer,sizeof(message),0)==-1){
                            printf("Erreur d'envoi, arrêt du serveur\n");
                            exit(1);
                        }
                        printf("\n");
                        printf("Envoi du message : %s\n",buffer);
                        printf("Emmeteur : client %d -- Destinataire : client %d\n",index,i );
                    }
                    i++;
                }
                pthread_mutex_unlock(&mutex_client);

            }
            //sinon
            else {
                pthread_mutex_lock(&mutex_client);
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
                        printf("Emmeteur : client %d -- Destinataire : client %d\n",index,i );
                    }
                    i++;
                }
                pthread_mutex_unlock(&mutex_client);

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

    pthread_t thread_clients[NB_MAX_CLIENTS];
    /*struct sockaddr_in adCv[NB_MAX_CLIENTS];
    socklen_t lgCv = sizeof(struct sockaddr_in);*/
    int socket,index;

    while(1){
        socket = accept(dS, (struct sockaddr *) &cli, &lgA);
        if(socket==-1)
        {
            printf("Erreur du accept, arrêt du serveur \n");
            exit(1);
        }
        printf("L'adresse IP du client est : %s\n",inet_ntoa(cli.sin_addr));
        printf("Son numéro de port est : %d\n",(int) ntohs(cli.sin_port));


        // on stock le descripteur de socket dans le tableau
        // on entre dans une section critique donc on bloque l'accès aux autres threads si un threads se trouve dans cette section
        pthread_mutex_lock(&mutex_client);

        index = chercher_place();
        if (index>-1){
            // Tableau de struct de socket client
            sock[index]=cli;
            printf("Le port est %d\n",(int) ntohs(sock[index].sin_port));
            printf("L'adresse est : %s\n",inet_ntoa(sock[index].sin_addr));
            // Tableau de descripteur de socket
            clients[index]=socket;
            // Passer l'index de la socket dans le tableau pour le thread
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