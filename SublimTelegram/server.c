/*----------------------------------------------
Serveur à lancer avant le client
------------------------------------------------*/
#include "server.h"

users user;
rooms room;
dictionnary dict;
pthread_mutex_t mutexUserFile;
sockaddr_in ips[50];

//Used to read the dictionnary and store it in an array
void readWords(dictionnary* d) {
	FILE *fp;
	fp = fopen("dictionnaire","r"); // read mode
	char * line = NULL;
	size_t len = 0;
	ssize_t read;
	int MAXLINES = 8; //nb de mots
	if (fp == NULL){
		printf("Cannot open the dictionnary file");
	} 
	d -> sz = 0;
	int i = 0;
	while (i <= MAXLINES && fgets(d -> words[i], sizeof(d -> words[0]), fp))
	{
		d -> words[i][strlen(d -> words[i])-1] = '\0';
		i = i + 1;
		d -> sz ++;
	}
	fclose(fp);
}

/* retourne 1 si l'utilisateur est nouveau, 0 si déja connu, -1 si erreur */
int updateUserFile(sockaddr_in adresse) {
	FILE* fichier = NULL;
	fichier = fopen("users.txt", "r"); // en lecture pour rechercher l'adresse du client
	if (fichier != NULL)
	{
		int present = 0;
		char line [128];
		char ip[9];
		char ipfile[9];
		while ( (fgets ( line, sizeof line, fichier ) != NULL) && !present )
      		{
	      		strncpy(ip, line,8); // on recupere l'adresse du fichier
	      		ip[8] = '\0';
	      		sprintf(ipfile, "%d", adresse.sin_addr.s_addr); // et celle du client
	      		ipfile[8] = '\0';
	      		if (strcmp(ipfile,ip)==0)
	      		{
	      			present = 1; //client deja repertorie
	      		}   	
      		}
		fclose(fichier);
		if (!present){
			//protection du fichier user.txt qui peut etre modifié en concurrence
			pthread_mutex_lock(&mutexUserFile);
			
			fichier = fopen("users.txt", "a");// en ecriture pour l'ajouter
			if (fichier != NULL)
			{
				fprintf(fichier, "%d:%d\n", adresse.sin_addr.s_addr, 0);
				fclose(fichier);
				//on relache la protection sur le fichier
				pthread_mutex_unlock(&mutexUserFile);
				return 1;
			}
			pthread_mutex_unlock(&mutexUserFile);
		}else{
			return 0;
		}
	}
	return -1;
}

/* retourne 1 si l'incrementation est effectuée, -1 sinon */
//TODO on a besoin de l'adresse ! mais on a que le socket_descriptor !
//     ? structure socket -> adresse ??????????
int incrementInsult(sockaddr_in adresse) {
	FILE* fichier = NULL;
	FILE* new = NULL;
	//protection du fichier user.txt qui peut etre modifié en concurrence
	pthread_mutex_lock(&mutexUserFile);
	fichier = fopen("users.txt", "r"); // en lecture
	new = fopen("new.txt", "w"); // en ecriture
	if ((fichier != NULL) && (new != NULL))
	{
		char * nbinsults;
		int present = 0;
		char line [128];
		char ip[9];
		char ipfile[9];
		while (fgets ( line, sizeof line, fichier ) != NULL)
      		{ // on copie tout le fichier user dans le nouveau en modifiant que la ligne de l'utilisateur voulu
	      		strncpy(ip, line,8);
	      		ip[8] = '\0';
	      		sprintf(ipfile, "%d", adresse.sin_addr.s_addr);
	      		ipfile[8] = '\0';
	      		if (strcmp(ipfile,ip)==0)
	      		{//c'est l'utilisateur vulgaire
				nbinsults = strtok(line,":");
				nbinsults = strtok (NULL, ":");
	      			fprintf(new, "%d:%ld\n", adresse.sin_addr.s_addr, strtol(nbinsults,NULL,10)+1);
	      		}else
			{// utilisateur normal
				fprintf(new, "%s", line);
			}   	
      		}
		fclose(fichier);
		fclose(new);
		remove("users.txt"); // on remplace l'ancien fichier par le nouveau
		rename("new.txt", "users.txt");
		pthread_mutex_unlock(&mutexUserFile);
		return 1;	
	}
	//on relache la protection sur le fichier
	pthread_mutex_unlock(&mutexUserFile);
	return -1;	
}

