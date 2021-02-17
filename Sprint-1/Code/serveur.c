#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>

// déclaration des variables serveur et deux clients
    int dS, Client1, Client2;
    struct sockaddr_in serverAddr,cli;
    socklen_t lgA;
    char buffer[1024];


// Fonction qui s'occupe de faire fonctionner une conversation une fois que
// deux clients soient connectés

void conversation(){
    // Variable EXIT, sert à continuer la conversation tant que personne n'a envoyé "fin"
    int EXIT = 0;

    // Envoyer un message au client 1 pour le prévenir du début de la conversation
        printf ("On prévient le client 1 que c'est son tour \n");
        strcpy(buffer,"C'est à vous de parler !");
        if(send(Client1,buffer,sizeof(buffer),0)==-1){
            printf("Erreur send, arrêt du serveur \n");
            exit(1);
        }
        memset(&buffer[0], 0, sizeof(buffer));

    while (EXIT == 0)
    {
        // Recevoir le message du client 1
        if(recv(Client1, buffer, 1024, 0)==-1){
            printf("Erreur de réception, arrêt du serveur \n");
            exit(1);
        }
       
        // Sortir de la boucle si le client 1 a envoyé "fin"
        if (strcmp(buffer, "fin")==0)
        {   
            printf("*** Déconnexion du client 1 ***\n");
            printf("Fin de la conversation, attente de la connexion de deux clients\n");
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
             // le renvoyer au client 2
            printf ("Envoi au Client 2, message transmit : '%s'\n", buffer);
            if(send(Client2,buffer,sizeof(buffer),0)==-1){
                printf("Erreur d'envoi, arrêt du serveur\n");
                exit(1);
            }
            // on vide le buffer
            memset(&buffer[0], 0, sizeof(buffer));
            // recevoir le message du client 2
            if(recv(Client2, buffer,1024, 0)==-1){
                printf("Erreur de réception, arrêt du serveur \n");
                exit(1);
            }
            if (strcmp(buffer, "fin")==0)
            {   
                printf("*** Déconnexion du client 2 ***\n");
                printf("Fin de la conversation, attente de la connexion de deux nouveaux clients\n");
                if(send(Client1,buffer,sizeof(buffer),0)==-1){
                    printf("Erreur send, arrêt du serveur\n");
                    exit(1);
                }
                EXIT = 1;
                // Clients 1 et 2 passent en status 'déconnecté'
                Client2=-1;
                Client1=-1;
            }
            //sinon
            else 
            {
                // le renvoyer au client 1
                printf ("Envoi au Client 1, message transmit : '%s'\n", buffer);
                if(send(Client1,buffer,sizeof(buffer),0)==-1){
                printf("Erreur d'envoi, arrêt du serveur\n");
                exit(1);
                }
                // si le client 2 a envoyé "fin"
            }
        }
    }
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
            conversation();
        }
        
    }
    return 0;
}