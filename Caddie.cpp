#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <mysql.h>
#include "protocole.h" // contient la cle et la structure d'un message

int idQ;

ARTICLE articles[10];
int nbArticles = 0;

int fdWpipe;
int pidClient;

MYSQL* connexion;

void handlerSIGALRM(int sig);

int main(int argc,char* argv[])
{
  // Masquage de SIGINT
  sigset_t mask;
  sigaddset(&mask,SIGINT);
  sigprocmask(SIG_SETMASK,&mask,NULL);

  // Armement des signaux
  // TO DO

  // Recuperation de l'identifiant de la file de messages
  fprintf(stderr,"(CADDIE %d) Recuperation de l'id de la file de messages\n",getpid());
  if ((idQ = msgget(CLE,0)) == -1)
  {
    perror("(CADDIE) Erreur de msgget");
    exit(1);
  }

  // Connexion à la base de donnée
  connexion = mysql_init(NULL);
  if (mysql_real_connect(connexion,"localhost","Student","PassStudent1_","PourStudent",0,0,0) == NULL)
  {
    fprintf(stderr,"(CADDIE) Erreur de connexion à la base de données...\n");
    exit(1);  
  }
  fprintf(stderr,"\n(CADDIE %d) apres connexion a la BD\n", getpid());

  MESSAGE m;
  MESSAGE reponse;
  
  char requete[200];
  char newUser[20];
  MYSQL_RES  *resultat;
  MYSQL_ROW  Tuple;

  fprintf(stderr,"\n(CADDIE %d) avant la recuperation du pipe\n", getpid());
  // Récupération descripteur écriture du pipe
  fdWpipe = atoi(argv[1]);
  fprintf(stderr,"\n(CADDIE %d) apres la recuperation du pipe\n", getpid());

  while(1)
  {
    fprintf(stderr,"\n(CADDIE %d) En attente de requete\n", getpid());
    if (msgrcv(idQ,&m,sizeof(MESSAGE)-sizeof(long),getpid(),0) == -1)
    {
      perror("(CADDIE) Erreur de msgrcv");
      exit(1);
    }

    switch(m.requete)
    {
      case LOGIN :  {
                      fprintf(stderr,"(CADDIE %d) Requete LOGIN reçue de %d\n",getpid(),m.expediteur);
                      break;
                    }
      case LOGOUT :   
                    {
                      mysql_close(connexion);
                      exit(0);
                      fprintf(stderr,"(CADDIE %d) Requete LOGOUT reçue de %d\n",getpid(),m.expediteur);
                      break;
                    }
      case CONSULT :  
                    {
                      reponse.type = m.expediteur;
                      reponse.expediteur = getpid();
                      reponse.requete = CONSULT;

                      // Construction de la requête SELECT
                      sprintf(requete, "SELECT * FROM UNIX_FINAL WHERE id = %d;", m.data1);

                      // Exécution de la requête
                      if (mysql_query(connexion, requete)) {
                          fprintf(stderr, "Erreur lors de la requête SELECT : %s\n", mysql_error(connexion));
                          mysql_close(connexion);
                          exit(1);
                      }

                      // Récupération des résultats
                      resultat = mysql_store_result(connexion);
                      if (resultat == NULL) {
                          fprintf(stderr, "Erreur lors de la récupération des résultats : %s\n", mysql_error(connexion));
                          mysql_close(connexion);
                          exit(1);
                      }

                      // Affichage des résultats
                      int num_champs = mysql_num_fields(resultat);

                      if ((Tuple = mysql_fetch_row(resultat))) 
                      {
                        reponse.data1 = atoi(Tuple[0]); //id
                        strncpy(reponse.data2, Tuple[1], sizeof(reponse.data2)); //nom de l'article
                        reponse.data5 = atof(Tuple[2]); //prix unitaire
                        strncpy(reponse.data3, Tuple[3], sizeof(reponse.data3)); //stock
                        strncpy(reponse.data4, Tuple[4], sizeof(reponse.data2)); //chemin de l'image
                      }

                      if(msgsnd(idQ, &reponse, sizeof(MESSAGE) - sizeof(long), 0) == -1)
                      {
                        perror("Erreur de msgsnd CONSULT de caddie\n");
                        exit(1);
                      }
                      kill(m.expediteur, SIGUSR1);
                      fprintf(stderr,"Mise a jour envoyé\n");

                      fprintf(stderr,"(CADDIE %d) Requete CONSULT reçue de %d\n",getpid(),m.expediteur);
                      break;
                    }
      case ACHAT :    // TO DO
                      fprintf(stderr,"(CADDIE %d) Requete ACHAT reçue de %d\n",getpid(),m.expediteur);

                      // on transfert la requete à AccesBD
                      
                      // on attend la réponse venant de AccesBD
                        
                      // Envoi de la reponse au client

                      break;

      case CADDIE :   // TO DO
                      fprintf(stderr,"(CADDIE %d) Requete CADDIE reçue de %d\n",getpid(),m.expediteur);
                      break;

      case CANCEL :   // TO DO
                      fprintf(stderr,"(CADDIE %d) Requete CANCEL reçue de %d\n",getpid(),m.expediteur);

                      // on transmet la requete à AccesBD

                      // Suppression de l'aricle du panier
                      break;

      case CANCEL_ALL : // TO DO
                      fprintf(stderr,"(CADDIE %d) Requete CANCEL_ALL reçue de %d\n",getpid(),m.expediteur);

                      // On envoie a AccesBD autant de requeres CANCEL qu'il y a d'articles dans le panier

                      // On vide le panier
                      break;

      case PAYER :    // TO DO
                      fprintf(stderr,"(CADDIE %d) Requete PAYER reçue de %d\n",getpid(),m.expediteur);

                      // On vide le panier
                      break;
    }
  }
}

void handlerSIGALRM(int sig)
{
  fprintf(stderr,"(CADDIE %d) Time Out !!!\n",getpid());

  // Annulation du caddie et mise à jour de la BD
  // On envoie a AccesBD autant de requetes CANCEL qu'il y a d'articles dans le panier

  // Envoi d'un Time Out au client (s'il existe toujours)
         
  exit(0);
}