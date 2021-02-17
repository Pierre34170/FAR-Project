#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
int main() {
    //déclaration variables
    int clientSocket;
    char buffer[1024];
    struct sockaddr_in serverAddr;
    socklen_t lgA;
    int EXIT = 0;

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

    printf("Réussite du connect\n");

    // On continue à envoyer/recevoir des msg tant que le client n'envoi/ne reçoit pas "fin"
    while (EXIT == 0)
    {
        // si le message envoyé n'est pas "fin"
        if (strcmp(buffer, "fin")!=0)
        {
            // on vide le buffer
            memset(&buffer[0], 0, sizeof(buffer));
            // rcv == 1 si pas reçu de message
            // sinon message reçu
            int rcv = recv(clientSocket, buffer, sizeof(buffer)-1, 0);
            if(rcv == -1){
                printf("Erreur rcv\n");
                exit(1);
            } else 
                // si rcv n'est pas égal a 1 alors un message a été reçu
                if (rcv != 1)
                {
                    // Si le message reçu n'est pas fin
                    if(strcmp(buffer,"fin")!=0){
                        //afficher le message reçu
                        printf("%s\n", buffer);
                        // on vide le buffer
                        memset(&buffer[0], 0, sizeof(buffer));
                    } else {
                        // Si le message reçu est fin, fin de la conv
                        printf("Fin de la conversation\n");
                        EXIT=1;
                    }               
                }
            // si rcv est égal a 1, pas de message reçu
            else
            {
                // Le client peut saisir son message 
                printf("Vous : ");
                scanf(" %[^\n]s", buffer);
                // et l'envoyer au serveur
                if(send(clientSocket,buffer,strlen(buffer)+1,0)==-1){
                    printf("Erreur du send\n");
                    exit(1);
                }
            }
        }
        // sinon fin
        else {
            printf("Fin de la conversation\n");
            EXIT=1;
        }
    }

    return 0;
}