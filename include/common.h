#ifndef COMMUN_H
#define COMMUN_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <pthread.h>
#define SOCKET int
#define INVALID_SOCKET -1
#define closesocket close
#endif
#include <pthread.h>
#define PORT 8888
#define MAX_CLIENTS 10
#define NB_OUTILS 4
#define TAILLE_BUFFER 256
#define MSG_DEMANDE_UN    "DEMANDE_UN"
#define MSG_DEMANDE_DEUX  "DEMANDE_DEUX"
#define MSG_LIBERE_UN     "LIBERE_UN"
#define MSG_LIBERE_DEUX   "LIBERE_DEUX"
#define MSG_OK            "OK"
#define MSG_NON           "NON"
#define MSG_FIN           "FIN"

typedef struct {
    int id;
    int occupe;
    int proprietaire;
    pthread_mutex_t mutex;
} Outil;

typedef struct {
    SOCKET socket;
    int id_bras;
    int outil1;
    int outil2;
    int outils_obtenus;
} Bras;

#endif
