/*
 * membox Progetto del corso di LSO 2017/2018
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 * 
 * Autore:Domenico Profumo matricola 533695
 * Si dichiara che il programma è in ogni sua parte, opera originale dell'autore
 * 
 */
/**
 * @file chatty.c
 * @brief File principale del server chatterbox
 */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>

/* inserire gli altri include che servono */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/un.h>	
#include <sys/mman.h>
#include "lista.c"		
#include "connections.c"
#include "user_struct.c"
#include "stats.h"
#include "icl_hash.c"

#define SYSCALL(r,c,e) \
    if((r=c)==-1) { perror(e);exit(errno); }
   
/* struttura che memorizza le statistiche del server, struct statistics 
 * e' definita in stats.h.
 *
 */
struct statistics  chattyStats = { 0,0,0,0,0,0,0 };

/* lista variabili globali che utilizzerò */
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;	//lock lista testa
static pthread_mutex_t mutexC = PTHREAD_MUTEX_INITIALIZER;	//lock lista connessi
static pthread_mutex_t mutexhash[10]={PTHREAD_MUTEX_INITIALIZER};		//lock hash
static pthread_mutex_t mutexfd_set = PTHREAD_MUTEX_INITIALIZER;	//lock fd_set
static pthread_cond_t cond=PTHREAD_COND_INITIALIZER;	

char UnixPath[200];
int MaxConnections;
int ThreadsInPool;
int MaxMsgSize;       
int MaxFileSize;      
int MaxHistMsgs;      
char DirName[200];         
char StatFileName[200];  

fd_set set;		//insieme file descriptor attivi
fd_set rdset;		//insieme file descriptor attesi in lettura

TipoNodoLista *testa=NULL;			//var globale per file desciptor
TipoNodoLista *connessi=NULL;

icl_hash_t *hash=NULL;

int fermo=0;
int lun=0;	

static void usage(const char *progname) {
    fprintf(stderr, "Il server va lanciato con il seguente comando:\n");
    fprintf(stderr, "  %s -f conffile\n", progname);
}

int chiave(icl_hash_t *ht, void* key){
    int hash_val;

    return hash_val = (* ht->hash_function)(key) % ht->nbuckets;
}	

/* funzione per il parsing */
void OpenConf(char *filename){
	FILE *fd;
	
/*variabili usate per scandire il file di conf*/	
char *buf=malloc(200*sizeof(char));
char *res;
char *primo=malloc(200*sizeof(char));
char *secondo=malloc(200*sizeof(char));
char *terzo=malloc(200*sizeof(char));

/* apro in lettura in file e verifico in caso di errore*/
	if( (fd= fopen(filename, "r")) == NULL){
		perror("open");
		exit(EXIT_FAILURE);
	}

/* metto i terminatori di fine stringa per evitare errori in fase di compilazione */
memset(primo, '\0', 200);	
memset(secondo, '\0', 200);	
memset(terzo, '\0', 200);	
memset(DirName, '\0', 200);	
memset(StatFileName, '\0', 200);
memset(UnixPath, '\0', 200);	
	
	/* il file è stato aperto con successo lo analizzo per righe*/
	while(1){
		res=fgets(buf, 200, fd);
		if(res==NULL) break;
		
		//escludo i commenti
		if(buf[0]!='#'){		
			sscanf(buf, "%s %s %s", primo, secondo, terzo);									
			
			if(strcmp(primo,"UnixPath")==0){
				strcpy(UnixPath,terzo);		
			}
			
			if(strcmp(primo,"ThreadsInPool")==0){
				ThreadsInPool=atoi(terzo);
			}
			
			if(strcmp(primo,"MaxConnections")==0){
				MaxConnections=atoi(terzo);
			}
			
			if(strcmp(primo,"MaxMsgSize")==0){
				MaxMsgSize=atoi(terzo);
			}
			
			if(strcmp(primo,"MaxFileSize")==0){
				MaxFileSize=atoi(terzo);
			}
			
			if(strcmp(primo,"MaxHistMsgs")==0){
				MaxHistMsgs=atoi(terzo);
			}
			
			if(strcmp(primo,"DirName")==0){
				strcpy(DirName,terzo);	
			}
			
			if(strcmp(primo,"StatFileName")==0){
				  strcpy(StatFileName,terzo);
			}
		}
	}

	free(buf);
	free(primo);
	free(secondo);
	free(terzo);
	fclose(fd);
}

void cleanup() {
    unlink(UnixPath);							 
}

static void gestore(int signum){	
	fermo=1;
	pthread_cond_broadcast(&cond);	
	unlink(UnixPath);
}

void stampastatistiche(){
	
FILE *fd=fopen(StatFileName, "a");	

if(fd==NULL){     
	perror("errore apertura file!");
	exit(EXIT_FAILURE);
} 

//stampo le statistiche nel file 
	printStats(fd);
	printf("Statistiche inserite con successo!\n");
	
//chiudo il file
	fclose(fd);
	
}

