#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>

#include "ListeSalon.h"


#define NB_MAX_CLIENTS 6
#define LONGEUR_MAX_PSEUDO 20
#define TAILLE_BUFFER 1024
#define NOMBRE_SALONS 5
#define TAILLE_SALON 5

// définition d'un format de transfert des données

// déclaration des variables serveur et clients
int dS;
struct sockaddr_in serverAddr,cli;
socklen_t lgA = sizeof(struct sockaddr_in);
char buffer[TAILLE_BUFFER];
int clients[NB_MAX_CLIENTS] = {0}; // tableau contenant les sockets des clients connectés
//int salons[NOMBRE_SALONS][2] = {{0}};
struct sockaddr_in sock[NB_MAX_CLIENTS];
pthread_t thread_clients[NB_MAX_CLIENTS]; // Contient tous les thread clients
struct Node* salons = NULL;  // Initialisation de la liste de Salons


// Structure qui contient l'indice pour i dans le tableau clients
// Ce struct servira à être transmit au thread
struct client_info
{
    int index_salon;
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

// Retourne le salon si le numéro entré correspond à un salon
// Retourne NULL sinon


// On enlève le client du tableau et on lui envoi "fin"
void deconnecter_client(int numSalon, int index, int socket){
    struct salon *salonClient = NULL;
    salonClient = (salon *) malloc(sizeof(salon));
    salonClient = getSalon(&salons,numSalon);
    salonClient->clients[index]=0;
    strcpy(buffer,"fin");
    if(send(socket,buffer,sizeof(buffer),0)==-1){
        printf("Erreur send, arrêt du serveur \n");
        exit(1);
    }
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
    int numSalon = client->index_salon;
    int index;
    char buffer[TAILLE_BUFFER];     // Contient les contenus de message
    char message[1024];             // Représente le message envoyé, càd ce qui va s'affiche chez le client (avec le pseudo)
    struct salon *salonCourant = NULL;
    salonCourant = (salon *) malloc(sizeof(salon));
    index = client->index_socket;
    salonCourant = getSalon(&salons,numSalon);
    printf("NumSalon : %d\n",numSalon);
    socket = salonCourant->clients[index];

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
        deconnecter_client(numSalon,index,socket);  
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
                printf("*** Déconnexion du client '%s' du salon %d ***\n",pseudo,numSalon);
                // on renvoi le message "fin" au client qui l'a envoyé pour le terminer
                deconnecter_client(numSalon,index,socket);
                EXIT = 1;
                

            } else if (strcmp(buffer, "file")==0){
                // Le client concerné par le thread veut envoyer un fichier, on envoi sa structure aux autre clients
                pthread_mutex_lock(&mutex_client);
                // On formatte les données sous la forme ip#port#
                sockaddr_inToStr(&sock[index],buffer);
                int i = 0;
                // le renvoyer aux clients
                while(i<salonCourant->nombre_places){
                    if(salonCourant->clients[i]>0 && i!=index){
                        if(send(salonCourant->clients[i],buffer,sizeof(message),0)==-1){
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
                while(i<salonCourant->nombre_places){
                    if(salonCourant->clients[i]>0 && i!=index){
                        if(send(salonCourant->clients[i],message,sizeof(message),0)==-1){
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



void *handle_salon(void* arg){

    int num_salon;
    char chatroom_name_struct[50]; // nom nom de la structure du salon que l'on créer
    char chatroom_name[200];
    char description[200];
    int *socket1 = (int*) arg;
    int socket = *socket1;
    char affichage[1024]=""; // buffer permettant d'afficher la liste des salons
    int place_salon;

    // si on souhaite créer un salon 
    if (strcmp(buffer, "/create-channel")==0){

        struct salon *saloncree = NULL;
        saloncree = (salon *) malloc(sizeof(salon));

        do {
            strcpy(buffer,"Veuillez entrer le numéro du salon que vous souhaitez créer");
            if(send(socket,buffer,sizeof(buffer),0)==-1){
                printf("Erreur send, arrêt du serveur \n");
                exit(1);
            }
            // Tant qu'on a pa reçu le numéro du salon
            while(recv(socket, buffer, 1024, 0)==-1){

            }
            sscanf(buffer,"%d",&num_salon);

            // si le salon existe deja alors saloncree!=NULL et on boucle, sinon saloncree==NULL et on continue
            saloncree=getSalon(&salons,num_salon);

        }while(saloncree!=NULL || num_salon<1);

        // on créer le nom de la structure salon
        strcpy(chatroom_name_struct,"s");
        strcat(chatroom_name_struct,buffer);

        // ---- Création d'un salon

        struct salon *chatroom_name_struct = NULL;
        chatroom_name_struct = (salon *) malloc(sizeof(salon));

        setNumero(num_salon,chatroom_name_struct);
        memset(&buffer[0], 0, sizeof(buffer));

        // on demande à l'utilisateur d'entrer un nom
        strcpy(buffer,"Veuillez entrer un nom pour ce salon");
        if(send(socket,buffer,sizeof(buffer),0)==-1){
            printf("Erreur send, arrêt du serveur \n");
            exit(1);
        }
        //on attend de recevoir le nom
        while(recv(socket, buffer, 1024, 0)==-1){

        }

        strcpy(chatroom_name,buffer);
        SetNom(chatroom_name,chatroom_name_struct);
        memset(&buffer[0], 0, sizeof(buffer));

        // on demande à l'utilisateur d'entrer une description
        strcpy(buffer,"Veuillez entrer une description");
        if(send(socket,buffer,sizeof(buffer),0)==-1){
            printf("Erreur send, arrêt du serveur \n");
            exit(1);
        }
        while(recv(socket, buffer, 1024, 0)==-1){

        }
        strcpy(description,buffer);
        setDescription(description,chatroom_name_struct);
        memset(&buffer[0], 0, sizeof(buffer));

        do { // le nombre de place doit être strictement superieur à 0
            strcpy(buffer,"Veuillez enfin entrer un nombre de place maximum");
            if(send(socket,buffer,sizeof(buffer),0)==-1){
                printf("Erreur send, arrêt du serveur \n");
                exit(1);
            }
            while(recv(socket, buffer, 1024, 0)==-1){

            }
            place_salon = atoi(buffer);

        }while(place_salon<=0);
        
        setNombrePlace(place_salon,chatroom_name_struct);
        memset(&buffer[0], 0, sizeof(buffer));

        push(&salons,chatroom_name_struct);

        // on affiche la liste des salons
        printList(salons,affichage);
        printf("%s",affichage);

        strcpy(buffer,"Le salon a bien été créer !");
        if(send(socket,buffer,sizeof(buffer),0)==-1){
            printf("Erreur send, arrêt du serveur \n");
            exit(1);
        }

    }
    else if (strcmp(buffer, "/delete-channel")==0){
        struct salon *salonsupprime = NULL;
        salonsupprime = (salon *) malloc(sizeof(salon));

        do {
            strcpy(buffer,"Veuillez entrer le numéro du salon que vous souhaitez supprimé (il ne doit pas contenir de personnes connectées)");
            if(send(socket,buffer,sizeof(buffer),0)==-1){
                printf("Erreur send, arrêt du serveur \n");
                exit(1);
            }
            while(recv(socket, buffer, 1024, 0)==-1){

            }

            sscanf(buffer,"%d",&num_salon);

            // si le salon n'existe pas alors salonsupprime == NULL et on boucle, sinon salonsupprime != NULL et on sort de la boucle
            salonsupprime=getSalon(&salons,num_salon);
  
        }while ((salonsupprime == NULL) || (salonsupprime->numero_salon==1) || (salonsupprime->nombre_places != place_libres(salonsupprime))); 
        // on ne peut pas supprimer le salon 1 ni un salon contenant des personnes connectées

        deleteNode(&salons,num_salon);

        printList(salons,affichage);
        printf("%s",affichage);

        strcpy(buffer,"Le salon a bien été supprimé");
        if(send(socket,buffer,sizeof(buffer),0)==-1){
            printf("Erreur send, arrêt du serveur \n");
            exit(1);
        }
    }
    else if (strcmp(buffer, "/modify-channel")==0){
        struct salon *salonmodifie = NULL;
        salonmodifie = (salon *) malloc(sizeof(salon));

        do {
            strcpy(buffer,"Veuillez entrer le numéro du salon que vous souhaitez modifié");
            if(send(socket,buffer,sizeof(buffer),0)==-1){
                printf("Erreur send, arrêt du serveur \n");
                exit(1);
            }
            while(recv(socket, buffer, 1024, 0)==-1){

            }

            sscanf(buffer,"%d",&num_salon);

            // si le salon n'existe pas alors salonsupprime == NULL et on boucle, sinon salonsupprime != NULL et on sort de la boucle
            salonmodifie=getSalon(&salons,num_salon);

        }while (salonmodifie == NULL);

        strcpy(buffer,"Veuillez entrer le nouveau nom du salon");
        if(send(socket,buffer,sizeof(buffer),0)==-1){
            printf("Erreur send, arrêt du serveur \n");
            exit(1);
        }
        while(recv(socket, buffer, 1024, 0)==-1){

        }
        strcpy(chatroom_name,buffer);
        SetNom(chatroom_name,getSalon(&salons,num_salon));
        memset(&buffer[0], 0, sizeof(buffer));

        strcpy(buffer,"Veuillez entrer une nouvelle description");
        if(send(socket,buffer,sizeof(buffer),0)==-1){
            printf("Erreur send, arrêt du serveur \n");
            exit(1);
        }
        while(recv(socket, buffer, 1024, 0)==-1){

        }
        strcpy(description,buffer);
        setDescription(description,getSalon(&salons,num_salon));
        memset(&buffer[0], 0, sizeof(buffer));

        if (salonmodifie->nombre_places == place_libres(salonmodifie)){
            do { // le nombre de place doit être strictement superieur à 0
                strcpy(buffer,"Veuillez enfin entrer un nombre de place maximum");
                if(send(socket,buffer,sizeof(buffer),0)==-1){
                    printf("Erreur send, arrêt du serveur \n");
                    exit(1);
                }
                while(recv(socket, buffer, 1024, 0)==-1){

                }
                place_salon = atoi(buffer);

            }while(place_salon<=0);

            setNombrePlace(place_salon,salonmodifie);
            memset(&buffer[0], 0, sizeof(buffer));
        }

        printList(salons,affichage);
        printf("%s",affichage);

        strcpy(buffer,"Le salon a bien été mis à jour");
        if(send(socket,buffer,sizeof(buffer),0)==-1){
            printf("Erreur send, arrêt du serveur \n");
            exit(1);
        }


    }
    pthread_exit(NULL);
}


void *connexion_salon(void* arg){
    int index;
    int *socket1 = (int*) arg;
    int socket = *socket1;
    int numSalonEntre=0;
    struct salon *salonChoisit = NULL;
    salonChoisit = (salon *) malloc(sizeof(salon));
    do{
             // On demande au client de choisir un salon
            strcpy(buffer,"Entrez un numéro de salon pour y acceder (le numéro doit être strictement superieur à 0): \n#Consulter infos salons : '/list-channels'  \n#Création de salon : '/create-channel' \n#suppression de salon : '/delete-channel' \n#modification de salon : '/modify-channel'");
            if(send(socket,buffer,sizeof(buffer),0)==-1){
            printf("Erreur send, arrêt du serveurzer \n");
            exit(1);
            }
            // Tant qu'on a pa reçu le numéro du salon
            while(recv(socket, buffer, 1024, 0)==-1){}
            // Si le client envoi fin
            if (strcmp(buffer,"fin")==0 || strcmp(buffer,"file")==0){
                strcpy(buffer,"fin");
                send(socket,buffer,sizeof(buffer),0);
                pthread_exit(NULL);

            } 
            // Si le client souhaite obtenir les infos sur les salons
            else if (strcmp(buffer,"/list-channels")==0){
                    memset(&buffer[0], 0, sizeof(buffer));
                    printList(salons,buffer);
                    send(socket,buffer,sizeof(buffer),0);
                } 
                // SI le client souhaite créer, supprimer ou modifier un salon

            else if ((strcmp(buffer, "/create-channel")==0) || (strcmp(buffer, "/delete-channel")==0) || (strcmp(buffer, "/modify-channel")==0)){
                pthread_t handleSalon;
                pthread_create(&handleSalon, NULL, handle_salon, &socket);
                pthread_join(handleSalon,NULL);
                sleep(1);
            }

            sscanf(buffer,"%d",&numSalonEntre);
 //           printf("buffer : %d\n",numSalonEntre);

            salonChoisit=getSalon(&salons,numSalonEntre);

         } while(salonChoisit==NULL);
        index = chercher_place_salon(salonChoisit);
        if (index>-1){
            // Tableau de struct de socket client
            sock[index]=cli;
            printf("Le port est %d\n",(int) ntohs(sock[index].sin_port));
            printf("L'adresse est : %s\n",inet_ntoa(sock[index].sin_addr));
            // Tableau de descripteur de socket
            salonChoisit->clients[index]=socket;
            // Passer l'index de la socket dans le tableau pour le thread
            client_struct.index_salon=numSalonEntre;
            client_struct.index_socket=index;
            printf("\n");
            printf("Client %d connecté !\n",index);
            //on lance le thread client
            pthread_create(&thread_clients[index], NULL, thread_client, &client_struct);
            printf("juste après thread clients[%d] = %d\n",index,salonChoisit->clients[index]);

        } else if(index<=-1){
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

    // Création d'un premier salon
    struct salon *s1 = NULL;
    s1 = (salon *) malloc(sizeof(salon));
    setNumero(1,s1);
    strcpy(buffer,"Home");
    SetNom(buffer,s1);
    strcpy(buffer,"Salon initial pour discussions diverses");
    setDescription(buffer,s1);
    setNombrePlace(10,s1);
    push(&salons,s1);


    int socket;
    while(1){
        socket = accept(dS, (struct sockaddr *) &cli, &lgA);
        if(socket==-1)
        {
            printf("Erreur du accept, arrêt du serveur \n");
            exit(1);
        }
        printf("L'adresse IP du client est : %s\n",inet_ntoa(cli.sin_addr));
        printf("Son numéro de port est : %d\n",(int) ntohs(cli.sin_port));

        pthread_t choixSalon;
        pthread_create(&choixSalon, NULL, connexion_salon, &socket);
        sleep(1);
/*
        // on stock le descripteur de socket dans le tableau
        // on entre dans une section critique donc on bloque l'accès aux autres threads si un threads se trouve dans cette section
        pthread_mutex_lock(&mutex_client);
        pthread_mutex_unlock(&mutex_client);

*/
    }
    return 0;
}