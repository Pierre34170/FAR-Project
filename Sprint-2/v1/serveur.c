#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>

// déclaration des variables serveur et deux clients
    int dS, Client1, Client2;
    struct sockaddr_in serverAddr,cli;
    socklen_t lgA;
    char buffer[1024];

// thread_client1 permet de recevoir le message du client1 et de l'envoyer au client2
void *thread_client1(void *arg){
    int EXIT = 0;

    // on vide le buffer
    memset(&buffer[0], 0, sizeof(buffer));

    while(EXIT==0){
        // on sort de la boucle si le client1 est déconnecté (le rcv ne peut donc pas recevoir de message et renvoie une erreur)
        if(recv(Client1, buffer, 1024, 0)==-1){
            EXIT = 1;
        }
        else {
            // Sortir de la boucle si le client 1 a envoyé "fin"
            if (strcmp(buffer, "fin")==0)
            {   
                printf("*** Déconnexion du client 1 ***\n");
                printf("Fin de la conversation, attente de la connexion de deux clients\n");
                // on envoie le message "fin" au deux clients pour qu'ils puissent s'arreter.
                if(send(Client2,buffer,sizeof(buffer),0)==-1){
                    printf("Erreur d'envoi, arrêt du serveur\n");
                    exit(1);
                }
                if(send(Client1,buffer,sizeof(buffer),0)==-1){
                    printf("Erreur d'envoi, arrêt du serveur\n");
                    exit(1);
                }

                EXIT = 1;
                // clients 1 et 2 passent en status 'déconnecté'
                Client1=-1;
                Client2=-1;
            }
            //sinon
            else {
                // le renvoyer au client 2
                printf ("Envoi au Client 2, message transmit : '%s'\n", buffer);
                if(send(Client2,buffer,sizeof(buffer),0)==-1){
                    printf("Erreur d'envoi, arrêt du serveur\n");
                    exit(1);
                }
                // on vide le buffer
                memset(&buffer[0], 0, sizeof(buffer));
            }
        }
    }
    pthread_exit(NULL);
}

// thread_client2 permet de recevoir le message du client2 et de l'envoyer au client1
void *thread_client2(void *arg){
    int EXIT = 0;

    memset(&buffer[0], 0, sizeof(buffer));

    while(EXIT==0){
        // on sort de la boucle si le client2 est déconnecté (le rcv ne peut donc pas recevoir de message et renvoie une erreur)
        if(recv(Client2, buffer, 1024, 0)==-1){
            EXIT = 1;
        }
        else {

            // Sortir de la boucle si le client 1 a envoyé "fin"
            if (strcmp(buffer, "fin")==0)
            {   
                printf("*** Déconnexion du client 2 ***\n");
                printf("Fin de la conversation, attente de la connexion de deux clients\n");
                // on envoie le message "fin" au deux clients pour qu'ils puissent s'arreter.
                if(send(Client1,buffer,sizeof(buffer),0)==-1){
                    printf("Erreur d'envoi, arrêt du serveur\n");
                    exit(1);
                }
                if(send(Client2,buffer,sizeof(buffer),0)==-1){
                    printf("Erreur d'envoi, arrêt du serveur\n");
                    exit(1);
                }
                EXIT = 1;
                // clients 1 et 2 passent en status 'déconnecté'
                Client1=-1;
                Client2=-1;
            }
            //sinon
            else {
                // le renvoyer au client 1
                printf ("Envoi au Client 1, message transmit : '%s'\n", buffer);
                if(send(Client1,buffer,sizeof(buffer),0)==-1){
                    printf("Erreur d'envoi, arrêt du serveur\n");
                    exit(1);
                }
                // on vide le buffer
                memset(&buffer[0], 0, sizeof(buffer));
            }
        }
    }
    pthread_exit(NULL);
}



int main() {
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
        printf("Erreur du bind, arrêt du serveur");
        exit(1);
    }
    printf("Réussite du bind\n");

    // Serveur en mode écoute
    if (listen(dS,5)==-1){
        printf("Erreur du listen, arrêt du serveur\n");
        exit(1);
    }
    printf("Écoute...\n");

    // Liaison entre  serveur et les deux clients
    lgA = sizeof cli;
    Client1 = -1;
    Client2 = -1;

    // Boucle infinie tant que 2 clients ne sont pas connectés on attend leur connexion et quand ils sont connectés
    // on lance une conversation entre ces derniers
    while(1){
        if(Client1 == -1 && Client2 == -1 ){
            printf("Attente du client 1...\n");
            Client1 = accept(dS, (struct sockaddr *) &cli, &lgA);
            if(Client1==-1){
                printf("Erreur du accept, arrêt du serveur \n");
                exit(1);
            }
            printf("Client 1 connecté !\n");
            printf("Attente du client 2...\n");
            Client2 = accept(dS, (struct sockaddr *) &cli, &lgA);
            if(Client2==-1){
                printf("Erreur du accept, arrêt du serveur\n");
                exit(1);
            }
            printf("Client 2 connecté !\n");
            printf("Clients présents, début de la conversation\n");

            // on avertit les deux clients qu'ils peuvent commencé à discuter
            strcpy(buffer,"Vous pouvez discuter !");
            if(send(Client1,buffer,sizeof(buffer),0)==-1){
                printf("Erreur send, arrêt du serveur \n");
                exit(1);
            }
            if(send(Client2,buffer,sizeof(buffer),0)==-1){
            printf("Erreur send, arrêt du serveur \n");
            exit(1);
            }
            // on vide le buffer
            memset(&buffer[0], 0, sizeof(buffer));

            // on initialise les deux threads
            pthread_t thread1;
            pthread_t thread2;

            //on lance les deux threads
            pthread_create(&thread1, NULL, thread_client1, NULL);
            pthread_create(&thread2, NULL, thread_client2, NULL);

            // on attend que les deux threads se terminent pour pouvoir se remettre en écoute
            pthread_join(thread1, NULL);
            pthread_join(thread2, NULL);

        }
        
    }
    return 0;
}