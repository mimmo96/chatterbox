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
 * @file  user_struct.c
 * @brief Contiene la struttura di ogni utente e le history dei 
 * 		  messaggi, con le eventuali operazioni su di essa 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "message.h"
#include "user_struct.h"

/**
 * @function cancellamessaggi
 * @brief Cancella tutti i messaggi presenti nella history 
 *
 * @param pl puntatore alla testa della history
 *
 */
void cancellamessaggi(history *pl){
history *curr=pl;
history *t;

	while(curr!=NULL){
		 t=curr;
		curr=curr->next;
		free(t);
	}

pl=NULL;
}

/**
 * @function Inseriscimessaggio
 * @brief Inserisce un nuovo messaggio nella history 
 *
 * @param pl puntatore alla testa della history
 * @param buf puntatore al buffer contenente il messaggio da salvare
 * @param nome nome mittente del messsaggio 
 * @param operation codice operazione
 * @param maxmessage numero massimo di messaggi che deve contenere la history
 *
 * @return puntatore alla testa della history
 */
history *Inseriscimessaggio(utente *u,history *pl, char *buf, char *nome, op_t operation, int maxmessage) {	
history *t;
history *curr;
int conta=1;
			/* caso lista inizialmente vuota */
  if(pl==NULL) {
    pl=malloc(sizeof(struct nodo));
    strcpy(pl->message,buf);
    strcpy(pl->sender,nome);
    pl->op=operation;
    pl->next=NULL;
    return pl;
  }

  t=pl;
	/* vado avanti fino alla fine della lista */
  while(t->next!=NULL){
	  t=t->next;
	  conta++;
	}
	/* qui t punta all'ultima struttura della lista: ne
	creo una nuova e sposto il puntatore in avanti */
	curr=malloc(sizeof(struct nodo));
	strcpy(curr->message,buf);
	strcpy(curr->sender,nome);
	curr->op=operation;
	curr->next=NULL;
	t->next=curr;
	conta++;
	
	//se ho superato il limite dei messaggi, tolgo il primo elemento in testa alla lista 
	if(conta>maxmessage){
		t=pl;
		pl=pl->next;
		u->inviato=pl;
		free(t);
	}
	
return pl;
}

/**
 * @function contamessaggidainviare
 * @brief Conta tutti i messaggi presenti nella history che non sono stati ancora inviati 
 *
 * @param L puntatore alla testa della history
 *
 * @return numero di messaggi nella history non ancora inviati
 */
int contamessaggidainviare(history *L,history *inviato){
	history *curr=L;
	int conta=0;	

	while(curr!=inviato){
		conta++;
		curr=curr->next;
	}
	
	return conta;
	
	
	
}
