#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>

// déclaration des variables
    int clientSocket;
    char buffer[1024];
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

        // on regarde si le message envoyé est "fin"
        if(strcmp(buffer,"fin")==0){
            EXIT = 1;
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
                    printf("interlocuteur : ");
                    // on affiche le message
                    puts(buffer);

                    // on vide le buffer
                    memset(&buffer[0], 0, sizeof(buffer));
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


