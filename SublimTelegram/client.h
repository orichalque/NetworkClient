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

/**
 * @function stop()
 * @brief Fonction d'arrêt du client, ferme le socket et les threads
 * @return void
 */
void stop();

/**
 * @function envoi_message
 * @param arg, message ecrit par le client
 * @return void
 * @brief Fonction threadée d'envoi de message, se ferme une fois le message envoyé
 */
void *envoi_message(void* arg);

/**
 * @function reception_message
 * @param arg le message reçu par le client
 * @return void
 * @brief Boucle de reception, traitement et affichage du message.
 * Thread unique contenant une boucle infinie. s'arrête a la deconnexion du client
 */
void *reception_message(void* arg);

/**
 * @function main 
 * @param argc
 * @param argv
 * @return 1 en cas d'echec, 0 sinon
 * @brief fonction main, se connecte au serveur et recupere les entrées client
 */
int main(int argc, char **argv);

#ifdef	__cplusplus
}
#endif

#endif	/* CLIENT_H */

