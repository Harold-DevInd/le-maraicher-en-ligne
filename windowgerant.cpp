#include "windowgerant.h"
#include "ui_windowgerant.h"
#include <iostream>
using namespace std;
#include <mysql.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/types.h>
#include "protocole.h"

//Fonctions de gestion du semaphore
int sem_wait(int num); 
int sem_signal(int num);

int idArticleSelectionne = -1;
MYSQL *connexion;
MYSQL_RES  *resultat;
MYSQL_ROW  Tuple;
char requete[200];
int idSem;
int idQ;
int nbrArticles;

int nombreDeLigne();
MYSQL_ROW afficherArticles(int);

MESSAGE m;

WindowGerant::WindowGerant(QWidget *parent) : QMainWindow(parent),ui(new Ui::WindowGerant)
{
    ui->setupUi(this);

    // Configuration de la table du stock (ne pas modifer)
    ui->tableWidgetStock->setColumnCount(4);
    ui->tableWidgetStock->setRowCount(0);
    QStringList labelsTableStock;
    labelsTableStock << "Id" << "Article" << "Prix à l'unité" << "Quantité";
    ui->tableWidgetStock->setHorizontalHeaderLabels(labelsTableStock);
    ui->tableWidgetStock->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableWidgetStock->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidgetStock->horizontalHeader()->setVisible(true);
    ui->tableWidgetStock->horizontalHeader()->setDefaultSectionSize(120);
    ui->tableWidgetStock->horizontalHeader()->setStretchLastSection(true);
    ui->tableWidgetStock->verticalHeader()->setVisible(false);
    ui->tableWidgetStock->horizontalHeader()->setStyleSheet("background-color: lightyellow");

    // Recuperation de la file de message
    fprintf(stderr,"(GERANT %d) Recuperation de l'id de la file de messages\n",getpid());
    if((idQ = msgget(CLE,0)) == -1)
    {
      perror("(GERANT) Erreur de msgget");
      exit(1);
    }

    // Récupération du sémaphore
    if ((idSem = semget(CLE,0,0)) == -1)
    {
      perror("Erreur de semget");
      exit(1);
    }
    fprintf(stderr, "idSem = %d\n",idSem);

    // Prise blocante du semaphore
    sem_wait(0);

    // Connexion à la base de donnée
    connexion = mysql_init(NULL);
    fprintf(stderr,"(GERANT %d) Connexion à la BD\n",getpid());
    if (mysql_real_connect(connexion,"localhost","Student","PassStudent1_","PourStudent",0,0,0) == NULL)
    {
      fprintf(stderr,"(GERANT %d) Erreur de connexion à la base de données...\n",getpid());
      exit(1);  
    }

    // Recuperation des articles en BD et affichage
    // Construction de la requête SELECT
    fprintf(stderr,"(GERANT %d) Recuperation des articles la base de données...\n",getpid());
    nbrArticles = nombreDeLigne();
    for(int i = 1; i <= nbrArticles; i++)
    {
      Tuple = afficherArticles(i);
      if (Tuple != NULL) 
      {
        ajouteArticleTablePanier(atoi(Tuple[0]), Tuple[1], atof(Tuple[2]), atoi(Tuple[3]));
      }
    }
}

