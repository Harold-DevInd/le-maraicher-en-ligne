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

struct sigaction A;

void handlerSIGALRM(int sig);

int main(int argc,char* argv[])
{
  // Masquage de SIGINT
  sigset_t mask;
  sigaddset(&mask,SIGINT);
  sigprocmask(SIG_SETMASK,&mask, NULL);

  // Armement des signaux
  A.sa_handler = handlerSIGALRM;
  sigemptyset(&A.sa_mask);
  A.sa_flags = 0;

  if (sigaction(SIGALRM, &A, NULL) == -1)
  {
    perror("Erreur de sigaction");
    exit(1);
  }

  // Recuperation de l'identifiant de la file de messages
  fprintf(stderr,"(CADDIE %d) Recuperation de l'id de la file de messages\n",getpid());
  if ((idQ = msgget(CLE,0)) == -1)
  {
    perror("(CADDIE) Erreur de msgget");
    exit(1);
  }

  MESSAGE m;
  MESSAGE reponse;
  MESSAGE requete;
  
  char newUser[20];

  // Récupération descripteur écriture du pipe
  //fdWpipe = atoi(argv[1]);
  if (argc < 2) {
    fprintf(stderr, "(CADDIE %d) Aucun argument fourni pour fdWpipe\n", getpid());
    exit(1);
  }

  fdWpipe = atoi(argv[1]);
  if (fdWpipe <= 0) {
      fprintf(stderr, "(CADDIE %d) Descripteur de pipe invalide : %s\n", getpid(), argv[1]);
      exit(1);
  }

  while(1)
  {
    alarm(60);
    fprintf(stderr,"\n(CADDIE %d) En attente de requete\n", getpid());
    if (msgrcv(idQ,&m,sizeof(MESSAGE)-sizeof(long),getpid(),0) == -1)
    {
      perror("(CADDIE) Erreur de msgrcv");
      sleep(3);
      exit(1);
    }
    
    alarm(0);
    switch(m.requete)
    {
      case LOGIN :  {
                      pidClient = m.expediteur;
                      fprintf(stderr,"(CADDIE %d) Requete LOGIN reçue de %d\n",getpid(),m.expediteur);
                      break;
                    }
      case LOGOUT :   
                    {
                      mysql_close(connexion);
                      close(fdWpipe);
                      fprintf(stderr,"(CADDIE %d) Requete LOGOUT reçue de %d\n",getpid(),m.expediteur);
                      exit(0);
                      break;
                    }
      case CONSULT :  
                    {
                      requete.type = m.expediteur;
                      requete.expediteur = getpid();
                      requete.requete = CONSULT;
                      requete.data1 = m.data1;
                      if (write(fdWpipe, &requete, sizeof(MESSAGE)) != sizeof(MESSAGE)) 
                      {
                        perror("Erreur de write(1) dans le caddie"); 
                        sleep(3);
                        exit(1); 
                      }

                      if (msgrcv(idQ,&requete,sizeof(MESSAGE)-sizeof(long),getpid(),0) == -1)
                      {
                        perror("(CADDIE) Erreur de msgrcv dans le CONSULT du caddie\n");
                        sleep(3);
                        exit(1);
                      }

                      if(requete.data1 != -1)
                      {
                        reponse.type = pidClient;
                        reponse.expediteur = getpid();
                        reponse.requete = CONSULT;

                        reponse.data1 = requete.data1; //id
                        strncpy(reponse.data2, requete.data2, sizeof(reponse.data2)); //nom de l'article
                        strncpy(reponse.data3, requete.data3, sizeof(reponse.data3)); //stock
                        strncpy(reponse.data4, requete.data4, sizeof(reponse.data2)); //chemin de l'image
                        reponse.data5 = requete.data5; //prix unitaire

                        if(msgsnd(idQ, &reponse, sizeof(MESSAGE) - sizeof(long), 0) == -1)
                        {
                          perror("Erreur de msgsnd CONSULT de caddie\n");
                          sleep(3);
                          exit(1);
                        }
                        kill(m.expediteur, SIGUSR1);
                        fprintf(stderr,"Mise a jour des consultation envoyé par le Caddie\n");
                      }
                      
                      fprintf(stderr,"(CADDIE %d) Requete CONSULT reçue de %d\n",getpid(),m.expediteur);
                      break;
                    }
      case ACHAT :  
                    {
                      if(nbArticles < 10)
                      {
                        // on transfert la requete à AccesBD
                        requete.type = m.expediteur;
                        requete.expediteur = getpid();
                        requete.requete = ACHAT;
                        requete.data1 = m.data1;
                        strcpy(requete.data2, m.data2);
                        if (write(fdWpipe, &requete, sizeof(MESSAGE)) != sizeof(MESSAGE)) 
                        {
                          perror("Erreur de write(2) dans le caddie"); 
                          sleep(3);
                          exit(1); 
                        }

                        // on attend la réponse venant de AccesBD
                        if (msgrcv(idQ,&requete,sizeof(MESSAGE)-sizeof(long),getpid(),0) == -1)
                        {
                          perror("(CADDIE) Erreur de msgrcv dans le ACHAT du caddie\n");
                          sleep(3);
                          exit(1);
                        }

                        // Envoi de la reponse au client
                        if(atoi(requete.data3) != 0)
                        {
                          reponse.type = pidClient;
                          reponse.expediteur = getpid();
                          reponse.requete = ACHAT;

                          reponse.data1 = requete.data1; //id
                          strncpy(reponse.data2, requete.data2, sizeof(reponse.data2)); //nom de l'article
                          strncpy(reponse.data3, requete.data3, sizeof(reponse.data3)); //stock
                          strncpy(reponse.data4, requete.data4, sizeof(reponse.data2)); //chemin de l'image
                          reponse.data5 = requete.data5; //prix unitaire

                          if(msgsnd(idQ, &reponse, sizeof(MESSAGE) - sizeof(long), 0) == -1)
                          {
                            perror("Erreur de msgsnd ACHAT de caddie\n");
                            sleep(3);
                            exit(1);
                          }
                          kill(m.expediteur, SIGUSR1);
                          fprintf(stderr,"Mise a jour des achats envoyé par le Caddie\n");

                          //Ajout d un article a notre liste d'article a acheté  
                          articles[nbArticles].id = reponse.data1;
                          strncpy(articles[nbArticles].intitule, reponse.data2, sizeof(articles[nbArticles + 1].intitule));
                          articles[nbArticles].prix = reponse.data5;
                          articles[nbArticles].stock = atoi(reponse.data3);
                          strncpy(articles[nbArticles].image, reponse.data4, sizeof(articles[nbArticles + 1].image));

                          nbArticles++;
                        }
                      }
                      
                      fprintf(stderr,"(CADDIE %d) Requete ACHAT reçue de %d\n",getpid(),m.expediteur);
                      break;
                    }
      case CADDIE :   
                    {
                      //Envie e chaque artice au client pour affichage
                      for(int i = 0; i < nbArticles; i++)
                      {
                        reponse.data1 = articles[i].id;
                        strncpy(reponse.data2, articles[i].intitule, sizeof(reponse.data2));
                        reponse.data5 = articles[i].prix;
                        sprintf(reponse.data3, "%d", articles[i].stock);
                        strncpy(reponse.data4, articles[i].image, sizeof(reponse.data4));

                        reponse.type = pidClient;
                        reponse.expediteur = getpid();
                        reponse.requete = CADDIE;
                        if(msgsnd(idQ, &reponse, sizeof(MESSAGE) - sizeof(long), 0) == -1)
                        {
                          perror("Erreur de msgsnd CADDIE de caddie\n");
                          sleep(3);
                          exit(1);
                        }
                        kill(m.expediteur, SIGUSR1);
                        sleep(3);
                      }

                      fprintf(stderr,"(CADDIE %d) Requete CADDIE reçue de %d\n",getpid(),m.expediteur);
                      break;
                    }
      case CANCEL :   
                    {
                      //Recuperation de l'article a supprimer                        
                      requete.data1 = articles[m.data1].id;
                      sprintf(requete.data2, "%d", articles[m.data1].stock);
                      fprintf(stderr, "(CADDIE %d) Element a supprimer, id = %d, stock dans le panier = %s\n", getpid(), requete.data1, requete.data2);

                      // on transmet la requete à AccesBD
                      requete.type = m.expediteur;
                      requete.expediteur = getpid();
                      requete.requete = CANCEL;
                      if (write(fdWpipe, &requete, sizeof(MESSAGE)) != sizeof(MESSAGE)) 
                      {
                        perror("Erreur de write(3) dans le caddie"); 
                        sleep(3);
                        exit(1); 
                      }

                      // Suppression de l'aricle du panier
                      for(int i = m.data1; i < 9; i++)
                      {
                        articles[i] = articles[i + 1];
                      }
                      nbArticles--;
                      fprintf(stderr,"(CADDIE %d) Requete CANCEL reçue de %d\n",getpid(),m.expediteur);
                      break;
                    }

      case CANCEL_ALL : 
                    {
                      requete.type = m.expediteur;
                      // On envoie a AccesBD autant de requeres CANCEL qu'il y a d'articles dans le panier
                      for(int i = 0; i < nbArticles; i++)
                      {
                        requete.expediteur = getpid();
                        requete.requete = CANCEL;

                        sprintf(requete.data2,"%d", articles[i].stock);
                        requete.data1 = articles[i].id;
                        
                        
                        if (write(fdWpipe, &requete, sizeof(MESSAGE)) != sizeof(MESSAGE)) 
                        {
                          perror("Erreur de write(3) dans le caddie"); 
                          sleep(3);
                          exit(1); 
                        }
                      }
                      
                      // On vide le panier
                      nbArticles = 0;
                      fprintf(stderr,"(CADDIE %d) Requete CANCEL_ALL reçue de %d\n",getpid(),m.expediteur);
                      break;
                    }

      case PAYER :    // TO DO
                      fprintf(stderr,"(CADDIE %d) Requete PAYER reçue de %d\n",getpid(),m.expediteur);

                      // On vide le panier
                      nbArticles = 0;
                      break;
    }
  }
}

