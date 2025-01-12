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
      case ACHAT :   
                    {
                      fprintf(stderr,"(ACCESBD %d) Requete ACHAT reçue de %d\n",getpid(),m.expediteur);
                      
                      // Construction de la requête SELECT
                      sprintf(requete, "SELECT * FROM UNIX_FINAL WHERE id = %d;", m.data1);

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

                      // Finalisation et envoi de la reponse
                      // Affichage des résultats
                      int num_champs = mysql_num_fields(resultat);

                      if ((Tuple = mysql_fetch_row(resultat))) 
                      {
                        reponse.data1 = atoi(Tuple[0]); //id
                        strncpy(reponse.data2, Tuple[1], sizeof(reponse.data2)); //nom de l'article
                        fprintf(stderr, "-----L element recupere se nomme : %s------\n", reponse.data2);
                        reponse.data5 = atof(Tuple[2]); //prix unitaire
                        strncpy(reponse.data3, Tuple[3], sizeof(reponse.data3)); //stock
                        strncpy(reponse.data4, Tuple[4], sizeof(reponse.data2)); //chemin de l'image
                      }
                      else
                        reponse.data1 = -1;

                      int stockActuel = atoi(reponse.data3) - atoi(m.data2);
                      
                      if(stockActuel < 0)
                      {
                        // Stock insuffisant
                        strcpy(reponse.data3, "0");
                      }
                      else
                      {
                        // Stock suffisant, mise à jour du stock
                        sprintf(requete, "UPDATE UNIX_FINAL SET stock = %d WHERE id = %d;", stockActuel, reponse.data1);
                        fprintf(stderr, "---->%s\n", requete);

                        if (mysql_query(connexion, requete)) 
                        {
                            fprintf(stderr, "Erreur lors de la requête UPDATE : %s\n", mysql_error(connexion));
                            mysql_close(connexion);
                            exit(1);
                        }

                        // Réponse avec la quantité achetée
                        strcpy(reponse.data3, m.data2);
                      }

                      // Envoi de la reponse au bon caddie
                      reponse.type = m.expediteur;
                      reponse.expediteur = getpid();
                      reponse.requete = CONSULT;
                      if(msgsnd(idQ, &reponse, sizeof(MESSAGE) - sizeof(long), 0) == -1)
                      {
                        perror("Erreur de msgsnd ACHAT de AccesBD\n");
                        exit(1);
                      }

                      fprintf(stderr,"Mise a jour des achats envoyé par AccesDB\n");
                      break;
                    }
      case CANCEL :   
                    {
                      // Acces BD
                      // Construction de la requête SELECT
                      fprintf(stderr, "(ACCESDB %d) Element a supprimer, id = %d, stock  dans le panier = %s\n", getpid(), m.data1, m.data2);

                      sprintf(requete, "SELECT * FROM UNIX_FINAL WHERE id = %d;", m.data1);

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

                      // Finalisation et envoi de la reponse
                      // Affichage des résultats
                      int num_champs = mysql_num_fields(resultat);

                      if ((Tuple = mysql_fetch_row(resultat))) 
                      {
                        reponse.data1 = atoi(Tuple[0]); //id
                        strncpy(reponse.data2, Tuple[1], sizeof(reponse.data2)); //nom de l'article
                        fprintf(stderr, "-----L element recupere se nomme : %s ", reponse.data2);
                        reponse.data5 = atof(Tuple[2]); //prix unitaire
                        strncpy(reponse.data3, Tuple[3], sizeof(reponse.data3)); //stock
                        fprintf(stderr, ", et il en reste : %s------\n", reponse.data3);
                        strncpy(reponse.data4, Tuple[4], sizeof(reponse.data2)); //chemin de l'image
                      }
                      else
                        reponse.data1 = -1;

                      int stockActuel = atoi(reponse.data3) + atoi(m.data2);

                      // Mise à jour du stock en BD
                      sprintf(requete, "UPDATE UNIX_FINAL SET stock = %d WHERE id = %d;", stockActuel, reponse.data1);
                      fprintf(stderr, "---->%s\n", requete);

                      if (mysql_query(connexion, requete)) 
                      {
                          fprintf(stderr, "Erreur lors de la requête UPDATE : %s\n", mysql_error(connexion));
                          mysql_close(connexion);
                          exit(1);
                      }
                      fprintf(stderr, "-----Apres mise a jour du stock : %d------\n", stockActuel);
                      fprintf(stderr,"(ACCESBD %d) Requete CANCEL reçue de %d\n",getpid(),m.expediteur);
                      break;
                    }

    }
  }
}
