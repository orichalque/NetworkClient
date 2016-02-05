/*----------------------------------------------
Serveur à lancer avant le client
------------------------------------------------*/
#include <stdlib.h>
#include <stdio.h>
#include <linux/types.h> 	/* pour les sockets */
#include <sys/socket.h>
#include <netdb.h> 		/* pour hostent, servent */
#include <string.h> 		/* pour bcopy, ... */  
#include <pthread.h> //threads
#include <unistd.h>
#include <time.h> 
#define TAILLE_MAX_NOM 256

typedef struct sockaddr sockaddr;
typedef struct sockaddr_in sockaddr_in;
typedef struct hostent hostent;
typedef struct servent servent;

typedef struct{
	int sz;
	int socks[];
} users;

users user;

int addUser(users *u, int* sock) {
	if (! u -> sz){
		printf("Liste vide \n");
		u -> sz ++;
		u -> socks[0] = *sock;
	} else {
		int exists = 0;
		//verification de la presence d'une sock avant son ajout
		if (u -> sz >= 10){
			printf("trop de monde \n");
			return exists;
		}		
		int i;
		for (i = 0; i < u -> sz; ++i){
			if (&u -> socks[i] == sock){
				exists = 1; //sock deja present
				printf("deja present \n");
			} 
		}		
		if (!exists){
			printf("ajoute \n");
			printf("%d", *sock);
			u -> socks[u -> sz] = *sock;
			u -> sz ++;			
		}
	}
}


/*------------------------------------------------------*/
void *thread_1(void *arg){
	printf("yolo \n");
	(void) arg;
	pthread_exit(NULL);
}

void *renvoi_message(void *arg){
	char date[11];
	char buffer[256];
	char message[512];
    int longueur;
    int * sock = arg;
    addUser(&user, sock);
    time_t seconds;
    struct tm instant;
    
    time(&seconds);
    instant = *localtime(&seconds);
    
	snprintf(date, sizeof date, "[%d:%d:%d]", instant.tm_hour, instant.tm_min, instant.tm_sec);
	
	//boucle de communication != boucle d'acceptation de connexion
    while(1){
		if ((longueur = read(*sock, buffer, sizeof(buffer))) <= 0){
			printf("%i\n",*sock);
			pthread_exit(NULL);
		}
		
		buffer[longueur] ='\0';
		snprintf(message, sizeof message, "%s %s \n", date, buffer);
		 printf("%s \n", message);
		write(*sock,message,strlen(message)+1);
    }
	pthread_exit(NULL);
}

/*------------------------------------------------------*/

/*------------------------------------------------------*/
main(int argc, char **argv) {
  	
    int 		socket_descriptor, 		/* descripteur de socket */
			nouv_socket_descriptor, 	/* [nouveau] descripteur de socket */
			longueur_adresse_courante; 	/* longueur d'adresse courante d'un client */
    sockaddr_in 	adresse_locale, 		/* structure d'adresse locale*/
			adresse_client_courant; 	/* adresse client courant */
    hostent*		ptr_hote; 			/* les infos recuperees sur la machine hote */
    servent*		ptr_service; 			/* les infos recuperees sur le service de la machine */
    char 		machine[TAILLE_MAX_NOM+1]; 	/* nom de la machine locale */
    
    gethostname(machine,TAILLE_MAX_NOM);		/* recuperation du nom de la machine */
    user.sz = 0; //initialisation des users
    
    /* recuperation de la structure d'adresse en utilisant le nom */
    if ((ptr_hote = gethostbyname(machine)) == NULL) {
		perror("erreur : impossible de trouver le serveur a partir de son nom.");
		exit(1);
    }
    
    /* initialisation de la structure adresse_locale avec les infos recuperees */			
    
    /* copie de ptr_hote vers adresse_locale */
    bcopy((char*)ptr_hote->h_addr, (char*)&adresse_locale.sin_addr, ptr_hote->h_length);
    adresse_locale.sin_family		= ptr_hote->h_addrtype; 	/* ou AF_INET */
    adresse_locale.sin_addr.s_addr	= INADDR_ANY; 			/* ou AF_INET */

    adresse_locale.sin_port = htons(5000);
    /*-----------------------------------------------------------*/
    
    printf("numero de port pour la connexion au serveur : %d \n", 
		   ntohs(adresse_locale.sin_port) /*ntohs(ptr_service->s_port)*/);
    
    /* creation de la socket */
    if ((socket_descriptor = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("erreur : impossible de creer la socket de connexion avec le client.");
		exit(1);
    }

    /* association du socket socket_descriptor à la structure d'adresse adresse_locale */
    if ((bind(socket_descriptor, (sockaddr*)(&adresse_locale), sizeof(adresse_locale))) < 0) {
		perror("erreur : impossible de lier la socket a l'adresse de connexion.");
		exit(1);
    }
    
    

    /* attente des connexions et traitement des donnees recues */
    for(;;) {
    	/* initialisation de la file d'ecoute */
	    listen(socket_descriptor,5);
		longueur_adresse_courante = sizeof(adresse_client_courant);
		
		/* adresse_client_courant sera renseigné par accept via les infos du connect */
		if ((nouv_socket_descriptor = 
			accept(socket_descriptor, 
			       (sockaddr*)(&adresse_client_courant),
			       &longueur_adresse_courante))
			 < 0) {
			perror("erreur : impossible d'accepter la connexion avec le client.");
			exit(1);
		}
		
		pthread_t t1;
    	if (pthread_create(&t1, NULL, renvoi_message, &nouv_socket_descriptor) == -1){
    		perror("pthread_create");    	
    	}
		//close(nouv_socket_descriptor);

    }
}

