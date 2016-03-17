/*----------------------------------------------
Serveur à lancer avant le client
------------------------------------------------*/
#include "server.h"
//TODO Shifting des sockets lors d'une deconnection
rooms room;
dictionnary dict;
int currentSock;
pthread_mutex_t mutexUserFile;
pthread_mutex_t mutexRoom;
sockaddr_in ips[104]; //100 sockets, puisqu'on commence au socket 4
pthread_t t1;

/*-----------------------------------------------------*/
//A éviter, complexité en O(nbRoom*Socket), c'est mauvais
char* findRoomFromSocket(int sock){
	int i, j;
	for (i = 0; i < room.sz; ++i){
		for (j = 0; j < room.room[i].sz; ++j){
			if (room.room[i].socks[j] == sock){
				return room.room[i].name;
			}
		}
	}
	char* nrf = "NoRoomFound";
	return nrf;
}

/*-----------------------------------------------------*/
int removeRoom(char* roomName){
	int i, j;
	while (i < room.sz && (strcmp(room.room[i].name, roomName) != 0)){		
		++i;
	}
	if (i == room.sz){
		return 0;
	} 		
	//bonne room
	while (i < room.sz -2){
		room.room[i] = room.room[i+1];
	}
	room.sz = room.sz - 1;
	return 1;
}

/*------------------------------------------------------*/
int removeSocketFromRoom(int sock, char* roomName){
	int i, j;
	pthread_mutex_lock(&mutexRoom);
	for (i = 0; i < room.sz; ++i){
		if (strcmp(room.room[i].name, roomName) == 0){
			//bonne room
			j = 0;
			
			//On se rend a la cellule ou est lelement a retirer
			while (room.room[i].socks[j] != sock){
				j++;
			}
			
			//On decalle tous les elements suivants
			while (j < (room.room[i].sz -2)){
				room.room[i].socks[j] = room.room[i].socks[j+1];
			}
			
			//On reduit la taille de 1, comme ça le dernier element ne compte plus
			room.room[i].sz = room.room[i].sz - 1;
			
			if (room.room[i].sz == 0){
				removeRoom(roomName);
			}
			
			return 1;
		}
	}
	return 0;
}

/*------------------------------------------------------*/
void afficherRooms(){
	int i = 0;
	int j = 0;
	printf("--------Affichage de toutes les rooms----------\n");
	for (i = 0; i < room.sz; ++i){
		printf("%s\n", room.room[i].name);
		for (j = 0; j < room.room[i].sz; ++j){
			printf("    |------Socket: %d\n", room.room[i].socks[j]);
		}
	}
	printf("----------Fin affichage---------------\n");
}

/*------------------------------------------------------*/
/* 1 -> envoi reussi */
void * sendMessageToRoom(void* rMsg) {
	int curr;
	int curr2;
	msgToRoomStruct* roomMsg = rMsg;
	int i = room.sz;
	for (curr = 0; curr < i; ++curr){
		if (strcmp(room.room[curr].name, roomMsg -> roomName) == 0){
			//bonne room			
			for (curr2 =0; curr2 < room.room[curr].sz; ++curr2){
				write(room.room[curr].socks[curr2], roomMsg -> msg, strlen(roomMsg -> msg)+1);
			}
		}
	}
}

/*------------------------------------------------------*/
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

/*------------------------------------------------------*/
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

/*------------------------------------------------------*/
/* retourne 1 si l'incrementation est effectuée, -1 sinon */
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
			  		if (atoi(nbinsults) >= 15){
			  			//kick user
			  			int userCursor = 0;
			  			while (ips[userCursor].sin_addr.s_addr != adresse.sin_addr.s_addr && userCursor < 104){
			  				userCursor ++;			  				
			  			}
						write(userCursor, "22", 2);
						removeSocketFromRoom(userCursor, findRoomFromSocket(userCursor));
			  		}
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

/*------------------------------------------------------*/
int addUser(users *u, int* sock) {

	if (! u -> sz){
		//salle existante mais vide
		u -> sz = 1;
		u -> socks[0] = *sock;
		currentSock = *sock;
		write(*sock, "24", 3);
		pthread_mutex_unlock(&mutexRoom);		
		return 1;
	} else {
		if (u -> sz >= 10){
			//Salle pleine
			write(*sock, "23", 3);
			pthread_mutex_unlock(&mutexRoom);
			return 0;
		}		
		
		//verification de la presence d'une sock avant son ajout
		int i;
		for (i = 0; i < u -> sz; ++i){
			if (u -> socks[i] == *sock){
				//sock deja present				
				pthread_mutex_unlock(&mutexRoom);
				return 1;
			}
		}
		//Ajout de l'utilisateur a la fin du tableau
		u ->socks[u -> sz] = *sock;
		//Incrémentation du nombre d'utilisateurs
		u -> sz ++;
		//Envoi de la confirmation au client
		write(*sock, "25", 2);
		currentSock = *sock;
		pthread_mutex_unlock(&mutexRoom);
		return 1;
	}
}
/*------------------------------------------------------*/
/*
 *@brief Ajout d'utilisateur dans une room. Si la room existe pas on la crée
 *
 */
