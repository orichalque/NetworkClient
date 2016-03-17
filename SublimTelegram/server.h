/*
 * File:   server.h
 * Author: E104607D
 *
 * Created on 29 janvier 2016, 11:09
 */

#ifndef SERVER_H
#define SERVER_H

#include <linux/types.h> /* pour les sockets */
#include <netdb.h>       /* pour hostent, servent */
#include <pthread.h>     //threads
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> /* pour bcopy, ... */
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>
#define TAILLE_MAX_NOM 256

typedef struct sockaddr sockaddr;
typedef struct sockaddr_in sockaddr_in;
typedef struct hostent hostent;
typedef struct servent servent;

// Room struct containing users and a name
typedef struct {
  char *name;
  int sz;
  int socks[10];
} users;

// struct containing rooms of users
typedef struct {
  int sz;
  users room[10];
} rooms;

// Dictionnary of 50 words of size 16chars
typedef struct {
  int sz;
  char words[50][19];
} dictionnary;

typedef struct {
  char *roomName;
  char *msg;
} msgToRoomStruct;

/**
 * @function findRoomFromSocket
 * @param sock le socket a chercher
 * @return char* room, la room où se trouve le socket
 * @brief Parcourt chaque room à la recherche du socket, et renvoie la room qui
 * correspond
 */
char *findRoomFromSocket(int sock);

/**
 * @function removeRoom
 * @param roomName le nom de la room a supprimer
 * @return 1 si la suppression est reussie, 0 sinon
 * @brief Retire la room désirée, et décalle toutes les autres dans la structure
 */
int removeRoom(char *roomName);

/**
 * @function removeSocketFromRoom
 * @param sock le socket a supprimer
 * @param roomName la room ou se trouve le socket
 * @return 1 si suppression reussie, 0 sinon.
 * @brief Supprime le socket d'une room et decalle tous les autres
 */
int removeSocketFromRoom(int sock, char *roomName);

/**
 * @function readWords
 * @param d le dictionnaire a remplir
 * @return void
 * @brief Ouvre le fichier de mots et lit chaque ligne pour les stocker dans le
 * dictionnaire
 */
void readWords(dictionnary *d);

/**
 * @function incrementInsult
 * @param adresse l'ip du client
 * @return 1 si l'incrementation est effectuée, -1 sinon
 * @brief incremente le nombre d'insultes d'un client, et le kick si supérieur à
 * 15 a chaque mesage
 */
int incrementInsult(sockaddr_in adresse);

/**
 * @function updateUserFile
 * @param adresse l'ip du client
 * @return  retourne 1 si l'utilisateur est nouveau, 0 si déja connu, -1 si
 * erreur
 * @brief vérifie la présence d'un utilisateur dans le fichier client
 */
int updateUserFile(sockaddr_in adresse);

/**
 * @function addUser
 * @param u le salon
 * @param sock le socket
 * @return 1 si ajout reussi, 0 sinon
 * @brief Ajoute un utilisateur dans la room en parametre
 */
int addUser(users *u, int *sock);

/**
 * @function addUserInRoom
 * @param sock le socket
 * @param roomName le nom de la room
 * @return 1 si ajout reussi, 0 sinon
 * @brief Crée la room, ou la retrouve, puis appelle addUser pour ajouter
 * l'utilisateur dedans
 */
int addUserInRoom(int *sock, char *roomName);

/**
 * @function afficherRooms
 * @return void
 * @brief affiche toutes les rooms et leur socket, coté serveur.
 */
void afficherRooms();

/**
 * @function analyseMEssage
 * @param message le message a analyser
 * @param d le dictionnaire de mots
 * @param sock le socket du message envoyé
 * @return le message
 * @brief analyse le message, et incrémente le nombre d'insulte s'il y en a une
 */
char *analyseMessage(char *message, dictionnary *d, int *sock);

/**
 * @function getServerResponse
 * @param commandLine la commande utilisateur entrée par le client
 * @return la reponse du serveur qui correspond
 * @brief analyse la commande du client pour agir en fonction
 */
char *getServerResponse(char *commandLine);

/**
 * @function renvoi_message
 * @param arg le message envoyé par le client
 * @return void
 * @brief Boucle de reception et envoi du message a la room
 */
void *renvoi_message(void *arg);

/**
 * @function sendMessageToRoom
 * @param rMsg le message a envoyer
 * @return void
 * @brief Fonction threadee d'envoi de message a tous les sockets d'une room
 */
void *sendMessageToRoom(void *rMsg);

/**
 * @function stop
 * @return void
 * @brief Ferme le server après avoir fermé toutes les room
 */
void stop();

#ifdef __cplusplus
}
#endif

#endif /* SERVER_H */
