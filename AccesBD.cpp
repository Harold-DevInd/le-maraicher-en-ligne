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
MYSQL* connexion;
MYSQL_RES  *resultat;
MYSQL_ROW  Tuple;
char requete[200];

int main(int argc,char* argv[])
{
  // Masquage de SIGINT
  sigset_t mask;
  sigaddset(&mask,SIGINT);
  sigprocmask(SIG_SETMASK,&mask,NULL);

  // Recuperation de l'identifiant de la file de messages
  fprintf(stderr,"(ACCESBD %d) Recuperation de l'id de la file de messages\n",getpid());
  if ((idQ = msgget(CLE,0)) == -1)
  {
    perror("(ACCESBD) Erreur de msgget");
    exit(1);
  }

  // Récupération descripteur lecture du pipe
  int fdRpipe = atoi(argv[1]);

  // Connexion à la base de donnée
  connexion = mysql_init(NULL);
  if (mysql_real_connect(connexion,"localhost","Student","PassStudent1_","PourStudent",0,0,0) == NULL)
  {
    fprintf(stderr,"(ACCESBD) Erreur de connexion à la base de données...\n");
    exit(1);  
  }
  fprintf(stderr,"\n(ACCESBD %d) apres connexion a la BD\n", getpid());

  MESSAGE m;
  MESSAGE reponse;

  while(1)
  {
    // Lecture d'une requete sur le pipe
    if ((read(fdRpipe,&m,sizeof(MESSAGE))) < 0) 
    {
      perror("Erreur de read dans AccesBD");
      mysql_close(connexion);
      close(fdRpipe);
      exit(1); 
    }

    switch(m.requete)
    {
      case CONSULT :  
                    {
                      fprintf(stderr,"(ACCESBD %d) Requete CONSULT reçue de %d\n",getpid(),m.expediteur);
                      
                      // Construction de la requête SELECT
                      sprintf(requete, "SELECT * FROM UNIX_FINAL WHERE id = %d;", m.data1);
                      fprintf(stderr, "%s\n", requete);

                      // Exécution de la requête
                      if (mysql_query(connexion, requete)) {
                          fprintf(stderr, "Erreur lors de la requête SELECT : %s\n", mysql_error(connexion));
                          mysql_close(connexion);
                          exit(1);
                      }

                      // Acces BD
                      // Récupération des résultats
                      resultat = mysql_store_result(connexion);
                      if (resultat == NULL) {
                          fprintf(stderr, "Erreur lors de la récupération des résultats : %s\n", mysql_error(connexion));
                          mysql_close(connexion);
                          exit(1);
                      }

                      // Preparation de la reponse
                      // Affichage des résultats
                      int num_champs = mysql_num_fields(resultat);

                      if ((Tuple = mysql_fetch_row(resultat))) 
                      {
                        reponse.data1 = atoi(Tuple[0]); //id
                        fprintf(stderr,"Valeur retourne apre la recherche avec pour id = %d\n", reponse.data1);
                        strncpy(reponse.data2, Tuple[1], sizeof(reponse.data2)); //nom de l'article
                        reponse.data5 = atof(Tuple[2]); //prix unitaire
                        strncpy(reponse.data3, Tuple[3], sizeof(reponse.data3)); //stock
                        strncpy(reponse.data4, Tuple[4], sizeof(reponse.data2)); //chemin de l'image
                      }
                      else
                        reponse.data1 = -1;

                      // Envoi de la reponse au bon caddie
                      reponse.type = m.expediteur;
                      reponse.expediteur = getpid();
                      reponse.requete = CONSULT;
                      if(msgsnd(idQ, &reponse, sizeof(MESSAGE) - sizeof(long), 0) == -1)
                      {
                        perror("Erreur de msgsnd CONSULT de AccesBD\n");
                        exit(1);
                      }

                      fprintf(stderr,"Mise a jour des consultation envoyé par AccesDB\n");
                      break;
                    }
      case ACHAT :    // TO DO
                      fprintf(stderr,"(ACCESBD %d) Requete ACHAT reçue de %d\n",getpid(),m.expediteur);
                      // Acces BD

                      // Finalisation et envoi de la reponse
                      break;

      case CANCEL :   // TO DO
                      fprintf(stderr,"(ACCESBD %d) Requete CANCEL reçue de %d\n",getpid(),m.expediteur);
                      // Acces BD

                      // Mise à jour du stock en BD
                      break;

    }
  }
}
