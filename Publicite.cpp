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
#include "protocole.h" // contient la cle et la structure d'un message

int idQ, idShm;
char *pShm;
void handlerSIGUSR1(int sig);
int fd;
int i;
MESSAGE m;

int main()
{
  // Armement des signaux
  // TO DO

  // Masquage des signaux
  sigset_t mask;
  sigfillset(&mask);
  sigdelset(&mask,SIGUSR1);
  sigprocmask(SIG_SETMASK,&mask,NULL);

  // Recuperation de l'identifiant de la file de messages
  fprintf(stderr,"(PUBLICITE %d) Recuperation de l'id de la file de messages\n",getpid());
  if ((idQ = msgget(CLE,0)) == -1)
  {
    perror("(PUBLICITE) Erreur de msgget");
    exit(1);
  }

  // Recuperation de l'identifiant de la mémoire partagée
  if ((idShm = shmget(CLE,0,0)) == -1)
  {
  perror("Erreur de shmget");
  exit(1);
  }
  fprintf(stderr,"idShm = %d\n",idShm);

  // Attachement à la mémoire partagée
  if ((pShm = (char*)shmat(idShm,NULL,0)) == (char*)-1)
  {
  perror("Erreur de shmat");
  exit(1);
  }
  fprintf(stderr,"pShm = %ld\n",pShm);

  // Mise en place de la publicité en mémoire partagée
  char pub[51];
  strcpy(pub,"Bienvenue sur le site du Maraicher en ligne !");

  for (int i=0 ; i<=50 ; i++) pShm[i] = ' ';
  pShm[51] = '\0';
  int indDebut = 25 - strlen(pub)/2;
  for (int i=0 ; i<strlen(pub) ; i++) pShm[indDebut + i] = pub[i];

  while(1)
  {
    // Envoi d'une requete UPDATE_PUB au serveur
    m.type = 1;
    m.expediteur = getpid();
    m.requete = UPDATE_PUB;

    if(msgsnd(idQ, &m, sizeof(MESSAGE) - sizeof(long), 0) == -1)
    {
      perror("Erreur de msgsnd UPDATE_PUB de la publicité\n");
      exit(1);
    }
    fprintf(stderr,"Mise a jour envoyé\n");

    sleep(1); 

    // Decallage vers la gauche
    char firstChar = pShm[0];
    for (i = 0; i < 50; i++) 
    {
        pShm[i] = pShm[i + 1];
    }
    pShm[50] = firstChar;
  }
}

void handlerSIGUSR1(int sig)
{
  fprintf(stderr,"(PUBLICITE %d) Nouvelle publicite !\n",getpid());

  // Lecture message NEW_PUB

  // Mise en place de la publicité en mémoire partagée
}