void handlerSIGALRM(int sig)
{
  MESSAGE requete;

  fprintf(stderr,"(CADDIE %d) Time Out !!!\n",getpid());

  // Annulation du caddie et mise à jour de la BD

  // On envoie a AccesBD autant de requetes CANCEL qu'il y a d'articles dans le panier
  for(int i = 0; i < nbArticles; i++)
  {
    requete.expediteur = getpid();
    requete.requete = CANCEL;

    sprintf(requete.data2,"%d", articles[i].stock);
    requete.data1 = articles[i].id;
    
    
    if (write(fdWpipe, &requete, sizeof(MESSAGE)) != sizeof(MESSAGE)) 
    {
      perror("Erreur de write(4) dans le caddie"); 
      sleep(3);
      exit(1); 
    }
  }

  nbArticles = 0;
  
  // Envoi d'un Time Out au client (s'il existe toujours)
  requete.expediteur = getpid();
  requete.type = pidClient;
  requete.requete = TIME_OUT;
  if(msgsnd(idQ, &requete, sizeof(MESSAGE) - sizeof(long),0) == -1)
  {
    perror("Erreur de msgnd dans le handlerAlarm du CADDIE\n");
  }
  kill(pidClient, SIGUSR1); 
  
  close(fdWpipe);
  exit(0);
}