int esegui_comando(int connfd, message_t msg){

	printf("---------STATO SERVER:-------------------\n");
	printf("Descrittore connessione: %d\n", connfd);
	printf("Operazione: %d\n", msg.hdr.op);
	printf("Nome sender: %s\n", msg.hdr.sender);
	printf("Numero utenti registrati: %ld\n", chattyStats.nusers);
	printf("Numero utenti online: %ld\n", chattyStats.nonline);
	
	if(readData(connfd,&msg.data)<0){
		perror("readData");
		exit(EXIT_FAILURE);
	}	

int op=msg.hdr.op;

char *nome=malloc((MAX_NAME_LENGTH + 1)*sizeof(char));
strncpy(nome,msg.hdr.sender,MAX_NAME_LENGTH + 1);

char *receiver=malloc((MAX_NAME_LENGTH + 1)*sizeof(char));
strncpy(receiver,msg.data.hdr.receiver,MAX_NAME_LENGTH + 1);	 

	printf("Il buffer: %s\n", msg.data.buf);	 
	printf("Nome receiver: %s\n", receiver);
	printf("---------OPERAZIONI-------------------\n");
		
		 switch(op) {
    case REGISTER_OP:{	
	
	printf("INIZIO OPEZAZIONE REGISTER_OP\n");	
	
	/* se il client non è registrato lo registro! */
	int i=chiave(hash, (void*)nome)%10;
	pthread_mutex_lock(&mutexhash[i]);
	utente *userID=(utente *)icl_hash_find(hash, (void*)nome);
	pthread_mutex_unlock(&mutexhash[i]);
	
	if(userID==NULL){
		
		/* inizializzo le struct utente e history per la history dei messaggi */
		utente *userID=malloc(sizeof(utente));
		userID->nmessage = 0;
		strncpy(userID->nome, nome, MAX_NAME_LENGTH + 1);
		userID->mutex=(pthread_mutex_t )PTHREAD_MUTEX_INITIALIZER;
		userID->testa=NULL;
		userID->inviato=NULL;
		
		/* inserisco nella tabella hash corrispontente al nome la struct che ho creato */		
		pthread_mutex_t mutexID=userID->mutex;
		pthread_mutex_lock(&mutexID);
		icl_entry_t *controll=icl_hash_insert(hash, userID->nome, (void*)userID);			
		pthread_mutex_unlock(&mutexID);
		
		if(controll!=NULL) printf("chiave %s Inserita \n", nome);
		else{
			printf("ERROR:Utente già regitrato!\n");
			setHeader(&msg.hdr,OP_NICK_ALREADY, "");
			sendHeader(connfd,&msg.hdr);
			free(msg.data.buf);		
			free(nome);		
			free(receiver);
			chattyStats.nerrors++;
			printf("FINE OPEZAZIONE REGISTER_OP\n");	
			return -1;
		}
		
		/* inserisco l'utente nella lista connessi */
		pthread_mutex_lock(&mutexC);
		int trova=Find(connessi,nome);
		if(trova<=0){
			//inserendolo aggiorno il suo fd
			connessi=Inserisciconn(connessi,userID->nome,connfd); 					
			printf("Utente non conesso, lo inserisco nella lista connessi\n");
			chattyStats.nonline++;
		}
		stampa(connessi);
		char *c=NULL;
		converti(connessi,&c);
		int lunghezza=conta(connessi)*(MAX_NAME_LENGTH+1);
		pthread_mutex_unlock(&mutexC);
		
		setHeader(&msg.hdr, OP_OK, "");
		setData(&msg.data, "", c, lunghezza);
		chattyStats.nusers++;
		sendHeader(connfd,&msg.hdr);
		sendData(connfd,&msg.data);
		free(msg.data.buf);
		free(nome);	
		free(receiver);
		printf("FINE OPEZAZIONE REGISTER_OP\n");
	return 1;	
	} 
	
	/* se il client è già registrato mando un errore*/
	else{
		printf("ERROR:Utente già regitrato!\n");
		setHeader(&msg.hdr,OP_NICK_ALREADY, "");
		sendHeader(connfd,&msg.hdr);
		free(msg.data.buf);		
		free(nome);	
		free(receiver);
		chattyStats.nerrors++;
		printf("FINE OPEZAZIONE REGISTER_OP\n");
		return -1;
	}
	
	}
	
    case CONNECT_OP:{	

printf("INIZIO OPEZAZIONE CONNECT_OP\n");
	int i=chiave(hash, (void*)nome)%10;
	
	pthread_mutex_lock(&mutexhash[i]);
	utente *userID=icl_hash_find(hash, (void*)nome);
	pthread_mutex_unlock(&mutexhash[i]);
	
	/* controllo se l'utente è già registrato */
	if(userID==NULL){
		printf("ERROR: %s non risulta registrato!\n", nome);
		setHeader(&msg.hdr, OP_NICK_UNKNOWN, "");
		sendHeader(connfd,&msg.hdr);
		free(msg.data.buf);	
		free(nome);		
		free(receiver);
		chattyStats.nerrors++;			
		return -1;
	}
	
	char *c=NULL;
	int lunghezza=0;
	int FD=0;
	message_t mess;
	memset(&mess, 0, sizeof(message_t));
		
	/* utente già registrato, controllo se è già connesso */
		pthread_mutex_lock(&mutexC);
		printf("utente già registrato, controllo se è già connesso\n");	
	//find mi ritorna il file descriptor del client che sto cercando
		FD=Find(connessi,nome);
	//se l'utente è connesso (fd>0)
			if(FD>0){
				printf("Utente già connesso, invio la lista \n");
				converti(connessi,&c);
				lunghezza=conta(connessi)*(MAX_NAME_LENGTH+1);				
				pthread_mutex_unlock(&mutexC);
	
				setHeader(&(mess.hdr), OP_OK, "");
				setData(&(mess.data), "", c, lunghezza);
				sendHeader(connfd,&(mess.hdr));
				sendData(connfd,&(mess.data));
				
				free(c);					
				free(msg.data.buf);	
				free(nome);	
				free(receiver);
				printf("FINE OPEZAZIONE CONNECT_OP\n");
				return 1;
			}	
			
		printf("Utente %s non connesso!! Lo inserisco nella lista\n", nome);
		connessi=Inserisciconn(connessi,nome,connfd);
						  
			converti(connessi,&c);
			lunghezza=conta(connessi)*(MAX_NAME_LENGTH+1);				
			pthread_mutex_unlock(&mutexC);
				
			setHeader(&mess.hdr, OP_OK, "");
			setData(&mess.data, "", c, lunghezza);
			sendHeader(connfd,&mess.hdr);
			sendData(connfd,&mess.data);
			
			chattyStats.nonline++;				
			free(c);
			free(msg.data.buf);	
			free(nome);		
			free(receiver);
		printf("FINE OPEZAZIONE CONNECT_OP\n");
	return 1;		
	}
    
    case POSTTXT_OP:{		 
	printf("INIZIO OPEZAZIONE POSTTXT_OP\n");
		int i=chiave(hash, (void*)nome)%10;
		pthread_mutex_lock(&mutexhash[i]);
		
		utente *userID=(utente *)icl_hash_find(hash, (void*)receiver);
		pthread_mutex_unlock(&mutexhash[i]);
		
		message_t new;
		memset(&new, 0, sizeof(message_t));
		
		//controllo se l'utente è registrato
		if(userID==NULL){
			printf("ERROR: richiesta invio messaggio ad utente non registrato!\n");
			setHeader(&(new.hdr), OP_NICK_UNKNOWN, "");
			sendHeader(connfd,&(new.hdr));
			
			chattyStats.nerrors++;
			free(msg.data.buf);		
			free(nome);		
			free(receiver);	
			printf("FINE OPEZAZIONE POSTTXT_OP\n");	
		  return -1;
		 }
		
		 else{
			// Se il messaggio è troppo lungo
			if (msg.data.hdr.len > MaxMsgSize) {
				printf("ERROR: messaggio troppo lungo\n");
				setHeader(&(msg.hdr), OP_MSG_TOOLONG, "");
				sendHeader(connfd, &(msg.hdr));
				
				free(msg.data.buf);	
				free(nome);	
				free(receiver);
				chattyStats.nerrors++;					
			  return -1;
			}
				
			pthread_mutex_lock(&mutexC);
			int fdrecever=Find(connessi,msg.data.hdr.receiver);
			pthread_mutex_unlock(&mutexC);
				
			char *buffer=malloc((MaxMsgSize+1)*sizeof(char));
			strcpy(buffer,msg.data.buf);
				
			//utente online, lo mando subito
			 if(fdrecever>0){
				printf("Utente già connesso, invio il messaggio \n");
				setHeader(&new.hdr, OP_OK, "");
				sendHeader(connfd,&new.hdr);
					
				message_t mess;
				memset(&mess, 0, sizeof(message_t));
				setHeader(&(mess.hdr), TXT_MESSAGE, nome);
				setData(&(mess.data),"",buffer,msg.data.hdr.len); 
					
				// Invio subito il messaggio
				sendRequest(fdrecever, &mess);
				
				chattyStats.ndelivered++;					
				free(msg.data.buf);	
				free(nome);		//controlla ordine
				free(receiver);
				free(buffer);
				printf("FINE OPEZAZIONE POSTTXT_OP\n");
			  return 1;
			 }
			 
			//utente non connesso ma registrato, lo salvo nella history
			printf("Utente %s non connesso, salvo il messaggio nella history\n",msg.data.hdr.receiver);
			pthread_mutex_t mutexuser=userID->mutex;	
			pthread_mutex_lock(&mutexuser);			
			userID->testa=Inseriscimessaggio(userID,userID->testa,buffer, nome, msg.hdr.op, MaxHistMsgs);
			chattyStats.nnotdelivered++;	
			if(userID->nmessage<MaxHistMsgs) userID->nmessage++;
			pthread_mutex_unlock(&mutexuser);
		
			setHeader(&(new.hdr), OP_OK, "");
			sendHeader(connfd,&(new.hdr));
		
			free(msg.data.buf);		
			free(nome);	
			free(receiver);
			free(buffer);
		
			printf("FINE OPEZAZIONE POSTTEXT_OP\n");
		  return 1;
		  }
	}
	
    case POSTTXTALL_OP:{	
		printf("INIZIO OPEZAZIONE POSTTEXTALL_OP\n");
		int i=0,conta=0;
		utente *userID;
		message_t new;
		memset(&new, 0, sizeof(message_t));
		
		// Se il messaggio è troppo lungo
			if (msg.data.hdr.len > MaxMsgSize) {
				printf("ERROR: messaggio troppo lungo\n");
				setHeader(&(msg.hdr), OP_MSG_TOOLONG, "");
				sendHeader(connfd, &(msg.hdr));
				free(msg.data.buf);		
				free(nome);		
				free(receiver);
				chattyStats.nerrors++;	
				printf("FINE OPEZAZIONE POSTTEXTALL_OP\n");				
				return -1;
			 }
				
		char *buffer=malloc((MaxMsgSize+1)*sizeof(char));
			strcpy(buffer,msg.data.buf);
	
				
   /* scandisco tutta la tabella hash e se trovo che la cella è occupata mando il messaggio */
		for(i=0;i<1024;i++){
			pthread_mutex_lock(&mutexhash[i%10]);
			if(hash->buckets[i]!=NULL){
				//recupero la struttura utente
				 userID=hash->buckets[i]->data;
				 
				 /* se l'utente è connesso gli mando il messaggio altrimenti lo salvo nella history */
				pthread_mutex_lock(&mutexC);	
				int fdrecever=Find(connessi,userID->nome);
				pthread_mutex_unlock(&mutexC);
				conta++; 
				
			 if(fdrecever>0){
					printf("Utente già connesso, invio il messaggio \n");
					message_t mess;
					memset(&mess, 0, sizeof(message_t));
					
					// Invio subito il messaggio e azzero il buffer
					setHeader(&(mess.hdr), TXT_MESSAGE, nome);
					setData(&mess.data,"",buffer,msg.data.hdr.len); 
					chattyStats.ndelivered++;			
					sendRequest(fdrecever, &mess);
			}
			else{	
			//utente non online, lo salvo nella history
				 pthread_mutex_t mutexuser=userID->mutex;	
				 pthread_mutex_lock(&mutexuser);			
				 userID->testa=Inseriscimessaggio(userID,userID->testa,buffer, nome, msg.hdr.op, MaxHistMsgs);	
				 chattyStats.nnotdelivered++;				
				 if(userID->nmessage<MaxHistMsgs) userID->nmessage++;
				 pthread_mutex_unlock(&mutexuser);
			 }
		   }
			pthread_mutex_unlock(&mutexhash[i%10]);
		}
		
		free(buffer);
		
		if(conta==1){
			printf("ERROR: Nessun utente registrato oltre a me!\n");
			setHeader(&(new.hdr), OP_FAIL, "");
			sendHeader(connfd,&(new.hdr));
			
			free(msg.data.buf);		
			free(nome);		
			free(receiver);
			chattyStats.nerrors++;	
			printf("FINE OPEZAZIONE POSTTEXTALL_OP\n");				
		  return -1;
		 }
		 
		else{
			printf("Ho inviato il messaggio a tutti i %d utenti registrati!\n", conta);
			setHeader(&(new.hdr), OP_OK, "");
			sendHeader(connfd,&(new.hdr));
			
			free(msg.data.buf);	
			free(nome);	
			free(receiver);
			printf("FINE OPEZAZIONE POSTTEXTALL_OP\n");
		return 1;
		}
	}
	
    case POSTFILE_OP:{	
		printf("INIZIO OPEZAZIONE POSTFILE_OP\n");
		int lunghezza=msg.data.hdr.len;

		int i=chiave(hash, (void*)receiver)%10;

		pthread_mutex_lock(&mutexhash[i]);
		utente *userID = (utente *)icl_hash_find(hash, (void *)receiver);
		pthread_mutex_unlock(&mutexhash[i]);
		
		message_t new;
		memset(&new, 0, sizeof(message_t));
		
		//controllo se l'utente è registrato
		if(userID==NULL){
		   printf("ERROR: richiesta invio messaggio ad utente non registrato!\n");
		   setHeader(&(new.hdr), OP_NICK_UNKNOWN, "");
		   sendHeader(connfd,&(new.hdr));
		  
		   free(msg.data.buf);
		   free(nome);	
		   free(receiver);
		   chattyStats.nerrors++;	
		   printf("FINE OPEZAZIONE POSTFILE_OP\n");	
		 return -1;
		 }

	printf("Ho ricevuto da %s il file %s, lo memorizzo in %s\n", nome, msg.data.buf, DirName);
	
	/* Leggo il file e lo memorizzo in file_data */
	message_data_t file_data;
    readData(connfd, &file_data);
    lunghezza=file_data.hdr.len;
      		
     //se la lunghezza è maggiore di quella consentita 	
	 if(lunghezza>(1024*MaxFileSize)){			
		message_t mess;
		memset(&mess, 0, sizeof(message_t));
		setHeader(&(mess.hdr), OP_MSG_TOOLONG, "");
		sendHeader(connfd, &(mess.hdr));
   	  
		free(msg.data.buf);	
		free(nome);		
		free(receiver);
		chattyStats.nerrors++;
		printf("FINE OPEZAZIONE POSTFILE_OP\n");	
	  return -1;
	 }
	
	/* alloco lo spazio per il patch del file */
	int path =strlen(DirName) + strlen(msg.data.buf) + 2;
    char *nomefile = malloc(path * sizeof(char *));
      memset(nomefile, 0, path);
      strcat(nomefile, DirName);
      strcat(nomefile, "/");
        
      /* MODIFICARE DA QUI */
      char *terminatore = strrchr(msg.data.buf, '/');
	  int lu=0;

      if(terminatore!=NULL){
		 terminatore++;
         lu=msg.data.hdr.len-(terminatore-msg.data.buf);
         strncat(nomefile, terminatore, lu);
       } 
        
      else strncat(nomefile, msg.data.buf, msg.data.hdr.len);
          
       /* FINO A QUI */
          
	struct stat st={0};

		/* se non è stata già creata la directory la creo */
		if(stat(DirName, &st)==-1){					//se la directory non esiste la creo
			if(mkdir(DirName, 0700)==-1){				//gestione errore creazione dir
				perror("errore creazione dir");
				return -1;
			}
		}

		/* apre in scrittura il file ricevuto */
		FILE *fd=fopen(nomefile, "w");		//apro in scrittura il file 

		if( fd == NULL){     
			perror("errore apertura file!");
			fclose(fd);
			return -1;
		} 
		
		printf("File Aperto con successo!\n");
		
		  if (fwrite(file_data.buf, sizeof(char),file_data.hdr.len, fd)!=file_data.hdr.len){
			perror("open");
			fprintf(stderr, "ERRORE: scrivendo il file %s\n", nomefile);
			fclose(fd);
			chattyStats.nerrors++;	
			
			free(msg.data.buf);		
			free(nome);	
			free(receiver);
			free(file_data.buf);  
			free(nomefile);  
			printf("FINE OPEZAZIONE POSTFILE_OP\n");	    						 
           return -1;
		   }

        fclose(fd);

		printf("file copiato con successo!\n");
		
		//controllo se l'utente è online
				pthread_mutex_lock(&mutexC);	
				int fdrecever=Find(connessi,userID->nome);
				pthread_mutex_unlock(&mutexC);
				
			 if(fdrecever>0){
				printf("Utente già connesso, invio il file\n");
				message_t mess;
				memset(&mess, 0, sizeof(message_t));
					
				// Invio subito il file e azzero il buffer
				setHeader(&(mess.hdr), FILE_MESSAGE, nome);
				setData(&mess.data,"",(char *)msg.data.buf,msg.data.hdr.len); 
				chattyStats.nfiledelivered++;			
				sendRequest(fdrecever, &mess);
			}
			
			else{
				pthread_mutex_t mutexuser=userID->mutex;	
				pthread_mutex_lock(&mutexuser);			
				userID->testa=Inseriscimessaggio(userID,userID->testa,msg.data.buf, msg.hdr.sender, FILE_MESSAGE, MaxHistMsgs);
				chattyStats.nfilenotdelivered++;				
				if(userID->nmessage<MaxHistMsgs)  userID->nmessage++;
				pthread_mutex_unlock(&mutexuser);
			 }
		// Invio l'esito
		setHeader(&(msg.hdr), OP_OK, "");
        sendHeader(connfd, &(msg.hdr));
        
        free(file_data.buf);  
        free(nomefile); 
		free(msg.data.buf);
		free(nome);	
		free(receiver);	
	   printf("FINE OPEZAZIONE POSTFILE_OP\n");			
     return 1;	
	}
	
    case UNREGISTER_OP: {		
		printf("INIZIO OPEZAZIONE UNREGISTER_OP\n");		
		
		//cerco l'utente nella tabella hash
		int i=chiave(hash, (void*)nome)%10;
	
	pthread_mutex_lock(&mutexhash[i]);
		utente *userID = (utente*)icl_hash_find(hash, (void *)nome);
		pthread_mutex_unlock(&mutexhash[i]);
	
		if(userID==NULL){
			 printf("ERRORE NICKNAME NON ESISTENTE\n");
			 setHeader(&msg.hdr, OP_NICK_UNKNOWN, "");
			 sendHeader(connfd,&msg.hdr);
			 free(msg.data.buf);
			 free(nome);	
			 free(receiver);
			 chattyStats.nerrors++;							 
		   	 printf("FINE OPEZAZIONE UNREGISTER_OP\n");	
		   return -1; 
		 }
		 
		else{
			printf("Cancello la history dei messaggi\n");
			pthread_mutex_t mutexuser=userID->mutex;	
			pthread_mutex_lock(&mutexuser);
			if(userID->testa!=NULL) cancellamessaggi(userID->testa);
			pthread_mutex_unlock(&mutexuser);
			
			printf("Elimino la struttura utente\n");	
	
			pthread_mutex_lock(&mutexhash[i]);
			icl_hash_delete(hash,nome,NULL,NULL);		//ATTENZIONE:ho modificato il file icl_hash.h perchè in delete mi incrementava anzicchè decrementare		
			pthread_mutex_unlock(&mutexhash[i]);
			
			setHeader(&msg.hdr, OP_OK, "");
			sendHeader(connfd,&msg.hdr);
			chattyStats.nusers--;					//decremento numero di utenti registrati	 
		}
				
		 free(msg.data.buf);
	     free(nome);		
		 free(receiver);
		 printf("FINE OPEZAZIONE UNREGISTER_OP\n");	
	  return 1;
	}
	
	case GETFILE_OP:  {  
		printf("INIZIO OPEZAZIONE GETFILE_OP\n");	
	
	int pathlen =strlen(DirName) + strlen(msg.data.buf) + 2;
	message_t new;
	memset(&new, 0, sizeof(message_t));
      
    char *filename = malloc(pathlen * sizeof(char *));
    memset(filename, 0, pathlen);
    strcat(filename, DirName);
    strcat(filename, "/");
    strcat(filename, msg.data.buf);      
      
    int fd = open(filename, O_RDONLY);
  
      // Se c'è stato un errore
     if (fd < 0) {
	    printf("ERRORE: aprendo il file %s\n", filename);
        setHeader(&new.hdr, OP_NO_SUCH_FILE, "");
        sendHeader(connfd, &new.hdr);
		close(fd);
        
        free(filename);
        free(msg.data.buf);
		free(nome);		
		free(receiver);
		chattyStats.nerrors++; 
		printf("FINE OPEZAZIONE GETFILE_OP\n");	
       return -1;
	}
		
	// controllo che il file esista
	struct stat st;
	if (stat(filename, &st)==-1) {
		fprintf(stderr, "ERRORE: nella stat del file %s\n", filename);
		setHeader(&(new.hdr), OP_NO_SUCH_FILE, "");
        sendHeader(connfd, &(new.hdr));
  
        free(filename);
        free(msg.data.buf);
		free(nome);		
		free(receiver);
		chattyStats.nerrors++;	
		printf("FINE OPEZAZIONE GETFILE_OP\n");		
	   return -1;
	 }
	
	//controlla il tipo di file    
	if (!S_ISREG(st.st_mode)) {
		fprintf(stderr, "ERRORE: il file %s non e' un file regolare\n", filename);
		setHeader(&(new.hdr), OP_NO_SUCH_FILE, "");
        sendHeader(connfd, &(new.hdr));
         
        free(msg.data.buf);
		free(nome);		
		free(receiver);
        chattyStats.nerrors++;	
        printf("FINE OPEZAZIONE GETFILE_OP\n");							 
	   return -1;
	}
		
	char *mappedfile;
	// mappiamo il file da spedire in memoria
	mappedfile = mmap(NULL, st.st_size, PROT_READ,MAP_PRIVATE, fd, 0);
	    
	if (mappedfile == MAP_FAILED) {
		perror("mmap");
		fprintf(stderr, "ERRORE: mappando il file %s in memoria\n", filename);
		close(fd);
		setHeader(&(new.hdr), OP_NO_SUCH_FILE, "");
        sendHeader(connfd, &(new.hdr));
     
        free(msg.data.buf);
		free(nome);		
		free(receiver);
		chattyStats.nerrors++;	
		printf("FINE OPEZAZIONE GETFILE_OP\n");			 
	   return -1;
	}
		
		close(fd);
		setHeader(&(new.hdr), OP_OK, "");
		setData(&new.data, "", mappedfile, st.st_size); 	// invio il nome del file
		sendHeader(connfd, &(new.hdr)); sendData(connfd, &(new.data));
		chattyStats.nfiledelivered++;							
		printf("File inviato con successo!\n");
	
		free(filename);
		free(msg.data.buf);
		free(nome);		
		free(receiver);
		printf("FINE OPEZAZIONE GETFILE_OP\n");		
	  return 1;
	}
	
    case GETPREVMSGS_OP: {		
		printf("INIZIO OPEZAZIONE GETPREVMSGS_OP\n");		
		int c=chiave(hash, (void*)nome)%10;
		pthread_mutex_lock(&mutexhash[c]);
		utente *userID = (utente*)icl_hash_find(hash, (void*)nome);
		pthread_mutex_unlock(&mutexhash[c]);
		
		//controllo che l'utente esista
		if(userID==NULL){
			setHeader(&msg.hdr, OP_NICK_UNKNOWN, "");	
			sendHeader(connfd,&msg.hdr);
			printf("ERROR: Nickname non esistente\n");
			
			free(msg.data.buf);
			free(nome);		
			free(receiver);
			chattyStats.nerrors++;		
			printf("FINE OPEZAZIONE GETPREVMSGS_OP\n");		
		  return -1;
		}
		
		printf("Utente già registrato, mando la lista dei messaggi:\n");	
		
		history *testa,*curr;
		size_t dim=0;
		int i=0;
		message_t mess;
			
		pthread_mutex_t mutexuser=userID->mutex;	
		pthread_mutex_lock(&mutexuser);
		testa=userID->testa;
		//salvo in dim il numero dei messaggi da inviare
		dim=contamessaggidainviare(testa,userID->inviato);
		pthread_mutex_unlock(&mutexuser);
		
		memset(&msg, 0, sizeof(message_t));
		setHeader(&msg.hdr, OP_OK, "");
		//mando il numero dei messaggi da inviare 
		setData(&msg.data, "", (char *)&dim, sizeof(size_t));
		sendRequest(connfd,&msg);
		
		pthread_mutex_lock(&mutexuser);
		curr=testa;	
		for(i=0;i<dim;i++){
			memset(&mess, 0, sizeof(message_t));
			//inizializzo il messaggio da inviare con le informazioni
			setHeader(&mess.hdr, curr->op, curr->sender);
			setData(&mess.data,"" , curr->message, strlen(curr->message)+1);
			
			// Invio il messaggio e sposto il puntatore
			sendRequest(connfd,&mess);
			userID->inviato=testa;
			curr=curr->next;
		}
		curr=NULL;
		pthread_mutex_unlock(&mutexuser); 
		chattyStats.ndelivered=chattyStats.ndelivered+dim;				
		chattyStats.nnotdelivered=chattyStats.nnotdelivered-dim;	
		
		free(curr);
		free(nome);	
		free(receiver);
		printf("FINE OPEZAZIONE GETPREVMSGS_OP\n");	
	  return 1;
	}
	 
    case USRLIST_OP: {		
	 printf("INIZIO OPEZAZIONE USRLIST_OP\n");	
	 char *c=NULL;
	 int lunghezza=0;
	 pthread_mutex_lock(&mutexC);
	 
	 if(connessi!=NULL){ 
		converti(connessi,&c);
		lunghezza=conta(connessi)*(MAX_NAME_LENGTH+1);
		pthread_mutex_unlock(&mutexC);
		printf("Invio la lista dei messaggi:\n");
		
		setHeader(&msg.hdr, OP_OK, "");
		setData(&msg.data, "", c, lunghezza);
		sendRequest(connfd,&msg);
		
		free(msg.data.buf);
		free(nome);	
		free(receiver);	
		printf("FINE OPEZAZIONE USRLIST_OP\n");	
	  return 1;
	 }
		
	 pthread_mutex_unlock(&mutexC);
	 setHeader(&msg.hdr, OP_FAIL, "");
	 sendHeader(connfd,&msg.hdr);		
	 chattyStats.nerrors++;			 
	 printf("ERROR:Nessun utente online! \n");
	
	 free(msg.data.buf);
	 free(nome);	
	 free(receiver);
	 printf("FINE OPEZAZIONE USRLIST_OP\n");	
	return -1;
	}
	
    case CREATEGROUP_OP: 	//lo ignoro
    case ADDGROUP_OP:		//lo ignoro
    case DELGROUP_OP: 		//lo ignoro
    default: {
			fprintf(stderr, "ERRORE: messaggio non valido\n");
			free(msg.data.buf);	
			free(nome);		//controlla ordine
			free(receiver);
		return -1;
    }
    }
	
}


