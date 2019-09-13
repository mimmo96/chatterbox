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
 * @file  lista.h
 * @brief Contiene la struct della lista utenti connessi 
 * 		  e le varie funzioni di gestione
 * 		   
 */
#include <stdlib.h>
#include <stdio.h>
#include <icl_hash.h>
#include <config.h>
#include <string.h>

typedef struct NodoLista {
  int fd;						
  char nome[MAX_NAME_LENGTH+1];
  struct NodoLista *next;			
  icl_hash_t *hashfun;
}TipoNodoLista;
	
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
TipoNodoLista *Inserisci(TipoNodoLista *pl, int filedescriptor);

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
TipoNodoLista *Inserisciconn(TipoNodoLista *pl, char * el, int filedescriptor);

/**
 * @function Elimina
 * @brief funzione per la lista del threadPool
 *   	  elimina l'elemento in testa dalla lista
 *
 * @param pl puntatore testa della lista
 *
 * @return nuovo puntatore testa della lista
 */
TipoNodoLista *Elimina(TipoNodoLista *testa);

/**
 * @function stampa
 * @brief funzione per la lista connessi
 *   	 stampa il numero di utenti connessi
 *
 * @param pl puntatore testa della lista connessi
 *
 */
void stampa(TipoNodoLista *pl);


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
int Find(TipoNodoLista *pl,char *name);

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
char * Findfd(TipoNodoLista *pl,int filedescriptor);

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
TipoNodoLista *delete(TipoNodoLista *pl,char *name);

/**
 * @function destroyList
 * @brief funzione per cancellare tutta la lista
 *
 * @param pl puntatore testa della lista
 *
 */
void  destroyList(TipoNodoLista *L);

/**
 * @function conta
 * @brief funzione per contare la lunghezza della lista
 *
 * @param L puntatore testa della lista
 *
 * @return lunghezza lista
 */
int conta(TipoNodoLista *L);

/**
 * @function converti
 * @brief funzione per la lista connessi
 *   	  converti in buffer la lista degli utenti
 *
 * @param L puntatore testa della lista
 * @param arraydinomi array dove salvare i nomi utenti
 *
 */
void converti(TipoNodoLista *L,char **arraydinomi);
