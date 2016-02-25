/* 
 * File:   server.h
 * Author: E104607D
 *
 * Created on 29 janvier 2016, 11:09
 */

#ifndef SERVER_H
#define	SERVER_H

#include <stdlib.h>
#include <stdio.h>
#include <linux/types.h> 	/* pour les sockets */
#include <sys/socket.h>
#include <netdb.h> 		/* pour hostent, servent */
#include <string.h> 		/* pour bcopy, ... */  
#include <pthread.h> //threads
#include <signal.h>
#include <unistd.h>
#include <time.h> 
#define TAILLE_MAX_NOM 256

typedef struct sockaddr sockaddr;
typedef struct sockaddr_in sockaddr_in;
typedef struct hostent hostent;
typedef struct servent servent;

//Room struct containing users and a name
typedef struct{
	char* name;
	int sz;
	int socks[];
} users;

//struct containing rooms of users
typedef struct{
	int sz;
	users room[];
} rooms;

//Dictionnary of 50 words of size 16chars
typedef struct{
	int sz;
	char words[50][16];
} dictionnary;

void stop();
int addUser(users *u, int* sock);
void *thread_1(void *arg);
void *renvoi_message(void *arg);
void readWords(dictionnary* d);
char* analyseMessage(char* message, dictionnary *d);

#ifdef	__cplusplus
}
#endif

#endif	/* SERVER_H */