void *worker(void *notuse){

TipoNodoLista *curr=NULL;
message_t msg;	
memset(&msg, 0, sizeof(message_t));
int nread; 	
int connfd;
int esito=0;

	while(!fermo){
		pthread_mutex_lock(&mutex);
		while(testa==NULL && !fermo) pthread_cond_wait(&cond, &mutex);	
		
		if(fermo==1){
			pthread_mutex_unlock(&mutex);
			 break;
		 }
			curr=testa;
			testa=testa->next;
			connfd= curr->fd;
			free(curr);	
		pthread_mutex_unlock(&mutex);										
				nread=readHeader(connfd,&(msg.hdr));
					
					//se non c'erano caratteri	
					if(nread<=0){			
						//azzero il corrispontende bit	
						pthread_mutex_lock(&mutexfd_set);	
						FD_CLR(connfd, &set);	
						pthread_mutex_unlock(&mutexfd_set);
						
						//disconnetto il client
						pthread_mutex_lock(&mutexC);
						char *nome=Findfd(connessi,connfd);
						printf("Disconnessione: %s\n", nome);
						if(nome!=NULL){
							  connessi=delete(connessi,nome);
							  chattyStats.nonline--;			 
						}
						pthread_mutex_unlock(&mutexC);
						
						//chiudo il descrittore
						close(connfd);		
					}
					
					else{				
						esito=esegui_comando(connfd,msg);		
						//op a buon fine
						if(esito==1){
								printf("Operazione %d eseguita correttamente!\n", msg.hdr.op);
								pthread_mutex_lock(&mutexfd_set);
								FD_SET(connfd, &set);
								pthread_mutex_unlock(&mutexfd_set);
							}
						else {
								printf("Operazione %d FALLITA!\n", msg.hdr.op);
								pthread_mutex_lock(&mutexC);
							 char *nome=Findfd(connessi,connfd);
								printf("Disconnessione: %s\n", nome);
								if(nome!=NULL){
								  connessi=delete(connessi,nome);
								  chattyStats.nonline--;				 
								}
								pthread_mutex_unlock(&mutexC);
						}	      
					
					}
	}

return 0;
}

