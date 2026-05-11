#include "../include/common.h"
#include <stdlib.h>
#include <time.h>

SOCKET socket_serveur;
int bras_id;
int outil1, outil2;
int outils_possedes = 0;
pthread_mutex_t outils_mutex = PTHREAD_MUTEX_INITIALIZER;

void envoyer_message(const char* message) {
    send(socket_serveur, message, strlen(message), 0);
}

int recevoir_reponse(char* reponse, int taille) {
    memset(reponse, 0, taille);
    int bytes = recv(socket_serveur, reponse, taille - 1, 0);
    if (bytes <= 0) return 0;
    reponse[bytes] = '\0';
    return 1;
}

void* thread_reflexion(void* arg) {
    while (1) {
        int duree = rand() % 5 + 1;
        printf("  Bras %d: Reflexion (%d sec)...\n", bras_id, duree);
        sleep(duree);
    }
    return NULL;
}

void* thread_communication(void* arg) {
    char message[100];
    char reponse[10];
    
    sleep(1);
    
    while (1) {
        pthread_mutex_lock(&outils_mutex);
        int a_outils = outils_possedes;
        pthread_mutex_unlock(&outils_mutex);
        
        if (a_outils == 0) {
            printf("  Bras %d: Demande des outils %d et %d\n", bras_id, outil1, outil2);
            snprintf(message, sizeof(message), "%s %d %d", MSG_DEMANDE_DEUX, outil1, outil2);
            envoyer_message(message);
            
            if (recevoir_reponse(reponse, sizeof(reponse)) && strcmp(reponse, MSG_OK) == 0) {
                printf("  Bras %d: Outils %d et %d obtenus !\n", bras_id, outil1, outil2);
                pthread_mutex_lock(&outils_mutex);
                outils_possedes = 1;
                pthread_mutex_unlock(&outils_mutex);
            } else {
                printf("  Bras %d: Refus des outils, nouvel essai plus tard...\n", bras_id);
                sleep(2);
            }
        }
        sleep(1);
    }
    return NULL;
}

void* thread_assemblage(void* arg) {
    char message[100];
    char reponse[10];
    
    while (1) {
        pthread_mutex_lock(&outils_mutex);
        int a_outils = outils_possedes;
        pthread_mutex_unlock(&outils_mutex);
        
        if (a_outils) {
            printf("  Bras %d: ASSEMBLAGE en cours...\n", bras_id);
            sleep(3);
            printf("  Bras %d: ASSEMBLAGE TERMINE !\n", bras_id);
            
            snprintf(message, sizeof(message), "%s", MSG_LIBERE_DEUX);
            envoyer_message(message);
            recevoir_reponse(reponse, sizeof(reponse));
            
            printf("  Bras %d: Outils liberes\n", bras_id);
            
            pthread_mutex_lock(&outils_mutex);
            outils_possedes = 0;
            pthread_mutex_unlock(&outils_mutex);
            
            printf("  Bras %d: Pause...\n", bras_id);
            sleep(2);
        }
        usleep(100000);
    }
    return NULL;
}

int main(int argc, char* argv[]) {
    if (argc < 4) {
        printf("Usage: %s <id_bras> <outil1> <outil2>\n", argv[0]);
        printf("Exemple: %s 1 1 2\n", argv[0]);
        return 1;
    }
    
    #ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("Erreur WSAStartup\n");
        return 1;
    }
    #endif
    
    bras_id = atoi(argv[1]);
    outil1 = atoi(argv[2]);
    outil2 = atoi(argv[3]);
    srand(time(NULL) + bras_id);
    
    socket_serveur = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_serveur == INVALID_SOCKET) {
        printf("Erreur: Impossible de creer le socket\n");
        #ifdef _WIN32
        WSACleanup();
        #endif
        return 1;
    }
    
    struct sockaddr_in serveur_addr;
    serveur_addr.sin_family = AF_INET;
    serveur_addr.sin_port = htons(PORT);
    serveur_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    
    printf("Connexion au serveur 127.0.0.1:%d...\n", PORT);
    
    if (connect(socket_serveur, (struct sockaddr*)&serveur_addr, sizeof(serveur_addr)) < 0) {
        printf("Erreur: Connexion echouee\n");
        printf("Verifiez que le serveur est lance\n");
        closesocket(socket_serveur);
        #ifdef _WIN32
        WSACleanup();
        #endif
        return 1;
    }
    
    printf("Connecte au serveur !\n");
    
    printf("========================================\n");
    printf("BRAS ROBOTIQUE %d CONNECTE\n", bras_id);
    printf("Outils demandes : %d et %d\n", outil1, outil2);
    printf("========================================\n");
    
    pthread_t t1, t2, t3;
    pthread_create(&t1, NULL, thread_reflexion, NULL);
    pthread_create(&t2, NULL, thread_communication, NULL);
    pthread_create(&t3, NULL, thread_assemblage, NULL);
    
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    pthread_join(t3, NULL);
    
    envoyer_message(MSG_FIN);
    closesocket(socket_serveur);
    
    #ifdef _WIN32
    WSACleanup();
    #endif
    
    return 0;
}
