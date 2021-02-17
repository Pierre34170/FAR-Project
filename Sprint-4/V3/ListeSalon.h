#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "salon.h"

// Noeuds contenant les salons
struct Node { 
    struct salon * data; 
    struct Node* next; // Pointer to next node in DLL 
    struct Node* prev; // Pointer to previous node in DLL 
};

// Pré: Aucun autre salon ne possède le même numéro de salon dans la liste
// Ajoute le salon à la liste de salons dispo
void push(struct Node** head_ref, struct salon * new_data)  
{  
    struct Node* new_node = (struct Node*)malloc(sizeof(struct Node));  
  
    new_node->data = new_data;  
  
    new_node->next = (*head_ref);  
    new_node->prev = NULL;  
  
    if ((*head_ref) != NULL)  
        (*head_ref)->prev = new_node;  
  
    (*head_ref) = new_node;  
}  

// Charge les informations de tous les salons de la liste de salon passée en paramètre
void printList(struct Node* node,char buffer[1024])  
{  
    while (node != NULL) { 
        print_salon(node->data,buffer); 
        node = node->next;  
    }    
} 

// head_ref est la tete de la liste
// Retourne 0 si supprimé
// Retourne 1 si pas supprimé
int deleteNode(struct Node** head_ref, int numero_salon)  
{  
    struct Node* current = *head_ref;
    struct Node* del = NULL;
    int notFound = 1;

    while(current != NULL && notFound){
        if(getNumeroSalon(current->data) == numero_salon){
            notFound=0;
            if(current==*head_ref){
                // Salon est la tête
                del = *head_ref;
                /*del = (*head_ref);
                (*head_ref)->next->prev=NULL;
                (*head_ref)=(*head_ref)->next;*/
                *head_ref = del->next; 
            } else if(current->next == NULL) {
                // Salon est le dernier
                del = current;
                current->prev->next=NULL;
            } else {
                // Salon est au milieu
                del = current;
                current->prev->next=current->next;
                current->next->prev=current->prev;
            }
        } 
        current=current->next;  
    } 
    free(del);
    return notFound;
}

// Retourne le salon qui a pour numero le numéro passé en paramètre
// Retourn NULL si aucun salon ne correspond
struct salon* getSalon(struct Node** head_ref,int numero_salon){
    struct Node* current = *head_ref;
    int notFound = 1;
    while(current != NULL && notFound){
        if(getNumeroSalon(current->data) == numero_salon){
            notFound=0;
        } else {
            current=current->next;
        }
    }

    if(current==NULL){
        return NULL;
    } else {
        return current->data;
    }
}
 