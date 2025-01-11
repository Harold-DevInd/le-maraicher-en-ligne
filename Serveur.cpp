#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <setjmp.h>
#include "protocole.h" // contient la cle et la structure d'un message

int idQ,idShm,idSem;
int i, idPub, idCad;
int fdPipe[2];
TAB_CONNEXIONS *tab;
sigjmp_buf contexte;

// Pour le fichier des utilisateurs
#define FICHIER_UTILISATEURS "clients.dat"

typedef struct
{
  char  nom[20];
  int   hash;
} UTILISATEUR;

//Handler qui gere la suppression de la file
void HandlerSIGINT(int);
void HandlerSIGUSR2(int);
void HandlerSIGCHLD(int sig);

struct sigaction A;
struct sigaction B;
struct sigaction C;

void afficheTab();
int estPresent(const char* nom);
int hash(const char* motDePasse);
void ajouteUtilisateur(const char* nom, const char* motDePasse);
int verifieMotDePasse(int pos, const char* motDePasse);

int main()
{
  // Armement des signaux
  // Armement du signal SIGINT
  A.sa_handler = HandlerSIGINT;
  sigemptyset(&A.sa_mask);
  A.sa_flags = 0;

  if(sigaction(SIGINT, &A, NULL) == -1)
  {
    perror("Erreur de SIGINT dans le serveur");
  }

  // Armement du signal SIGUSR1
  B.sa_handler = HandlerSIGUSR2;
  sigemptyset(&B.sa_mask);
  B.sa_flags = 0;

  if(sigaction(SIGUSR2, &B, NULL) == -1)
  {
    perror("Erreur de SIGUSR2 dans le serveur");
  }

  // Armement du signal SIGCHLD
  C.sa_handler = HandlerSIGCHLD;
  sigemptyset(&C.sa_mask);
  C.sa_flags = 0;

  if(sigaction(SIGCHLD, &C, NULL) == -1)
  {
    perror("Erreur de SIGCHILD dans le serveur");
  }

  // Creation des ressources
  // Creation de la file de message
  fprintf(stderr,"(SERVEUR %d) Creation de la file de messages\n",getpid());
  if ((idQ = msgget(CLE,IPC_CREAT | IPC_EXCL | 0600)) == -1)  // CLE definie dans protocole.h
  {
    perror("(SERVEUR) Erreur de msgget");
    exit(1);
  }

  // TO BE CONTINUED
  //Creation de la  memoire partagé
  if ((idShm = shmget(CLE,52,IPC_CREAT | IPC_EXCL | 0600)) == -1)
  {
    perror("Erreur de shmget");
    exit(1);
  }
  fprintf(stderr,"idShm = %d\n",idShm);

  // Creation du pipe
  // TO DO

  // Creation du processus Publicite (étape 2)
  if((idPub = fork()) == -1)
  {
    perror("Erreur de Fork() pour la publicite");
    exit(1);
  }
  else if (idPub == 0)
        {
          if(execl("./Publicite", "Publicite", NULL) == -1)
          {
            perror("Erreur de EXECL fils 1\n");
            exit(1);
          }
        }

  // Initialisation du tableau de connexions
  tab = (TAB_CONNEXIONS*) malloc(sizeof(TAB_CONNEXIONS)); 

  for (int i=0 ; i<6 ; i++)
  {
    tab->connexions[i].pidFenetre = 0;
    strcpy(tab->connexions[i].nom,"");
    tab->connexions[i].pidCaddie = 0;
  }
  tab->pidServeur = getpid();
  tab->pidPublicite = idPub;

  afficheTab();

  // Creation du processus AccesBD (étape 4)
  // TO DO

  MESSAGE m;
  MESSAGE reponse;

  //Sauvegarde du contexte avant les signaux
  sigsetjmp(contexte,1);
  while(1)
  {
  	fprintf(stderr,"(SERVEUR %d) Attente d'une requete...\n",getpid());
    if (msgrcv(idQ,&m,sizeof(MESSAGE)-sizeof(long),1,0) == -1)
    {
      perror("(SERVEUR) Erreur de msgrcv");
      msgctl(idQ,IPC_RMID,NULL);
      exit(1);
    }

    switch(m.requete)
    {
      case CONNECT : 
                    {
                      for (i=0 ; i<6 ; i++)
                      {
                        if(tab->connexions[i].pidFenetre == 0)
                        {
                          tab->connexions[i].pidFenetre = m.expediteur;
                          i = 7;
                        }
                      }
                      fprintf(stderr,"(SERVEUR %d) Requete CONNECT reçue de %d\n",getpid(),m.expediteur);
                      break;
                    } 
      case DECONNECT : 
                    {
                      for (i=0 ; i<6 ; i++)
                      {
                        if(tab->connexions[i].pidFenetre == m.expediteur)
                        {
                          tab->connexions[i].pidFenetre = 0;
                          //strcpy(tab->connexions[i].nom,"");
                          //tab->connexions[i].pidCaddie = 0;
                          i = 7;
                        }
                      }
                      fprintf(stderr,"(SERVEUR %d) Requete DECONNECT reçue de %d\n",getpid(),m.expediteur);
                      break;
                    } 
      case LOGIN :  
                    {
                      int presence = estPresent(m.data2);
                      int res = -1;
                      reponse.type = m.expediteur;
                      reponse.expediteur = 1;
                      reponse.requete = LOGIN;

                      if (m.data1 == 1)
                      {
                        if(presence > 0)
                        {
                          reponse.data1 = 1;
                          strcpy(reponse.data4, " Utilisateur déjà existant ! ");
                          res = 1;
                        }
                        else {
                          ajouteUtilisateur(m.data2, m.data3);
                          reponse.data1 = 1;
                          strcpy(reponse.data4, " Nouvel utilisateur créé : bienvenue ! ");
                          res =1;
                        }
                      }
                      else 
                      {
                        if(presence > 0)
                        {
                          int mdp = verifieMotDePasse(presence, m.data3);

                          if(mdp == 1)
                          {
                            reponse.data1 = 1;
                            strcpy(reponse.data4, " Re-bonjour cher utilisateur ! ");
                            res = 1;
                          }
                          else{
                            reponse.data1 = 0;
                            strcpy(reponse.data4, " Mot de passe incorrect ! ");
                            res = 0;
                          }
                        }
                        else {
                          reponse.data1 = 0;
                          strcpy(reponse.data4, " Utilisateur inconnu... ");
                          res = 0;
                        }
                      }

                      if(res != -1)
                      {
                        //Creation du cadie
                        if((idCad = fork()) == -1)
                        {
                          perror("Erreur de Fork() pour le caddie");
                          exit(1);
                        }
                        else if (idCad == 0)
                              {
                                if(execl("./Caddie", "Caddie", "1", NULL) == -1)
                                {
                                  perror("Erreur de EXECL du caddie\n");
                                  exit(1);
                                }
                              }

                        for (i=0 ; i<6 ; i++)
                        {
                          if((tab->connexions[i].pidFenetre == m.expediteur) && res == 1)
                          {
                            strcpy(tab->connexions[i].nom,m.data2);
                            tab->connexions[i].pidCaddie = idCad;
                            i = 7;
                          }
                        }
                      }
                      if(msgsnd(idQ, &reponse, sizeof(MESSAGE) - sizeof(long), 0) == -1)
                      {
                        perror("Erreur de msgnd LOGIN du serveur vers le client\n");
                        exit(1);
                      }
                      fprintf(stderr,"(SERVEUR %d) Requete LOGIN reçue de %d : --%d--%s--%s--\n",getpid(),m.expediteur,m.data1,m.data2,m.data3);
                      kill(reponse.type, SIGUSR1);
                      break; 
                    }
      case LOGOUT :   
                    {
                      for (i=0 ; i<6 ; i++)
                      {
                        if(tab->connexions[i].pidFenetre == m.expediteur)
                        {
                          //tab->connexions[i].pidFenetre = m.expediteur;
                          strcpy(tab->connexions[i].nom,"");

                          //On tue le process caddie
                          kill(tab->connexions[i].pidCaddie, 9);
                          tab->connexions[i].pidCaddie = 0;
                          i = 7;
                        }
                      }
                      fprintf(stderr,"(SERVEUR %d) Requete LOGOUT reçue de %d\n",getpid(),m.expediteur);
                      break;
                    }
      case UPDATE_PUB : 
                      {
                        for (i=0 ; i<6 ; i++)
                        {
                          if(tab->connexions[i].pidFenetre != 0)
                          {
                            kill(tab->connexions[i].pidFenetre, SIGUSR2);
                          }
                        }
                        fprintf(stderr,"(SERVEUR %d) Requete UPDATE reçue de %d\n",getpid(),m.expediteur);
                        break;
                      }

      case CONSULT :  
                    {
                      for (i=0 ; i<6 ; i++)
                      {
                        if((tab->connexions[i].pidFenetre == m.expediteur) && (tab->connexions[i].pidCaddie !=0))
                        {
                          reponse.type = tab->connexions[i].pidCaddie;
                          reponse.expediteur = m.expediteur;
                          reponse.requete = CONSULT;
                          reponse.data1 = m.data1;

                          if(msgsnd(idQ, &reponse, sizeof(MESSAGE) - sizeof(long), 0) == -1)
                          {
                            perror("Erreur de msgnd CONSULT du serveur vers le caddie\n");
                            kill(idPub, 9);
                            exit(1);
                          }
                          i = 7;
                        }
                        else
                        {
                          fprintf(stderr,"(SERVEUR %d) Le caddie n'existe pas pour le client %d\n",getpid(), m.expediteur);
                        }
                      }
                      fprintf(stderr,"(SERVEUR %d) Requete CONSULT reçue de %d\n",getpid(),m.expediteur);
                      break;
                    }
      case ACHAT :    // TO DO
                      fprintf(stderr,"(SERVEUR %d) Requete ACHAT reçue de %d\n",getpid(),m.expediteur);
                      break;

      case CADDIE :   // TO DO
                      fprintf(stderr,"(SERVEUR %d) Requete CADDIE reçue de %d\n",getpid(),m.expediteur);
                      break;

      case CANCEL :   // TO DO
                      fprintf(stderr,"(SERVEUR %d) Requete CANCEL reçue de %d\n",getpid(),m.expediteur);
                      break;

      case CANCEL_ALL : // TO DO
                      fprintf(stderr,"(SERVEUR %d) Requete CANCEL_ALL reçue de %d\n",getpid(),m.expediteur);
                      break;

      case PAYER : // TO DO
                      fprintf(stderr,"(SERVEUR %d) Requete PAYER reçue de %d\n",getpid(),m.expediteur);
                      break;

      case NEW_PUB :  // TO DO
                      fprintf(stderr,"(SERVEUR %d) Requete NEW_PUB reçue de %d\n",getpid(),m.expediteur);
                      break;
    }
    afficheTab();
  }
}