int main(int argc, char *argv[]) {
	
//a fine programma mi cancella il path del socket che ho creato
cleanup();
atexit(cleanup); 	
  
	if (argc <= 2 || strcmp("-f", argv[1])!=0) {
		usage(argv[0]);
		return -1;
    }
    
    //effettuo il parsing
     OpenConf(argv[2]);  
     
int notused;
int listenfd,des;
int fd_num=0, connfd=0,i,fd=0;							
	
	//creo i thread 
	pthread_t *tid=malloc(ThreadsInPool*sizeof(pthread_t));		
	
struct timeval tv;

	//creo tabella hash
	hash = icl_hash_create(1024, ulong_hash_function, ulong_key_compare);		

//creo e setto le strutture per la gestione dei segnali
struct sigaction s; 
struct sigaction ign;
struct sigaction stats;
  
    memset( &s, 0, sizeof(s));
    s.sa_handler=gestore;
    
    memset( &ign, 0, sizeof(ign));
    ign.sa_handler=SIG_IGN;
	
	memset( &stats, 0, sizeof(stats));
    stats.sa_handler=stampastatistiche;
    
    //assegno a SIGINT,SIGQUIT,SIGTERM la gestione con gestore
    //ignoro SIGPIPE e stampo le statistiche se ricevo SIGUSR1
    SYSCALL(notused, sigaction(SIGINT, &s, NULL), "sigaction"); 
    SYSCALL(notused, sigaction(SIGQUIT, &s, NULL), "sigaction"); 
    SYSCALL(notused, sigaction(SIGTERM, &s, NULL), "sigaction");    
	SYSCALL(notused, sigaction(SIGPIPE, &ign, NULL), "sigaction");
	SYSCALL(notused, sigaction(SIGUSR1, &stats, NULL),"sigaction");
	
	//creo i thread worker 
	for(i=0;i<ThreadsInPool;i++){
		des=i;
		if (pthread_create(&tid[i], NULL, worker, &des) != 0) {
			fprintf(stderr, "pthread_create failed \n");
			return (EXIT_FAILURE);
		}
    }

	struct sockaddr_un serv_addr;					
	SYSCALL(listenfd, socket(AF_UNIX, SOCK_STREAM, 0), "socket");	
	
   // setto l'indirizzo
    memset(&serv_addr, '0', sizeof(serv_addr));				 
    serv_addr.sun_family = AF_UNIX;    
    strncpy(serv_addr.sun_path, UnixPath, strlen(UnixPath)+1);
    
    // assegno l'indirizzo al socket 
    SYSCALL(notused, bind(listenfd, (struct sockaddr*)&serv_addr,sizeof(serv_addr)), "bind");		 
																						
    // definisco un n. massimo di connessioni pendenti e mi metto in ascolto
    SYSCALL(notused, listen(listenfd, MaxConnections), "listen");	

    if(listenfd > fd_num) fd_num= listenfd;
	FD_ZERO(&set);							
	FD_SET(listenfd, &set);				

		while(1){
			//se ho ricevuto il segnale per fermarmi, esco
			if(fermo==1) break;
			tv.tv_sec=0;
			tv.tv_usec=1000;
			rdset=set;				
			if(select(fd_num+1, &rdset, NULL, NULL, &tv)>=0){
			  for(fd=0; fd<=fd_num;fd++)		//controllo ogni descrittore
			
			  //se il bit corrispondente è=1
			    if(FD_ISSET(fd, &rdset)){		
				  if(fd==listenfd){		
					//prima di accettare controllo se supero il massimo delle connessioni	
					if( chattyStats.nonline+1 > MaxConnections ) continue;
  
					SYSCALL(connfd, accept(listenfd, (struct sockaddr*)NULL ,NULL), "accept");	  
					
					//setto il bit 
					pthread_mutex_lock(&mutexfd_set);
					FD_SET(connfd, &set);				
					pthread_mutex_unlock(&mutexfd_set);
					
					//aggiorno la lunghezza 
						if(connfd > fd_num) fd_num= connfd;						
					}
					
					else{	 							
						connfd=fd;
					}
					
					//cancello il bit dalla maschera
					pthread_mutex_lock(&mutexfd_set);
					FD_CLR(connfd, &set);
					pthread_mutex_unlock(&mutexfd_set);	
					
					//metto in lista il descrittore del client che ha fatto la richiesta
					pthread_mutex_lock(&mutex);
					testa=Inserisci(testa,connfd);
					pthread_cond_signal(&cond);	
					pthread_mutex_unlock(&mutex);	
				}	

			}
		}
		
		//aspetto che terminano i thread
		for(i=0;i<ThreadsInPool;i++){
		if (pthread_join(tid[i], NULL) != 0) {
			fprintf(stderr, "pthread_join failed (Producer)\n");
			return (EXIT_FAILURE);
		}
    }
    
    free(tid);
   
   //cancello la struct utente e tutti i messaggi salvati
    for(i=0;i<1024;i++){
			if(hash->buckets[i]!=NULL){
				icl_entry_t *curr=hash->buckets[i];
				utente *userID=curr->data;
				if(userID->testa!=NULL) cancellamessaggi(userID->testa);
				free(userID);
			}
		}

//distruggo tutta la tabella hash
icl_hash_destroy(hash, NULL, NULL);					

//libero la lista
if(testa) destroyList(testa);						
if(connessi) destroyList(connessi);

//chiude il socket
close(listenfd);					

return 0;
}
