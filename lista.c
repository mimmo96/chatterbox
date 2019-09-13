/*
 * chatterbox Progetto del corso di LSO 2017/2018
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 * 
 * Autore:Domenico Profumo matricola 533695
 * Si dichiara che il programma è in ogni sua parte, opera originale dell'autore
 * 
 */
   /**
 * @file  lista.c
 * @brief Contiene la struct della lista utenti connessi 
 * 		  e le varie funzioni di gestione
 * 		   
 */
#include <stdlib.h>
#include <stdio.h>
#include <icl_hash.h>
#include <config.h>
#include <string.h>
#include "lista.h"

/**
 * @function Inserisci
 * @brief funzione per la lista del threadPool
 * 		  inserisce il fd del client nella lista del pool di thread
 *
 * @param pl puntatore testa della lista
 * @param filedescriptor descrittore connessione da inserire
 *
 * @return puntatore testa della lista
 */
TipoNodoLista *Inserisci(TipoNodoLista *pl, int filedescriptor) {	
TipoNodoLista *t;
TipoNodoLista *curr;
			/* caso lista inizialmente vuota */
  if(pl==NULL) {
    pl=malloc(sizeof(struct NodoLista));
    pl->fd=filedescriptor;
    pl->next=NULL;
    return pl;
  }

  t=pl;
	/* vado avanti fino alla fine della lista */
  while(t->next!=NULL)
    t=t->next;
	/* qui t punta all'ultima struttura della lista: ne
	creo una nuova e sposto il puntatore in avanti */
  curr=malloc(sizeof(struct NodoLista));
  curr->fd=filedescriptor;
  curr->next=NULL;
	t->next=curr;
  

return pl;
}

/**
 * @function Inserisciconn
 * @brief funzione per la lista connessi
 * 		  mi inserisce il nome utente connesso e il suo fd
 *
 * @param pl puntatore testa della lista
 * @param el nome utente connesso
 * @param filedescriptor descrittore connessione da inserire
 *
 * @return puntatore testa della lista
 */
TipoNodoLista *Inserisciconn(TipoNodoLista *pl, char * el, int filedescriptor) {	
TipoNodoLista *t;
  TipoNodoLista *curr;
			/* caso lista inizialmente vuota */
  if(pl==NULL) {
    pl=malloc(sizeof(struct NodoLista));
    strcpy(pl->nome,el);
    pl->fd=filedescriptor;
    pl->next=NULL;
    return pl;
  }

  t=pl;
	/* vado avanti fino alla fine della lista */
  while(t->next!=NULL){
		if(strcmp(t->nome,el)==0){
			  t->fd=filedescriptor;
			  printf("Già c'era -------------------\n");
			 return pl;
		 }
		t=t->next;
	}
	/* qui t punta all'ultima struttura della lista: ne
	creo una nuova e sposto il puntatore in avanti */
  curr=malloc(sizeof(struct NodoLista));
  strcpy(curr->nome,el);
  curr->fd=filedescriptor;
  curr->next=NULL;
	t->next=curr;
  
return pl;
}

/**
 * @function Elimina
 * @brief funzione per la lista del threadPool
 *   	  elimina l'elemento in testa dalla lista
 *
 * @param pl puntatore testa della lista
 *
 * @return nuovo puntatore testa della lista
 */
TipoNodoLista *Elimina(TipoNodoLista *testa){
TipoNodoLista *curr;

	if(testa==NULL) return NULL;
	
	else{	
			curr=testa;
			printf("tolgo l'elemento: %d", testa->fd);
			testa=testa->next;
	}
	
free(curr);
return testa;
}

/**
 * @function stampa
 * @brief funzione per la lista connessi
 *   	 stampa il numero di utenti connessi
 *
 * @param pl puntatore testa della lista connessi
 *
 */
void stampa(TipoNodoLista *pl){
TipoNodoLista *curr=pl;
printf("La Lista è: ");
	while(curr!=NULL){
		 printf("->%s", curr->nome);
		curr=curr->next;
	}
	printf("\n");
}

/**
 * @function Find
 * @brief funzione per la lista connessi
 *   	  trova il nome utente(verificare se è connesso)
 *
 * @param pl puntatore testa della lista
 * @param name nome utente
 *
 * @return il suo fd se esiste, 0 altrimenti
 */

int Find(TipoNodoLista *pl,char *name){
TipoNodoLista *curr=pl;

	while(curr!=NULL){
		if(strcmp(name,curr->nome)==0){
			 return curr->fd;	
		 }
		curr=curr->next;
	}
	
return 0;
}

/**
 * @function Findfd
 * @brief funzione per la lista connessi,
 *   	  cerca il nome associato a quel file descriptor
 * 		  (lo utilizzo per verificare se è online)
 *
 * @param pl puntatore testa della lista
 * @param filedescriptor descrittore connessione
 *
 * @return nome associato al filedescriptor, NULL altrimenti
 */
char * Findfd(TipoNodoLista *pl,int filedescriptor){
TipoNodoLista *curr=pl;

	while(curr!=NULL){
		if(filedescriptor==curr->fd){
			 return curr->nome;	//trovato
		 }
		curr=curr->next;
	}
	
return NULL;
}

/**
 * @function delete
 * @brief funzione per la lista connessi,
 *   	  cancella utente(name) dalla lista
 *
 * @param pl puntatore testa della lista
 * @param name nome utente (che si è disconnesso)
 *
 * @return nuovo puntatore della lista
 */
TipoNodoLista *delete(TipoNodoLista *pl,char *name){
TipoNodoLista *curr=pl;
TipoNodoLista *appoggio=pl;

	if(pl==NULL) return NULL;
	
	if(strcmp(name,pl->nome)==0){
		pl=pl->next;
		free(curr);
		return pl;
	}

		while(curr->next!=NULL){	
			appoggio=curr->next;
			if(strcmp(name,appoggio->nome)==0){
				curr->next=curr->next->next;
				free(appoggio);
				return pl;
			}	
			curr=curr->next;
		}
	
return pl;		
}

/**
 * @function destroyList
 * @brief funzione per cancellare tutta la lista
 *
 * @param pl puntatore testa della lista
 *
 */
void destroyList(TipoNodoLista *L) {
    TipoNodoLista *p = NULL;
    
    while(L!=NULL) {
		p=L;
		L = L->next;
		free(p);
    }
 
}

/**
 * @function conta
 * @brief funzione per contare la lunghezza della lista
 *
 * @param L puntatore testa della lista
 *
 * @return lunghezza lista
 */
int conta(TipoNodoLista *L){
	TipoNodoLista *curr=L;
int conta=0;	

	while(curr!=NULL){
		conta++;
		curr=curr->next;
	}
	
	return conta;
}

/**
 * @function converti
 * @brief funzione per la lista connessi
 *   	  converti in buffer la lista degli utenti
 *
 * @param L puntatore testa della lista
 * @param arraydinomi array dove salvare i nomi utenti
 *
 */
void converti(TipoNodoLista *L,char **arraydinomi){
	
	int i=0,dim=MAX_NAME_LENGTH+1;
	TipoNodoLista *curr=L;	
  
  *arraydinomi=(char*)malloc(conta(L)*dim*sizeof(char));
 
   char *stringa = *arraydinomi;
	while(curr!=NULL){
		strncpy(stringa+(dim*i), curr->nome, dim);
		i++;
		curr=curr->next;
	}

}
