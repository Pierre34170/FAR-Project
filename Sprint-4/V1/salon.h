#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct salon {
    int index;
    char nom[20];
    char description[140];
    int clients[2];
} salon ;

// Fonction qui compte le nombre de place libre dans un salon
int place_libres(salon * s){
    int places=0;
    int i;
    for(i=0; i<2 ; i++){
        if (s->clients[i]==0){
            places++;
        }
    }
    return places;
}

// Fonction qui prend un salon en paramètre et un buffer
// Place dans le buffer les infos actuelles du salon
void print_salon(salon * s,char affichage[1024]){
    char buffer[300]="";
    sprintf(buffer,"----- Salon numéro : %d -----\n",s->index);
    strcat(affichage,buffer);
    sprintf(buffer,"Nom : %s\n",s->nom);
    strcat(affichage,buffer);
    sprintf(buffer,"Description : %s\n",s->description);
    strcat(affichage,buffer);
    sprintf(buffer,"Places libres : %d\n",place_libres(s));
    strcat(affichage,buffer);
}

// Fonction qui cherche une place dans un salon
// retourn l'index de la place libre si on trouve une place libre
// retourne -1 si aucune place trouvée
int chercher_place_salon(salon * s){
    int i = 0;
    while(i<2){
        // Si place trouvée on retourne l'index libre
        if(s->clients[i]==0){
            return i;
        }
        i++;
    }

    // Place non trouvée => salon plein
    return -1;
}