void afficheTab()
{
  fprintf(stderr,"Pid Serveur   : %d\n",tab->pidServeur);
  fprintf(stderr,"Pid Publicite : %d\n",tab->pidPublicite);
  fprintf(stderr,"Pid AccesBD   : %d\n",tab->pidAccesBD);
  for (int i=0 ; i<6 ; i++)
    fprintf(stderr,"%6d -%20s- %6d\n",tab->connexions[i].pidFenetre,
                                                      tab->connexions[i].nom,
                                                      tab->connexions[i].pidCaddie);
  fprintf(stderr,"\n");
}

//////////////////////////Gestion du login/////////////////////////////////
int estPresent(const char* nom)
{
  int fd = open(FICHIER_UTILISATEURS, O_RDONLY);

  if (fd == -1)
  {
    return -1;
  }
  else
  {
    UTILISATEUR u;
    int i = 0, rc;

    while((rc = read(fd, &u, sizeof(UTILISATEUR))) >0 )
    {
      if (strcmp(u.nom, nom) == 0)
      {
        close(fd);
        return i + 1;
      }
      i++;
    }

    close(fd);
    return 0;
  }
}

int hash(const char* motDePasse)
{
  int mdpHashe, i = 1, x = 0;
  char buffer[2];

  while(*motDePasse != '\0')
  {

    x += (*motDePasse) * i;

    motDePasse++;
    i++;
  }

  //printf("modulo de %d par 97 est %d", x, mdpHashe);
  mdpHashe = x % 97;

  return mdpHashe;
}

