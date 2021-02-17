#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct salon {
    int numero_salon;
    int nombre_places;
    char nom_salon[140];
    char description_salon[140];
    int* clients;
} salon ;

// Fonction qui compte le nombre de place libre dans un salon
int place_libres(salon * s){
    int places=0;
    int i;
    for(i=0; i<s->nombre_places ; i++){
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
    sprintf(buffer,"----- Salon numéro : %d -----\n",s->numero_salon);
    strcat(affichage,buffer);
    sprintf(buffer,"Nom : %s\n",s->nom_salon);
    strcat(affichage,buffer);
    sprintf(buffer,"Description : %s\n",s->description_salon);
    strcat(affichage,buffer);
    sprintf(buffer,"Places libres : %d\n",place_libres(s));
    strcat(affichage,buffer);
}

// Fonction qui cherche une place dans un salon
// retourn l'index de la place libre si on trouve une place libre
// retourne -1 si aucune place trouvée
int chercher_place_salon(salon * s){
    int i = 0;
    while(i<s->nombre_places){
        // Si place trouvée on retourne l'index libre
        if(s->clients[i]==0){
            return i;
        }
        i++;
    }

    // Place non trouvée => salon plein
    return -1;
}

struct  salon* create_salon(){
    struct salon *retour = NULL;
    retour = (salon *) malloc(sizeof(salon));
    char buffer[140];
    /*printf("Entrez le nombre de place");
    scanf("%d",retour->);*/
    printf("Entrer le numéro du salon : ");
    scanf("%d",&retour->numero_salon);
    printf("\nSaisissez un nom de salon : ");
    scanf("%s",buffer);
    strcpy(retour->nom_salon,buffer);
    printf("\nSaisissez une description : ");
    scanf("%s",buffer);
    strcpy(retour->description_salon,buffer);
    return retour;
}

// Pré : n >= 0
void setNumero(int numero,struct salon * s){
    s->numero_salon=numero;
}

void SetNom(char buffer[140],struct salon * s){
    strcpy(s->nom_salon,buffer);
}

void setDescription(char buffer[140],struct salon * s){
    strcpy(s->description_salon,buffer);
}

void setNombrePlace(int nombre_place,struct salon * s){
    s->nombre_places=nombre_place;
    s->clients=calloc(nombre_place,sizeof(int));
}

int getNumeroSalon(struct salon * s){
    return s->numero_salon;
}



