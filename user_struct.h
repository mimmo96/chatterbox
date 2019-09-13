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
 * @file  user_struct.h
 * @brief Contiene la struttura di ogni utente e le history dei 
 * 		  messaggi, con le eventuali operazioni su di essa 
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "message.h"

typedef struct nodo{
    char message[200];		//max lun message len +1
    char sender[MAX_NAME_LENGTH + 1];		//max lun nome utente +1
    op_t     op;
    struct nodo *next;
} history;


typedef struct {
  char nome[MAX_NAME_LENGTH + 1];				//nome utente
  int nmessage;					//conta il numero dei messaggi presenti in lista
  history *testa;				//puntatore alla lista dei messaggi
  history *inviato;				//puntatore all'ultimo messaggio inviato
  pthread_mutex_t mutex;			//mutex sulla history
} utente;


static inline unsigned int fnv_hash_function( void *key, int len ) {
    unsigned char *p = (unsigned char*)key;
    unsigned int h = 2166136261u;
    int i;
    for ( i = 0; i < len; i++ )
        h = ( h * 16777619 ) ^ p[i];
    return h;
}

/* funzioni hash per per l'hashing di interi */
static inline unsigned int ulong_hash_function( void *key ) {
    int len = sizeof(unsigned long);
    unsigned int hashval = fnv_hash_function( key, len );
    return hashval;
}

static inline int ulong_key_compare( void *key1, void *key2  ) {
    return ( *(char*)key1 == *(char*)key2 );
}

/**
 * @function cancellamessaggi
 * @brief Cancella tutti i messaggi presenti nella history 
 *
 * @param pl puntatore alla testa della history
 *
 * @return <=0 se c'e' stato un errore
 */
void cancellamessaggi(history *pl);

/**
 * @function Inseriscimessaggio
 * @brief Inserisce un nuovo messaggio nella history 
 *
 * @param u puntatore alla struttura utente
 * @param pl puntatore alla testa della history
 * @param buf puntatore al buffer contenente il messaggio da salvare
 * @param nome nome mittente del messsaggio 
 * @param operation codice operazione
 * @param maxmessage numero massimo di messaggi che deve contenere la history
 *
 * @return puntatore alla testa della history
 */
history *Inseriscimessaggio(utente *u,history *pl, char *buf, char *nome, op_t operation, int maxmessage);

/**
 * @function contamessaggidainviare
 * @brief Conta tutti i messaggi presenti nella history che non sono stati ancora inviati 
 *
 * @param L puntatore alla testa della history
 *
 * @return numero di messaggi nella history non ancora inviati
 */
int contamessaggidainviare(history *L, history *inviato);


