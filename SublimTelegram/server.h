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
	int socks[10];
} users;

//struct containing rooms of users
typedef struct{
	int sz;
	users room[10];
} rooms;

//Dictionnary of 50 words of size 16chars
typedef struct{
	int sz;
	char words[50][19];
} dictionnary;


typedef struct {
    char* roomName;
    char* msg;
}msgToRoomStruct ;

char *findRoomFromSocket(int sock);
int removeRoom(char* roomName);
int removeSocketFromRoom(int sock, char* roomName);

void readWords(dictionnary* d) ;
int incrementInsult(sockaddr_in adresse);

int updateUserFile(sockaddr_in adresse);
int addUser(users *u, int* sock);
int addUserInRoom(int* sock, char* roomName);
void afficherRooms();

char *analyseMessage(char* message, dictionnary *d, int* sock);
char *getServerResponse(char* commandLine);
void *renvoi_message(void *arg);
void *sendMessageToRoom(void* rMsg);

void stop();
#ifdef	__cplusplus
}
#endif

#endif	/* SERVER_H */