int addUserInRoom(int* sock, char* roomName){
	pthread_mutex_lock(&mutexRoom);
	int cpt;
	if (room.sz == 0){
		//aucune salle existe on la crée:		
		room.room[0].name = roomName;
		room.sz = 1;	
		//On ajoute l'utilisateur
		//Salon crée
		//Envoi de la confirmation au client		
		return addUser(&(room.room[0]), sock);
	}
	
	for (cpt = 0; cpt < room.sz; cpt++){
		if (strcmp(room.room[cpt].name, roomName) == 0){
			//La salle existe déjà
			//On ajoute l'utilisateur dedans
			return addUser(&(room.room[cpt]), sock);						
		}
	}
	
	//On a atteint le maximum de room
	if (room.sz >= 10){
		pthread_mutex_unlock(&mutexRoom);
		return 0;		
	} else {
		//Création d'une room au bout du tableau
		room.room[room.sz].name = roomName;
		//Augmentation du nombre de room
		room.sz ++;	
		//On ajoute l'utilisateur
		return addUser(&(room.room[room.sz-1]), sock);
	}
}

/*------------------------------------------------------*/
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
char* getServerResponse(char* commandLine){
	char *response;
	if (!strcmp(commandLine, "@exit\0")){
		response = "20";
		printf("@exit\n");
	} else
	if (!strcmp(commandLine, "@kick\0")){
		response = "22";
	} else
	if (!strcmp(commandLine, "@help\0")){
		response = "26";
	} else
	if (!strcmp(commandLine, "@sock\0")){
		response = "27";
	} else {
		response = "28";
	}
	return response;
}
/*------------------------------------------------------*/

void *renvoi_message(void *arg){
    int *s = arg;
    int *sock;
    sock = malloc(sizeof(int));
    *sock = *s;
    
    // Récupérer les 12 premiers caractères de la trame --> room
	
    time_t seconds;
    struct tm instant;

    //boucle de communication avec un client
    while(1){ 
    	char date[11];
    	char message[491];
    	char roomname[14];
    	int longueur;
    	char buffer2[242];
    	char buffer[257];

    	
    	char command;
    	
		if ((longueur = read(*sock, buffer, sizeof(buffer))) <= 0){
			continue;
		}
		
		/* Création de la trame */
		
		printf("---%s\n",buffer);
		//Récupération du type de message
		command = buffer[0];
		//Traitement du message;

		//recuperation du nom de la room
		memcpy(roomname, buffer, 14);
		
		if (*sock > currentSock){
			addUserInRoom(arg, roomname);
			afficherRooms();
		}				
		//On enleve la room
		memcpy(buffer2, buffer+14, strlen(buffer+14)+1);
		if (command == '0'){
			//récupération du message
			
		} else if (command == '1'){
			//Commande utilisateur de type @command
			//Recupération de la commande 
			char *commandLine = malloc(6* sizeof(char));
			//on prend les 5 dernier char
			commandLine = strchr(buffer2, '@');
			commandLine[5] = '\0';
			char resp[2];
			printf("commande: %s||\n", commandLine);
			memcpy(resp, getServerResponse(commandLine), 2);
			write(*sock, resp, 2);
		}
		
		//Création du champs date
		time(&seconds);
    	instant = *localtime(&seconds);
    	
    	//Concaténation des champs
		snprintf(date, sizeof date, "[%d:%d:%d]", instant.tm_hour, instant.tm_min, instant.tm_sec); //Date 
		snprintf(message, sizeof message, "%c%s %s \n", command, date, buffer2); // Command + Date + Message
		analyseMessage(message, &dict, sock);
						
		//ecriture dans la room
		pthread_t t;
		msgToRoomStruct msgStr;
		msgStr.roomName = roomname;
		msgStr.msg = message;
		// Envoi du message
		if (pthread_create(&t, NULL, sendMessageToRoom, (void*)&msgStr) == -1){
    		perror("pthread_create");    	
    	}
		memset(buffer,' ',strlen(buffer));
    }
	pthread_exit(NULL);
}

/*------------------------------------------------------*/
void stop() {

	int curr;
	int curr2;
	int i = room.sz;
	for (curr = 0; curr < i; ++curr){
			for (curr2 =0; curr2 < room.room[curr].sz; ++curr2){
				write(room.room[curr].socks[curr2], "21", 3);
				close(room.room[curr].socks[curr2]);
			}
	}
	pthread_cancel(t1);
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
    currentSock = 0;
    
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
		
		updateUserFile(adresse_client_courant);
		//association socket -> ip
		ips[nouv_socket_descriptor] = adresse_client_courant;
    	if (pthread_create(&t1, NULL, renvoi_message, &nouv_socket_descriptor) == -1){
    		perror("pthread_create");    	
    	}
    	
    }
}

