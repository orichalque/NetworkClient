/*-----------------------------------------------------------
Client a lancer apres le serveur avec la commande :
client <adresse-serveur> <pseudonyme>
------------------------------------------------------------*/
#include <stdlib.h>
#include <stdio.h>
#include <linux/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>

//TODO definir signification des messages systemes

//Structures sockets
typedef struct sockaddr 	sockaddr;
typedef struct sockaddr_in 	sockaddr_in;
typedef struct hostent 		hostent;
typedef struct servent 		servent;

sockaddr_in adresse_locale;
hostent *ptr_host;
servent *ptr_service;

pthread_t t1;
pthread_t t2;

int socket_descriptor;
char *host;
char *pseudo;
char *salon; //limite 16 char
char pseudoWithComa[14];
char pseudoWithComaAndRoom[26];

//code thread envoi message
void *envoi_message(void* arg){
    char buffer[256];
    char mesg[512];
    int longueur;

	strcpy(mesg, pseudoWithComaAndRoom);    
	strcat(mesg, arg);

    mesg[strlen(mesg)-1]='\0';
    // envoi du message vers le serveur
    if ((write(socket_descriptor, mesg, strlen(mesg))) < 0) {
		perror("erreur : impossible d'ecrire le message destine au serveur.");
		exit(1);
    }
    pthread_exit(NULL);
}

//code thread reception message
void *reception_message(void* arg){
    char buffer[256];
    int longueur;

    // lecture de la reponse en provenance du serveur
    while(1){
		while((longueur = read(socket_descriptor, buffer, sizeof(buffer))) > 0) {
			if (!strcmp(buffer, "0")){
				printf("Deconnection");
				exit(1);
			}
			write(1,buffer,longueur);
		}
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
    pthread_cancel(t1);
    pthread_cancel(t2);
    
    printf("connexion avec le serveur fermee, fin du programme.\n");
    exit(0);
}

int main(int argc, char **argv) {	
    char mesg[512];
    if (argc < 3) { // verification du nombre d'argument
		perror("Pas assez d'arguments : ./client <adresse-serveur> <pseudonyme> ");
		exit(1);
    }
    
    if (argc == 4) {
    	if (strlen(argv[3]) > 14) {
    		memcpy( salon, &argv[3][0], 14 );
    		//printf("salon tronqué: %s", salon);
    	} else {
    		salon = argv[3];
    		while (strlen(salon) < 14){
    			strcat(salon, " ");
    		}
    	}
    } else {
    	salon = "room commune  ";
    	int sizeSalon = strlen(salon);
    	printf("taille du salon: %d", sizeSalon);
    	
    }

    host = argv[1];
    pseudo = argv[2];
    
    printf("connecte au serveur  : %s \n", host);
    printf("sous le pseudo   : %s \n", pseudo);
    strcpy(pseudoWithComa, pseudo);
    strcat(pseudoWithComa, ": ");
    strcat(salon, pseudoWithComa);
    strcpy(pseudoWithComaAndRoom, salon);
    
    
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
    if (pthread_create(&t2, NULL, (void*(*)(void*))reception_message, NULL) == -1){
				perror("Impossible de creer le thread de reception de message");
	}
    while (1){
		fgets(mesg, 489, stdin); //pas plus de 489 car + car fin chaine
		if (pthread_create(&t1, NULL, (void*(*)(void*))envoi_message, &mesg) == -1){
				perror("Impossible de creer le thread d'envoi de message");
		}
    }
      
    exit(0);
    
}
