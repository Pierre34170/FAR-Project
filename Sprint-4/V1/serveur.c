#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>

#include "salon.h"


#define NB_MAX_CLIENTS 6
#define LONGEUR_MAX_PSEUDO 20
#define TAILLE_BUFFER 1024
#define NOMBRE_SALONS 5

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
struct salon *salons[NOMBRE_SALONS];

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

void afficher_salons(char buffer[1024]){
    int i;
    for(i=0;i<5;i++){
        print_salon(salons[i],buffer);
    }
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

// Retourne le numéro du salon s'il est valide, 
// -1 sinon
int nombreValable(char *buffer){
    int numSalon=0;
    sscanf(buffer,"%d",&numSalon);
    printf("Num salon = %d\n",numSalon);
    if((numSalon<=0)||(numSalon>NOMBRE_SALONS)){
        numSalon = -1;
    }
    return numSalon;
}

// On enlève le client du tableau et on lui envoi "fin"
void deconnecter_client(int numSalon, int index, int socket){
        salons[numSalon]->clients[index]=0;
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

    index = client->index_socket;
    socket = salons[numSalon]->clients[index];

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
                while(i<2){
                    if(salons[numSalon]->clients[i]>0 && i!=index){
                        if(send(salons[numSalon]->clients[i],buffer,sizeof(message),0)==-1){
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
                while(i<2){
                    if(salons[numSalon]->clients[i]>0 && i!=index){
                        if(send(salons[numSalon]->clients[i],message,sizeof(message),0)==-1){
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

void *connexion_salon(void* arg){
    int index,numSalonChoisit;
    int *socket1 = (int*) arg;
    int socket = *socket1;
    do{
             // On demande au client de choisir un salon
            strcpy(buffer,"Entrez un numéro de salon (Consulter les infos des salons : '/list-channels')");
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
            else if (strcmp(buffer,"/list-channels")==0){
                    memset(&buffer[0], 0, sizeof(buffer));
                    afficher_salons(buffer);
                    send(socket,buffer,sizeof(buffer),0);
                }
            numSalonChoisit=nombreValable(buffer);

         } while(numSalonChoisit==-1);
        numSalonChoisit=numSalonChoisit-1;
        index = chercher_place_salon(salons[numSalonChoisit]);
        if (index>-1){
            // Tableau de struct de socket client
            sock[index]=cli;
            printf("Le port est %d\n",(int) ntohs(sock[index].sin_port));
            printf("L'adresse est : %s\n",inet_ntoa(sock[index].sin_addr));
            // Tableau de descripteur de socket
            salons[numSalonChoisit]->clients[index]=socket;
            // Passer l'index de la socket dans le tableau pour le thread
            client_struct.index_salon=numSalonChoisit;
            client_struct.index_socket=index;
            printf("\n");
            printf("Client %d connecté !\n",index);
            //on lance le thread client
            pthread_create(&thread_clients[index], NULL, thread_client, &client_struct);
            printf("juste après thread clients[%d] = %d\n",index,clients[index]);

        } else if(index<=-1){
            // Le serveur est plein on refuse le client
            printf("\n");    
            printf("Refus du client\n");
            strcpy(buffer,"Salon plein");
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

    // ------ Création des 5 salons 

    struct salon *s1 = NULL;
    s1 = (salon *) malloc(sizeof(salon));
    strcpy(s1->nom,"Github");
    strcpy(s1->description,"Ici on parle beaucoup de Github !");
    s1->index=1;

    struct salon *s2 = NULL;
    s2 = (salon *) malloc(sizeof(salon));
    strcpy(s2->nom,"Slack");
    strcpy(s2->description,"Ici on parle beaucoup de Slack !");
    s2->index=2;

    struct salon *s3 = NULL;
    s3 = (salon *) malloc(sizeof(salon));
    strcpy(s3->nom,"NodeJS");
    strcpy(s3->description,"Pour les passionnés de NodeJS");
    s3->index=3;

    struct salon *s4 = NULL;
    s4 = (salon *) malloc(sizeof(salon));
    strcpy(s4->nom,"FAR");
    strcpy(s4->description,"Pour rechercher de l'aide sur les sprint difficiles");
    s4->index=4;

    struct salon *s5 = NULL;
    s5 = (salon *) malloc(sizeof(salon));
    strcpy(s5->nom,"Divers");
    strcpy(s5->description,"On parle de tout et n'importe quoi ici !");
    s5->index=5;

    salons[0]=s1;
    salons[1]=s2;
    salons[2]=s3;
    salons[3]=s4;
    salons[4]=s5;

    // ------ Création des 5 salons

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