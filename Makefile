all: serveur client

serveur: serveur/gestionnaire_outils.c include/common.h
	gcc -Wall -Wextra -std=c99 -pthread -Iinclude -o serveur serveur/gestionnaire_outils.c

client: client/bras_robotique.c include/common.h
	gcc -Wall -Wextra -std=c99 -pthread -Iinclude -o client client/bras_robotique.c

run-serveur: serveur
	./serveur

run-client1: client
	./client 1

run-client2: client
	./client 2

run-client3: client
	./client 3

clean:
	rm -f serveur client serveur.log

.PHONY: all serveur client run-serveur run-client1 run-client2 run-client3 clean