int addUser(users *u, int* sock) {
	if (! u -> sz){
		u -> sz ++;
		u -> socks[0] = *sock;
		return 1;
	} else {
		if (u -> sz >= 10){
			//Salle pleine
			return 0;
		}		
		
		//verification de la presence d'une sock avant son ajout
		int i;
		for (i = 0; i < u -> sz; ++i){
			if (&u -> socks[i] == sock){
				//sock deja present
				return 0;
			} 
		}		
		printf("%d", *sock);
		u -> socks[u -> sz] = *sock;
		u -> sz ++;					
		return 1;
	}
}

/*
 *@brief Ajout d'utilisateur dans une room. Si la room existe pas on la crée
 *
 */
int addUserInRoom(rooms *room, int* sock, char* roomName){
	int cpt;
	int added = 0;
	if (!room->sz){
		//aucune salle existe on la crée:
		room->room[0].name = roomName;
		room->sz ++;	
		//On ajoute l'utilisateur
		char* message = "Vous avez crée une salle.\n";
		write(*sock, message, strlen(message)+1);
		return addUser(&room->room[room->sz-1], sock);
	}
	
	for (cpt = 0; cpt < room -> sz; cpt++){
		if (strcmp(room->room[cpt].name, roomName) == 0){
			//La salle existe déjà
			//On ajoute l'utilisateur dedans			
			added = 1;
			char* message = "Vous avez rejoint une salle.\n";
			write(*sock, message, strlen(message)+1);	
			return addUser(&room->room[cpt], sock);
						
		}
	}
	
	if (!added){
		//La salle n'existe pas, on la crée:
		//TODO: Vérifier nombre de salle maximum		
		if (room->sz >= 10){
			char* message = "Le serveur est complet. Rejoignez une salle déjà existante.\n";
			write(*sock, message, strlen(message)+1);
			return 0;
			
		}
		room->room[room->sz].name = roomName;
		room->sz ++;	
		//On ajoute l'utilisateur
		printf("Salle crée et user ajouté\n");
		return addUser(&room->room[room->sz-1], sock);
	}
}

char* analyseMessage(char* message, dictionnary *d, int* sock) {
	char* messageBis;
	int i = 0;
	int j;
	char* ptr;
	for (i = 0; i < d -> sz; i++){
		if((ptr = strstr(message, d -> words[i])) != NULL ){
			incrementInsult(ips[*sock]);
			for (j = 0; j < strlen(d -> words[i]); ++j){
				*ptr = '*';
				ptr++;
			}
		}
	}
	return message;
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
    char message[490];
    int longueur;
    int * sock = arg;
	if( !addUserInRoom(&room, sock, "room1") ) {		
		write(*sock, "0", 1);
		//TODO
	}	
    
//    if (addUser(&user, sock) == 0) {
//	      printf("echec ajout");
//    } else {
    		
//    }
    time_t seconds;
    struct tm instant;

    //boucle de communication avec un client
    while(1){
		if ((longueur = read(*sock, buffer, sizeof(buffer))) <= 0){
			pthread_exit(NULL);
		}
		time(&seconds);
    		instant = *localtime(&seconds);
		snprintf(date, sizeof date, "[%d:%d:%d]", instant.tm_hour, instant.tm_min, instant.tm_sec);
		buffer[longueur] ='\0';
		snprintf(message, sizeof message, "%s %s \n", date, buffer);
		analyseMessage(message, &dict, sock);
		int i = 0;
		
		//ecriture dans la room
		for (i = 0; i < room.room[0].sz; ++i) {
			printf("%s", message);
			write(room.room[0].socks[i], message, strlen(message)+1);
		}
		
    }
	pthread_exit(NULL);
}

/*------------------------------------------------------*/
void stop() {
	int i = 0;
	for (i = 0; i < user.sz; ++i){
		close(user.socks[i]);
	}
	printf("Fermeture du serveur \n");
	exit(0);
}

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
    readWords(&dict);
    
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

	int enable = 1;
	if (setsockopt(socket_descriptor, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
		error("setsockopt(SO_REUSEADDR) failed");
	}

    /* association du socket socket_descriptor à la structure d'adresse adresse_locale */
    if ((bind(socket_descriptor, (sockaddr*)(&adresse_locale), sizeof(adresse_locale))) < 0) {
		perror("erreur : impossible de lier la socket a l'adresse de connexion.");
		exit(1);
    }
    
    signal(SIGINT,stop);
	
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
		updateUserFile(adresse_client_courant);
		//association socket -> ip
		ips[nouv_socket_descriptor] = adresse_client_courant;
    	if (pthread_create(&t1, NULL, renvoi_message, &nouv_socket_descriptor) == -1){
    		perror("pthread_create");    	
    	}
    	
    }
}