WindowGerant::~WindowGerant()
{
    delete ui;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// Fonctions utiles Table du stock (ne pas modifier) //////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowGerant::ajouteArticleTablePanier(int id,const char* article,float prix,int quantite)
{
    char Id[20],Prix[20],Quantite[20];

    sprintf(Id,"%d",id);
    sprintf(Prix,"%.2f",prix);
    sprintf(Quantite,"%d",quantite);

    // Ajout possible
    int nbLignes = ui->tableWidgetStock->rowCount();
    nbLignes++;
    ui->tableWidgetStock->setRowCount(nbLignes);
    ui->tableWidgetStock->setRowHeight(nbLignes-1,10);

    QTableWidgetItem *item = new QTableWidgetItem;
    item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
    item->setTextAlignment(Qt::AlignCenter);
    item->setText(Id);
    ui->tableWidgetStock->setItem(nbLignes-1,0,item);

    item = new QTableWidgetItem;
    item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
    item->setTextAlignment(Qt::AlignCenter);
    item->setText(article);
    ui->tableWidgetStock->setItem(nbLignes-1,1,item);

    item = new QTableWidgetItem;
    item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
    item->setTextAlignment(Qt::AlignCenter);
    item->setText(Prix);
    ui->tableWidgetStock->setItem(nbLignes-1,2,item);

    item = new QTableWidgetItem;
    item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
    item->setTextAlignment(Qt::AlignCenter);
    item->setText(Quantite);
    ui->tableWidgetStock->setItem(nbLignes-1,3,item);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowGerant::videTableStock()
{
    ui->tableWidgetStock->setRowCount(0);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
int WindowGerant::getIndiceArticleSelectionne()
{
    QModelIndexList liste = ui->tableWidgetStock->selectionModel()->selectedRows();
    if (liste.size() == 0) return -1;
    QModelIndex index = liste.at(0);
    int indice = index.row();
    return indice;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowGerant::on_tableWidgetStock_cellClicked(int row, int column)
{
    //cerr << "ligne=" << row << " colonne=" << column << endl;
    ui->lineEditIntitule->setText(ui->tableWidgetStock->item(row,1)->text());
    ui->lineEditPrix->setText(ui->tableWidgetStock->item(row,2)->text());
    ui->lineEditStock->setText(ui->tableWidgetStock->item(row,3)->text());
    idArticleSelectionne = atoi(ui->tableWidgetStock->item(row,0)->text().toStdString().c_str());
    //cerr << "id = " << idArticleSelectionne << endl;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// Fonctions utiles : ne pas modifier /////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
float WindowGerant::getPrix()
{
    return atof(ui->lineEditPrix->text().toStdString().c_str());
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
int WindowGerant::getStock()
{
    return atoi(ui->lineEditStock->text().toStdString().c_str());
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
const char* WindowGerant::getPublicite()
{
  strcpy(publicite,ui->lineEditPublicite->text().toStdString().c_str());
  return publicite;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////// CLIC SUR LA CROIX DE LA FENETRE /////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowGerant::closeEvent(QCloseEvent *event)
{
  fprintf(stderr,"(GERANT %d) Clic sur croix de la fenetre\n",getpid());
  // TO DO
  // Deconnexion BD
  mysql_close(connexion);

  // Liberation du semaphore
  sem_signal(0),

  exit(0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// Fonctions clics sur les boutons ////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowGerant::on_pushButtonPublicite_clicked()
{
  fprintf(stderr,"(GERANT %d) Clic sur bouton Mettre a jour\n",getpid());
  // TO DO (étape 7)
  // Envoi d'une requete NEW_PUB au serveur
  m.type = 1;
  m.expediteur = getpid();
  m.requete = NEW_PUB;
  strcpy(m.data4, getPublicite());

  if(msgsnd(idQ, &m, sizeof(MESSAGE) - sizeof(long), 0) == -1)
  {
    perror("Erreur de msgnd NEW_PUB du gerant\n");
    exit(1);
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowGerant::on_pushButtonModifier_clicked()
{
  fprintf(stderr,"(GERANT %d) Clic sur bouton Modifier\n",getpid());
  // TO DO
  //cerr << "Prix  : --"  << getPrix() << "--" << endl;
  //cerr << "Stock : --"  << getStock() << "--" << endl;

  char Prix[20];
  sprintf(Prix,"%f",getPrix());
  string tmp(Prix);
  size_t x = tmp.find(",");
  if (x != string::npos) tmp.replace(x,1,".");

  fprintf(stderr,"(GERANT %d) Modification en base de données pour id=%d\n",getpid(),idArticleSelectionne);

  // Mise a jour table BD
  //Mise à jour du stock
  sprintf(requete, "UPDATE UNIX_FINAL SET stock = %d WHERE id = %d;", getStock(), idArticleSelectionne);
  fprintf(stderr, "---->%s\n", requete);

  if (mysql_query(connexion, requete)) 
  {
      fprintf(stderr, "Erreur lors de la requête UPDATE du stock : %s\n", mysql_error(connexion));
      mysql_close(connexion);
      exit(1);
  }
  //Mise à jour du prix
  sprintf(requete, "UPDATE UNIX_FINAL SET prix = %d WHERE id = %d;", stoi(tmp), idArticleSelectionne);
  fprintf(stderr, "---->%s\n", requete);

  if (mysql_query(connexion, requete)) 
  {
      fprintf(stderr, "Erreur lors de la requête UPDATE du prix : %s\n", mysql_error(connexion));
      mysql_close(connexion);
      exit(1);
  }

  // Recuperation des articles en BD et affichage
  videTableStock();
  fprintf(stderr,"(GERANT %d) Mise a jour des articles dans la base de données...\n",getpid());
  nbrArticles = nombreDeLigne();
  for(int i = 1; i <= nbrArticles; i++)
  {
    Tuple = afficherArticles(i);
    if (Tuple != NULL) 
    {
      ajouteArticleTablePanier(atoi(Tuple[0]), Tuple[1], atof(Tuple[2]), atoi(Tuple[3]));
    }
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
MYSQL_ROW afficherArticles(int nA)
{
  MYSQL_ROW result;

  sprintf(requete, "SELECT * FROM UNIX_FINAL WHERE id = %d;", nA);

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

  if ((result = mysql_fetch_row(resultat))) 
  {
    return result;
  }

  return NULL;
}

int nombreDeLigne()
{
  // Construction de la requête COUNT
  sprintf(requete, "SELECT COUNT(*) FROM UNIX_FINAL;");

  // Exécution de la requête
  if (mysql_query(connexion, requete)) {
      fprintf(stderr, "Erreur lors de la requête COUNT : %s\n", mysql_error(connexion));
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

  // Retourné le résultat
  if ((Tuple = mysql_fetch_row(resultat))) {
      printf("Nombre d'éléments dans UNIX_FINAL : %d\n", atoi(Tuple[0]));
      return atoi(Tuple[0]);
  }
  
  return -1;
}

// Fonction pour manipuler le semaphore 
// un sémaphore valant 1 si la base de données est accessible par les clients et 0 sinon 

//Passe le semaphore à 0 donc le recupere (c’est-à-dire si le gérant est actif)
int sem_wait(int num)
{
  struct sembuf action;
  action.sem_num = num;
  action.sem_op = -1; // pour retirer 1 au semaphore
  action.sem_flg = SEM_UNDO;
  int semVal = semop(idSem,&action,1);
  fprintf(stderr,"(GARANT %d) Prend la main, semaphore : %d\n",getpid(), semVal);
  return semVal;
}

//Passe la semaphore a 1 donc le libere (c’est-à-dire si le gérant n est plus actif)
int sem_signal(int num)
{
  struct sembuf action;
  action.sem_num = num;
  action.sem_op = 1; // pour ajouter 1 au semaphores
  action.sem_flg = SEM_UNDO;
  int semVal = semop(idSem,&action,1);
  fprintf(stderr,"(GARANT %d) Libere la main, semaphore :  %d\n",getpid(), semVal);
  return semVal; 
}