# Le maraicher en ligne

L’application réaliser est une application d’achats en ligne de fruits/légumes intitulée « Le 
Maraicher en ligne » en C principalement mais avec une structure en C++ et dans un environnement Unix (Debian dans notre cas) : 
• Les clients de l’application doivent se logger à l’aide d’un couple nom/mot de passe stocké 
dans un fichier binaire. Une fois loggés, ils peuvent naviguer dans le catalogue du magasin en 
ligne et faire leurs achats. Leurs achats sont stockés dans un panier apparaissant dans le bas de 
leur interface graphique. Une publicité apparaît en permanence en rouge dans la zone jaune 
située au milieu de la fenêtre. Cette publicité défile de la droite vers la gauche en permanence 
qu’il y ait un client loggé ou pas. 

• Un gérant pourra également se connecter à l’application afin de modifier son stock de fruits 
et légumes et mettre à jour la publicité.  

• Evidemment, les clients n’accèdent pas eux-mêmes à la base de données. Toutes les actions 
réalisées par un utilisateur connecté provoqueront l’envoi d’une requête au serveur qui la 
traitera.

## Etape 1, 2 et 3 
**- Etape 1 :Connexion/Déconnexion de la fenêtre et login/logout d’un utilisateur**
On vous demande tout d’abord de créer le makefile permettant de réaliser la compilation 
automatisée de tous les exécutables nécessaires (le fichier Compile.sh est fourni).
Etape 1 : Connexion/Déconnexion de la fenêtre et login/logout d’un 
utilisateur 
a) Connexion/Déconnexion d’une fenêtre sur le serveur : Lorsque l’application Client est lancée, elle doit tout d’abord annoncer sa présence au serveur en lui envoyant son PID. Pour cela, avant même d’apparaître, elle envoie une requête CONNECT au serveur.  
b) Login/Logout d’un utilisateur : Un utilisateur peut donc à présent entrer en session en encodant son nom et son mot de passe.
c) A la réception de cette requête, le serveur recherche une ligne vide dans son tableau de 
connexions et l’insère

**- Etape 2 : Affichage de la publicité dans les fenêtres connectées**
Il s’agit à présent de gérer l’affichage d’une publicité au milieu de chaque fenêtre client 
connectée : Il n’est pas nécessaire qu’un client soit en session pour que les publicités apparaissent. Dès la connexion de la fenêtre sur le serveur, celle-ci va recevoir les mises à jour des publicités. 
Pour l’instant, c’est toujours la même publicité qui est affichée. Cependant, elle défile de 
manière circulaire dans la zone de texte prévue à cet effet. Cette zone de texte contient 51 
cases précisément, dès lors, on se limitera à une publicité dont la chaîne de caractères ne 
dépasse pas 50 caractères. Toutes les secondes, la publicité est décalée vers la gauche et le 
caractère (même un espace vide) qui sort par la gauche ré-entre par la droite, il s’agit donc 
d’un décalage avec rotation. 
Les fenêtres clients ne sont pas responsables de la « rotation » de la publicité. Elles doivent 
récupérer la publicité en l’état à l’aide d’une mémoire partagée qui va être une simple 
chaîne de caractères (51 caractères + 1 pour le ‘\0’ = 52)

**- Etape 3: Affichage des articles et premiers accès à la base de données**
	Il s’agit à présent de gérer les premiers accès à la base de données contenant les articles du 
maraicher en ligne, ainsi que leur affichage dans l’interface graphique : Ce n’est pas le processus Serveur qui va accéder directement à la base de données. A chaque client « loggé » va correspondre un processus Caddie qui va (dans un premier temps) gérer l’accès à la base de données (mais également la gestion du panier du client correspondant ; voir étape 5). Dans cette étape, le processus Caddie va se contenter de réaliser la recherche en base de données lorsqu’il reçoit une requête de consultation et transmettre le résultat au processus client correspondant.

## Etape 4 :  Accès unique à la base de données et création d’un pipe 
On se rend rapidement compte que la manière de faire décrite plus haut n’est pas optimale : 
• Il y a autant de connexions à la base de données qu’il y a de processus Caddie en 
cours d’exécution (c’est-à-dire qu’il y a de clients connectés) 
• Lors de futurs achats, il risque d’y avoir des accès concurrents à la base de données 
qui risquent de rendre le stock de marchandises inconsistant. 

