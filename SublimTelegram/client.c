/*-----------------------------------------------------------
Client a lancer apres le serveur avec la commande :
client <adresse-serveur> <pseudonyme>
//TODO Modifier la trame pour indiquer la room ou l'on se connecte
------------------------------------------------------------*/
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

sockaddr_in adresse_locale;
hostent *ptr_host;
servent *ptr_service;
int socket_descriptor;
char *host;
char *pseudo;
char *room; //limite 16 char
char pseudoWithComa[14];
char pseudiWithComaAndRoom[30];
//pseudo:room

//code thread envoi message
void *envoi_message(void* arg){
    char buffer[256];
    char mesg[512];
    int longueur;
    strcpy(mesg, pseudoWithComa);    
    strcat(mesg, arg);
    if (strcmp(arg, "0") == 0){
    	//message de deconnexion venant du server (Impossible de rejoindre un salon)
    	exit(1);    
    }
    
    // envoi du message vers le serveur
    if ((write(socket_descriptor, mesg, strlen(mesg))) < 0) {
		perror("erreur : impossible d'ecrire le message destine au serveur.");
		exit(1);
    }
    // lecture de la reponse en provenance du serveur
    while((longueur = read(socket_descriptor, buffer, sizeof(buffer))) > 0) {
		write(1,buffer,longueur);
    }
    pthread_exit(NULL);
}

// Fonction d'arret
void stop(){
    //message automatique pour prevenir de la deconnexion
    char mesg[512];
    int longueur;
    strcpy(mesg,strcat(pseudo," est deconnecte."));

    // envoi du message vers le serveur
    if ((write(socket_descriptor, mesg, strlen(mesg))) < 0) {
		perror("erreur : impossible d'ecrire le message destine au serveur.");
		exit(1);
    }
    close(socket_descriptor);
    
    printf("connexion avec le serveur fermee, fin du programme.\n");
    exit(0);
}

int main(int argc, char **argv) {	
    char mesg[512];
    if (argc != 3) { // verification du nombre d'argument
		perror("Pas assez d'arguments : ./client <adresse-serveur> <pseudonyme> ");
		exit(1);
    }
    if (argc == 4) {
    	salon = arg[3];
    }

    host = argv[1];
    pseudo = argv[2];
    
    printf("connecte au serveur  : %s \n", host);
    printf("sous le pseudo   : %s \n", pseudo);
    strcpy(pseudoWithComa, pseudo);
    strcat(pseudoWithComa, ": ");
    
    if ((ptr_host = gethostbyname(host)) == NULL) {
		perror("erreur : impossible de trouver le serveur a partir de son adresse.");
		exit(1);
    }
    
    // copie caractere par caractere des infos de ptr_host vers adresse_locale
    bcopy((char*)ptr_host->h_addr, (char*)&adresse_locale.sin_addr, ptr_host->h_length);
    adresse_locale.sin_family = AF_INET;
    //utilisation du port suivant
    adresse_locale.sin_port = htons(5000);
    printf("numero de port pour la connexion au serveur : %d \n", ntohs(adresse_locale.sin_port));

    //creation de la socket
    if ((socket_descriptor = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("erreur : impossible de creer la socket de connexion avec le serveur.");
		exit(1);
    }
    // tentative de connexion avec le serveur
    if ((connect(socket_descriptor, (sockaddr*)(&adresse_locale), sizeof(adresse_locale))) < 0) {
		perror("erreur : impossible de se connecter au serveur.");
		exit(1);
    }

    printf("connexion etablie avec le serveur. \n");

    //execution de stop() si l'utilisateur presse CTRL+C
    signal(SIGINT,stop);

    while (1){
		fgets(mesg, 489, stdin); //pas plus de 489 car + car fin chaine
		pthread_t t1;
		if (pthread_create(&t1, NULL, (void*(*)(void*))envoi_message, &mesg) == -1){
				perror("Impossible de creer le thread d'envoi de message");
		}
    }
      
    exit(0);
    
}