void ajouteUtilisateur(const char* nom, const char* motDePasse)
{
  int fd = open(FICHIER_UTILISATEURS, O_WRONLY | O_APPEND | O_CREAT, 0644);
  int w;
  UTILISATEUR u;

  if(fd == -1)
  {
    perror("Erreur de open dans la fonction ajouteUtilisateur");
  }
  else
  {
    strcpy(u.nom, nom);
    u.hash = hash(motDePasse);

    w = write(fd, &u, sizeof(UTILISATEUR));
    if(w == -1)
    {
      perror("Erreur de write dans la fonction ajouteUtilisateur");
    }
  }
  
  close(fd);
}

int verifieMotDePasse(int pos, const char* motDePasse)
{
  UTILISATEUR u;
  int fd = open(FICHIER_UTILISATEURS, O_RDWR );
  int l, r;

  if(fd == -1)
  {
    return -1;
  }
  else{
    l = lseek(fd, sizeof(UTILISATEUR)* (long)(pos - 1), SEEK_SET);
    
    if(l  == -1)
    {
      perror("Erreur de lseek dans la fonction verifieMotDePasse");
      close(fd);
      return -1;
    }
    else{
      r = read(fd, &u, sizeof(UTILISATEUR));

      if(r < 0)
      {
        perror("Erreur de read dans la fonction verifieMotDePasse");
        close(fd);
        return -1;
      }
      else{
        if(u.hash == (hash(motDePasse)))
        {
          close(fd);
          return 1;
        }
      }
    }
  }

  close(fd);
  return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// Handlers de signaux ////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void HandlerSIGINT(int sig)
{
    // Suppression de la memoire partagee
  if (shmctl(idShm,IPC_RMID,NULL) == -1)
  {
    perror("Erreur de shmctl");
    exit(1);
  }

  if(msgctl(idQ, IPC_RMID, NULL) == -1)
  {
    perror("Erreur de msgctl sur le serveur");
    exit(1);
  }

  kill(idPub, 9);
  wait(NULL);
}

void HandlerSIGUSR2(int sig)
{

}

void HandlerSIGCHLD(int sig)
{
  int status, idFils;

  if((idFils = wait(NULL)) == -1)
  {
    perror("Erreur dans le wait du serveur");
    kill(idPub, 9);
    wait(NULL);
    exit(1);
  }

  for(i = 0; i < 6; i++)
  {
    if(idFils == tab->connexions[i].pidCaddie)
    {
      tab->connexions[i].pidCaddie = 0;
      fprintf(stderr, "(SERVEUR %d) Suppression du fils zombi %d\n", getpid(), idFils);
      i = 7;
    }
  }
  //Retour au while(1)
  siglongjmp(contexte,10);
}