L’idée est alors de confier l’accès à la base de données à un seul processus AccesBD. Tous les 
processus Caddie transmettront alors (via un unique pipe de communication) leurs requêtes 
à AccesBD : la connexion à la base de données sera alors unique et il n’y aura plus aucun accès concurrent. Le transfert de données via le pipe ne se fait donc que des processus Caddie vers l’unique processus AccesBD. 

## Etape 5 : Achats d’articles et mise à jour du panier 
Il s’agit ici de gérer l’achat d’articles par le client, Cette étape va se faire en 2 temps : 
• L’envoi d’une requête d’ACHAT au serveur et la mise à jour de la base de données. 
• L’envoi d’une requête CADDIE au serveur afin de mettre à jour l’affichage du caddie 
dans la table de la fenêtre du client.

## Etape 6 : Suppression d’articles, paiement et mise ne place d’un Time Out 
Il s’agit ici tout d'abord de gérer la suppression d’articles du panier par le client ainsi que le paiement : 
• Lors d’un clic sur le bouton « Supprimer article », cela a pour effet de supprimer complètement l’article du panier et remettre à jour la base de données
• Lors d’un clic sur le bouton « Vider le panier », cela a pour effet de vider le panier sans 
payer (on abandonne donc l’achat) et le stock sera remis à jour en base de données
• Lors d’un clic sur le bouton « Payer », cela a pour effet de valider définitivement l’achat des 
articles du panier

> IMPORTANT : Remarquez que si l’utilisateur réalise un LOGOUT ou quitte l’application 
cliente alors que le panier contient des articles, la base de données doit être remise à jour !

Ensuite si le client reste en inactivité trop longtemps (nous dirons 60 secondes), il doit être 
automatiquement deloggé (l’application cliente doit continuer à tourner et permettre un 
nouveau login). De plus, si des articles sont présents dans le panier lors du time out, ils 
doivent être remis en base de données. 
Autre événement qui peut se produire : l’application client plante sans pouvoir envoyer de 
requête au serveur. Il est nécessaire que la base de données soit mise à jour malgré tout. 
Pour toutes ces raisons, le time out ne peut pas être géré au niveau du processus client. Un 
time out doit être mis en place dans le processus Caddie. Si ce processus ne reçoit pas de 
requête pendant plus de 60 secondes, il doit recevoir le signal SIGALRM (utilisation de 
alarm). Attention que dès que le processus Caddie reçoit une nouvelle requête, il doit annuler 
l’alarme (utilisation de alarm(0)).

## Etape 7 : Processus Gérant – Mise en place d’un sémaphore 
Une application « Gérant du Maraicher en ligne » peut être utilisée pour modifier les articles 
de la base de données (prix et stock uniquement) et mettre à jour la publicité qui défile dans 
les interfaces graphiques des clients.

Il s’agit d’une application indépendante qui devra être lancée en ligne de commande. Et qui pourra : 
**• Modifier le prix et/ou du stock des articles de la base de données :** aucun login ni mot de passe n’est nécessaire pour utiliser l’application Gerant. Il suffit de la lancer en ligne de commande. Cette application se connecte directement à la base de données. Une fois démarrée, elle récupère directement l’ensemble des articles de la base de données et les affiches dans la table « Stock » de l’interface graphique.
**• Empêcher les clients de modifier/consulter la base de données si Gérant est actif :** Lorsque le processus Gérant est lancé, on doit empêcher les clients de se logger, de consulter ou de modifier le stock en réalisant de achats. Les clients doivent donc être prévenus que le « serveur est en maintenance ».  Pour cela, nous allons utiliser un sémaphore valant 1 si la base de données est accessible par les clients et 0 sinon (c’est-à-dire si le gérant est actif). C’est le processus Serveur qui doit créer et initialiser correctement le sémaphore. Ce sera également à lui de le supprimer lors de sa fin.
**• Modifier la publicité dans la fenêtre des clients :** La publicité peut être mise à jour par le Gérant à tout moment et s’afficher de manière synchronisée dans toutes les fenêtres clients connectées au serveur (pas nécessairement loggée).

> La gestion du sémaphore dans notre implémentation n'est pas correcte à celle demandée, en effet dans notre solution nous avons implémenté un sémaphore qui permet effectivement au gérant d'être le seul à avoir accès à la BD lorsqu'il est lancé, mais celui-ci autorise également un seul client à un seul client à la fois, ce qui n'est pas vraiment le résultat attendu. De plus, la gestion de la fermeture des différents processus fils de serveur est aussi à revoir (kill et HandlerSIGINT). Si vous avez des améliorations à suggérer, à vous le clavier.