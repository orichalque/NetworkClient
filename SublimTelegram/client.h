/* 
 * File:   client.h
 * Author: E104607D
 *
 * Created on 29 janvier 2016, 11:08
 */

#ifndef CLIENT_H
#define	CLIENT_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdio.h>
#include <linux/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
    
//Structures sockets
typedef struct sockaddr 	sockaddr;
typedef struct sockaddr_in 	sockaddr_in;
typedef struct hostent 		hostent;
typedef struct servent 		servent;

void stop();
void *envoi_message(void* arg);
void *reception_message(void* arg);
int main(int argc, char **argv);

#ifdef	__cplusplus
}
#endif

#endif	/* CLIENT_H */

