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
 
#ifndef CONNECTIONS_H_
#define CONNECTIONS_H_

#define MAX_RETRIES     10
#define MAX_SLEEPING     3
#if !defined(UNIX_PATH_MAX)
#define UNIX_PATH_MAX  64
#endif

#include <message.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <config.h>

/**
 * @file  connection.h
 * @brief Contiene le funzioni che implementano il protocollo 
 *        tra i clients ed il server
 */

/**
 * @function openConnection
 * @brief Apre una connessione AF_UNIX verso il server 
 *
 * @param path Path del socket AF_UNIX 
 * @param ntimes numero massimo di tentativi di retry
 * @param secs tempo di attesa tra due retry consecutive
 *
 * @return il descrittore associato alla connessione in caso di successo
 *         -1 in caso di errore
 */
int openConnection(char* path, unsigned int ntimes, unsigned int secs){			//fatto
	
	struct sockaddr_un serv_addr;
    int sockfd,i=0;
    sockfd=socket(AF_UNIX, SOCK_STREAM, 0);								//creo il nuovo socket 
    
    //setto l'indirizzo
    memset(&serv_addr, '0', sizeof(serv_addr));												 
    strncpy(serv_addr.sun_path,path, UNIX_PATH_MAX);
	serv_addr.sun_family = AF_UNIX;  
	
	if(secs>MAX_SLEEPING){
		printf("Tempo troppo alto, lo abbasso a:%d\n", MAX_SLEEPING);
	}
	
	if(secs>MAX_SLEEPING){
		printf("Troppi tentativi, li abbasso a:%d\n", MAX_RETRIES);
	}
	
    while(connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr))==-1 && i<ntimes){		//cerca di connttersi con l'indirizzo del socket dato
			printf("Tentativo n:%d\n", i );
			i++;
			sleep(secs);
	}
		
	if(i==ntimes)	return -1;
		
	return sockfd;
}

// -------- server side ----- 
/**
 * @function readHeader
 * @brief Legge l'header del messaggio
 *
 * @param fd     descrittore della connessione
 * @param hdr    puntatore all'header del messaggio da ricevere
 *
 * @return <=0 se c'e' stato un errore 
 *         (se <0 errno deve essere settato, se == 0 connessione chiusa) 
 */
 
int readHeader(long connfd, message_hdr_t *hdr){
	 memset(hdr, 0, sizeof(message_hdr_t));
int lun=0;	 
lun=read(connfd, hdr, sizeof(message_hdr_t));
			
	return lun;			//tutto bene
}


/**
 * @function readData
 * @brief Legge il body del messaggio
 *
 * @param fd     descrittore della connessione
 * @param data   puntatore al body del messaggio
 *
 * @return <=0 se c'e' stato un errore
 *         (se <0 errno deve essere settato, se == 0 connessione chiusa) 
 */
 /* Da controllare!! */
int readData(long fd, message_data_t *data){
	 memset(data, 0, sizeof(message_data_t));
	 
int lun,sal;	 

lun=read(fd, &(data->hdr), sizeof(message_data_hdr_t));

	if(lun==-1) return -1;		


int lunBuf=data->hdr.len;
	
	if (lunBuf==0) data->buf = NULL;
	
	else {
		data->buf= (char *)malloc(lunBuf*sizeof(char));		//alloco il buffer 
		memset(data->buf, 0, lunBuf * sizeof(char));
		
		char *tmp = (char*)data->buf;
		while (lunBuf > 0) {
			sal = read(fd, tmp, lunBuf);
			if (sal < 0){
				 return -1;
			 }
				tmp += sal;
				lunBuf -= sal;
		}
	}
			
return 1;			//tutto bene
}
   

/**
 * @function readMsg
 * @brief Legge l'intero messaggio
 *
 * @param fd     descrittore della connessione
 * @param data   puntatore al messaggio
 *
 * @return <=0 se c'e' stato un errore
 *         (se <0 errno deve essere settato, se == 0 connessione chiusa) 
 */
 
int readMsg(long fd, message_t *msg){
	//setto il msg
	 memset(msg, 0, sizeof(message_t));
	int lun=0;
	lun=readHeader(fd, &(msg->hdr));
	if(lun==-1){		//in caso di errore nella richiesta
		return -1;		
	} 
	lun=readData(fd, &(msg->data));
	if(lun==-1){		//in caso di errore nella richiesta
		return -1;		
	} 
			
	return lun;			//tutto bene ritorna la lunghezza byte letti
}

/* da completare da parte dello studente con altri metodi di interfaccia */


// ------- client side ------

/**
 * @function sendData
 * @brief Invia il body del messaggio al server
 *
 * @param fd     descrittore della connessione
 * @param msg    puntatore al messaggio da inviare
 *
 * @return <=0 se c'e' stato un errore
 */
 

int sendData(long fd, message_data_t *msg){			//msg è la struct message_data_t quindi non di message_t!!
	int v=0;
	v=write(fd, &(msg->hdr), sizeof(message_data_hdr_t));
	if(v==-1){		//in caso di errore nella richiesta
		return -1;		
	} 

int lun,sal;
lun=msg->hdr.len;
char *buf=msg->buf;
		while (lun > 0) {
			sal = write(fd, buf, lun);
			if (sal < 0) return -1;
				buf += sal;
				lun -= sal;
		}
			
return 1;			//tutto bene
}

/**
 * @function sendHeader
 * @brief Invia l'header del messaggio al server
 *
 * @param fd     descrittore della connessione
 * @param msg    puntatore al messaggio da inviare
 *
 * @return <=0 se c'e' stato un errore
 */
 
int sendHeader(long fd, message_hdr_t *msg){			//msg è la struct message_hdr_t quindi non di message_t!!
	int c=write(fd,msg, sizeof(message_hdr_t));
	if(c==-1){		//in caso di errore nella richiesta
		return -1;		
	} 
			
	return 1;			//tutto bene

}

/**
 * @function sendRequest
 * @brief Invia un messaggio di richiesta al server 
 *
 * @param fd     descrittore della connessione
 * @param msg    puntatore al messaggio da inviare
 *
 * @return <=0 se c'e' stato un errore
 */
 
int sendRequest(long fd, message_t *msg){		
	int k=0;
	k=sendHeader(fd, &(msg->hdr));
	if(k==-1){		//in caso di errore nella richiesta
		return -1;		
	} 
	int p=0;
	p=sendData(fd, &(msg->data));
	if(p==-1){		//in caso di errore nella richiesta
		return -1;		
	} 
	
			
	return 1;			//tutto bene
}

#endif /* CONNECTIONS_H_ */
