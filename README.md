# Le maraicher en ligne

L’application à réaliser est une application d’achats en ligne de fruits/légumes intitulée « Le 
Maraicher en ligne » : 
- Les clients de l’application doivent se logger à l’aide d’un couple nom/mot de passe stocké 
dans un fichier binaire. Une fois loggés, ils peuvent naviguer dans le catalogue du magasin en 
ligne et faire leurs achats. Leurs achats sont stockés dans un panier apparaissant dans le bas de 
leur interface graphique. Une publicité apparaît en permanence en rouge dans la zone jaune 
située au milieu de la fenêtre. Cette publicité défile de la droite vers la gauche en permanence 
qu’il y ait un client loggé ou pas. 

- Un gérant pourra également se connecter à l’application afin de modifier son stock de fruits 
et légumes et mettre à jour la publicité.  

- Evidemment, les clients n’accèdent pas eux-mêmes à la base de données. Toutes les actions 
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

##Etape 4 :  Accès unique à la base de données et création d’un pipe 
On se rend rapidement compte que la manière de faire décrite plus haut n’est pas optimale : 
• Il y a autant de connexions à la base de données qu’il y a de processus Caddie en 
cours d’exécution (c’est-à-dire qu’il y a de clients connectés) 
• Lors de futurs achats, il risque d’y avoir des accès concurrents à la base de données 
qui risquent de rendre le stock de marchandises inconsistant. 

L’idée est alors de confier l’accès à la base de données à un seul processus AccesBD. Tous les 
processus Caddie transmettront alors (via un unique pipe de communication) leurs requêtes 
à AccesBD : la connexion à la base de données sera alors unique et il n’y aura plus aucun accès concurrent. Le transfert de données via le pipe ne se fait donc que des processus Caddie vers l’unique processus AccesBD. 