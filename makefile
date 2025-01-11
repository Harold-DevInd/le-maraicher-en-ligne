.SILENT:

OBJS1=maingerant.o windowgerant.o moc_windowgerant.o
OBJS2=mainclient.o windowclient.o moc_windowclient.o
PROGRAM=Client Gerant Serveur CreationBD Publicite Caddie AccesBD
LIBG1=-Wno-unused-parameter -c -pipe -g -std=gnu++11 -Wall -W -D_REENTRANT -fPIC -DQT_DEPRECATED_WARNINGS -DQT_QML_DEBUG -DQT_WIDGETS_LIB -DQT_GUI_LIB -DQT_CORE_LIB 
LIBG2=-I. -isystem /usr/include/qt5 -isystem /usr/include/qt5/QtWidgets -isystem /usr/include/qt5/QtGui -isystem /usr/include/qt5/QtCore -I. -I. -I/usr/lib64/qt5/mkspecs/linux-g++
LIBQT1=/usr/lib64/libQt5Widgets.so /usr/lib64/libQt5Gui.so /usr/lib64/libQt5Core.so /usr/lib64/libGL.so -L/usr/lib64/mysql -lmysqlclient -lpthread -lz -lm -lrt -lssl -lcrypto -ldl
LIBQT2=/usr/lib64/libQt5Widgets.so /usr/lib64/libQt5Gui.so /usr/lib64/libQt5Core.so /usr/lib64/libGL.so -lpthread
LIBMSQL=-I/usr/include/mysql -m64 -L/usr/lib64/mysql -lmysqlclient -lpthread -lz -lm -lrt -lssl -lcrypto -ldl

all: $(PROGRAM)

Gerant:	$(OBJS1)
	echo "Compilation de l' executable Gerant"
	g++ -Wno-unused-parameter -o Gerant $(OBJS1) $(LIBQT1)

maingerant.o:	maingerant.cpp
	echo "Compilation du maingerant.o"
	g++ $(LIBG1) -I../Administrateur $(LIBG2) -o maingerant.o maingerant.cpp

windowgerant.o:	windowgerant.cpp
	echo "Compilation du windowgerant.o"
	g++ $(LIBG1) -I../Administrateur $(LIBG2) -I/usr/include/mysql -m64 -L/usr/lib64/mysql -o windowgerant.o windowgerant.cpp

moc_windowgerant.o:	moc_windowgerant.cpp
	echo "Compilation du moc_windowgerant.o"
	g++ $(LIBG1) -I../Administrateur $(LIBG2) -o moc_windowgerant.o moc_windowgerant.cpp

Client:	$(OBJS2)
	echo "Compilation de l' executable Client"
	g++ -Wno-unused-parameter -o Client $(OBJS2) $(LIBQT2)

mainclient.o:	mainclient.cpp
	echo "Compilation du mainclient.o"
	g++ $(LIBG1) -I../UNIX_DOSSIER_FINAL $(LIBG2) -o mainclient.o mainclient.cpp

windowclient.o:	windowclient.cpp
	echo "Compilation du windowclient.o"
	g++ $(LIBG1) -I../UNIX_DOSSIER_FINAL $(LIBG2) -o windowclient.o windowclient.cpp

moc_windowclient.o: moc_windowclient.cpp
	echo "Compilation du moc_windowclient.o"
	g++ $(LIBG1) -I../UNIX_DOSSIER_FINAL $(LIBG2) -o moc_windowclient.o moc_windowclient.cpp

Serveur:	Serveur.cpp
	echo "Compilation de l' executable Serveur"
	g++ Serveur.cpp -o Serveur $(LIBMSQL)

CreationBD:	CreationBD.cpp
	echo "Compilation de l' executable CreationBD"
	g++ -o CreationBD CreationBD.cpp $(LIBMSQL)

Publicite:	Publicite.cpp
	echo "Compilation de l' executable Publicite"
	g++ -o Publicite Publicite.cpp

Caddie:	Caddie.cpp
	echo "Compilation de l' executable Caddie"
	g++ -o Caddie Caddie.cpp $(LIBMSQL)

AccesBD:	AccesBD.cpp
	echo "Compilation de l' executable AccesBD"
	g++ -o AccesBD AccesBD.cpp $(LIBMSQL)

clean:
	echo "Suppression des fichiers objets suivant : $(OBJS1) $(OBJS2) "
	rm -f $(OBJS1) $(OBJS2)

clobber: clean
	echo "Suppression de tous les programmes obtenues apres compilation : $(PROGRAM)"
	rm -f $(PROGRAM)