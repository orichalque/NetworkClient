/*-----------------------------------------------------------
Client a lancer apres le serveur avec la commande :
client <adresse-serveur> <pseudonyme>
------------------------------------------------------------*/

#include "client.h"
sockaddr_in adresse_locale;
hostent *ptr_host;
servent *ptr_service;

pthread_t t1;
pthread_t t2;

int socket_descriptor;
char *host;
char *pseudo;
char *salon; // limite 16 char
char pseudoWithComa[14];
char pseudoWithComaAndRoom[26];

/* ----------------------------------------------------------------- */
void stop() {
  // message automatique pour prevenir de la deconnexion
  char mesg[512];
  int longueur;
  strcpy(mesg, strcat(pseudo, " est deconnecte."));

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

/* ----------------------------------------------------------------- */
void *envoi_message(void *arg) {
  char buffer[257];
  char mesg[512];
  int longueur;
  char input[489];
  strcpy(input, arg);
  if (input[0] == '@') { // commande utilisateur
    strcpy(mesg, "1");
  } else { // message utilisateur
    strcpy(mesg, "0");
  }
  strcat(mesg, pseudoWithComaAndRoom);
  strcat(mesg, arg);
  mesg[strlen(mesg)] = '\0';
  // envoi du message vers le serveur
  if ((write(socket_descriptor, mesg, strlen(mesg))) < 0) {
    perror("erreur : impossible d'ecrire le message destine au serveur.");
    stop();
  }
  pthread_exit(NULL);
}

/* ----------------------------------------------------------------- */
void *reception_message(void *arg) {
  char buffer[257];
  int longueur;

  // lecture de la reponse en provenance du serveur
  while (1) {
    memset(buffer, ' ', strlen(buffer));
    if ((longueur = read(socket_descriptor, buffer, sizeof(buffer))) > 0) {
      if (buffer[0] == '0') { // Message utilisateur recu
        write(1, buffer + 1, longueur);
      } else if (buffer[0] == '2') { // Message systeme recu
        printf("######-Message systeme-######\n");
        if (buffer[1] == '0') {
          printf("Le salon a ete ferme par l'administrateur\n");
          printf("Deconnexion ...\n");
          exit(0);
          break;
        } else if (buffer[1] == '1') {
          printf("Le serveur a ete interrompue ferme\n");
          printf("Deconnexion ...\n");
          exit(0);
          break;
        } else if (buffer[1] == '2') {
          printf(
              "Vous etes expulse du serveur. Un peu de courtoisie. Merci !\n");
          printf("Deconnexion ...\n");
          exit(0);
          break;
        } else if (buffer[1] == '3') {
          printf("le salon est plein. Revenez plus tard\n");
          printf("Deconnexion ...\n");
          exit(0);
        } else if (buffer[1] == '4') {
          printf("Vous venez de creer un salon.\n");
        } else if (buffer[1] == '5') {
          printf("Vous venez de rejoindre un salon.\n");
        } else if (buffer[1] == '6') {
          printf("Les commandes disponibles:");
          printf("%s\n", strchr(buffer, ':') + 1);
        } else if (buffer[1] == '7') {
          printf("dernier socket:\n");
          printf("%s\n", strchr(buffer, ':') + 1);
        } else if (buffer[1] == '8') {
          printf("Vous venez d'expulser un utilisateur.\n");
        } else if (buffer[1] == '9') {
          printf("Vous ne pouvez que kick le dernier utilisateur.\n");
        } else {
          printf("message systeme inconnu.\n");
        }
      }
    }
  }
  pthread_exit(NULL);
}

/* ----------------------------------------------------------------- */
int main(int argc, char **argv) {
  char mesg[512];
  if (argc < 4) { // verification du nombre d'argument
    perror("Pas assez d'arguments : ./client <adresse-serveur> <pseudonyme> "
           "<room> ");
    exit(1);
  }
  if (argc > 4) {
    perror(
        "Trop d'arguments : ./client <adresse-serveur> <pseudonyme> <room> ");
    exit(1);
  }

  if (argc == 4) {
    if (strlen(argv[3]) > 14) {
      memcpy(salon, &argv[3][0], 14);
    } else {
      salon = argv[3];
      while (strlen(salon) < 14) {
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
    perror(
        "erreur : impossible de trouver le serveur a partir de son adresse.");
    exit(1);
  }

  // copie caractere par caractere des infos de ptr_host vers adresse_locale
  bcopy((char *)ptr_host->h_addr, (char *)&adresse_locale.sin_addr,
        ptr_host->h_length);
  adresse_locale.sin_family = AF_INET;
  // utilisation du port suivant
  adresse_locale.sin_port = htons(5000);
  printf("numero de port pour la connexion au serveur : %d \n",
         ntohs(adresse_locale.sin_port));

  // creation de la socket
  if ((socket_descriptor = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror(
        "erreur : impossible de creer la socket de connexion avec le serveur.");
    exit(1);
  }
  // tentative de connexion avec le serveur
  if ((connect(socket_descriptor, (sockaddr *)(&adresse_locale),
               sizeof(adresse_locale))) < 0) {
    perror("erreur : impossible de se connecter au serveur.");
    exit(1);
  }

  printf("connexion etablie avec le serveur. \n");
  strcpy(mesg, "vient de rejoindre le salon");
  pthread_create(&t1, NULL, (void *(*)(void *))envoi_message, &mesg);
  // execution de stop() si l'utilisateur presse CTRL+C
  signal(SIGINT, stop);
  if (pthread_create(&t2, NULL, (void *(*)(void *))reception_message, NULL) ==
      -1) {
    perror("Impossible de creer le thread de reception de message");
  }
  while (1) {
    fgets(mesg, 489, stdin); // pas plus de 489 car + car fin chaine
    if (pthread_create(&t1, NULL, (void *(*)(void *))envoi_message, &mesg) ==
        -1) {
      perror("Impossible de creer le thread d'envoi de message");
    }
  }

  exit(0);
}
