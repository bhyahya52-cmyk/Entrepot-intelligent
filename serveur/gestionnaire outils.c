#include "../include/common.h"
#include <time.h>

Outil outils[NB_OUTILS];
pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
FILE* log_fichier;

void ecrire_log(const char* message) {
    pthread_mutex_lock(&log_mutex);
    time_t t = time(NULL);
    struct tm* tm = localtime(&t);
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%H:%M:%S", tm);
    fprintf(log_fichier, "[%s] %s\n", timestamp, message);
    fflush(log_fichier);
    pthread_mutex_unlock(&log_mutex);
}

void init_outils(void) {
    for (int i = 0; i < NB_OUTILS; i++) {
        outils[i].id = i + 1;
        outils[i].occupe = 0;
        outils[i].proprietaire = -1;
        pthread_mutex_init(&outils[i].mutex, NULL);
    }
    ecrire_log("Serveur demarre - Outils initialises");
}

void afficher_etat_outils(void) {
    char buf[256];
    int pos = 0;
    pos += snprintf(buf + pos, sizeof(buf) - pos, "ETAT -> ");
    for (int i = 0; i < NB_OUTILS; i++) {
        pthread_mutex_lock(&outils[i].mutex);
        if (outils[i].occupe)
            pos += snprintf(buf + pos, sizeof(buf) - pos,
                            "[O%d:B%d] ", outils[i].id, outils[i].proprietaire);
        else
            pos += snprintf(buf + pos, sizeof(buf) - pos, "[O%d:LIBRE] ", outils[i].id);
        pthread_mutex_unlock(&outils[i].mutex);
    }
    ecrire_log(buf);
}

int demander_deux_outils(int id1, int id2, int bras_id) {
    if (id1 > id2) {
        int tmp = id1;
        id1 = id2;
        id2 = tmp;
    }
    
    if (id1 < 1 || id2 > NB_OUTILS || id1 == id2) return 0;
    
    Outil* o1 = &outils[id1 - 1];
    Outil* o2 = &outils[id2 - 1];
    int accorde = 0;
    
    pthread_mutex_lock(&o1->mutex);
    pthread_mutex_lock(&o2->mutex);
    
    if (o1->occupe == 0 && o2->occupe == 0) {
        o1->occupe = 1;
        o1->proprietaire = bras_id;
        o2->occupe = 1;
        o2->proprietaire = bras_id;
        accorde = 1;
    }
    
    pthread_mutex_unlock(&o2->mutex);
    pthread_mutex_unlock(&o1->mutex);
    
    char log[256];
    if (accorde) {
        snprintf(log, sizeof(log), "Bras %d : OUTILS %d+%d ACCORDES", bras_id, id1, id2);
    } else {
        snprintf(log, sizeof(log), "Bras %d : OUTILS %d+%d REFUSES", bras_id, id1, id2);
    }
    ecrire_log(log);
    afficher_etat_outils();
    
    return accorde;
}

int liberer_deux_outils(int id1, int id2, int bras_id) {
    if (id1 > id2) {
        int tmp = id1;
        id1 = id2;
        id2 = tmp;
    }
    
    Outil* o1 = &outils[id1 - 1];
    Outil* o2 = &outils[id2 - 1];
    
    pthread_mutex_lock(&o1->mutex);
    pthread_mutex_lock(&o2->mutex);
    
    if (o1->proprietaire == bras_id) {
        o1->occupe = 0;
        o1->proprietaire = -1;
    }
    if (o2->proprietaire == bras_id) {
        o2->occupe = 0;
        o2->proprietaire = -1;
    }
    
    pthread_mutex_unlock(&o2->mutex);
    pthread_mutex_unlock(&o1->mutex);
    
    char log[256];
    snprintf(log, sizeof(log), "Bras %d : OUTILS %d+%d LIBERES", bras_id, id1, id2);
    ecrire_log(log);
    afficher_etat_outils();
    
    return 1;
}

void* gerer_bras(void* arg) {
    Bras* bras = (Bras*)arg;
    char buffer[TAILLE_BUFFER];
    char commande[50];
    int id1, id2;
    
    char log[200];
    snprintf(log, sizeof(log), "Bras %d CONNECTE", bras->id_bras);
    ecrire_log(log);
    
    while (1) {
        memset(buffer, 0, TAILLE_BUFFER);
        int bytes = recv(bras->socket, buffer, TAILLE_BUFFER - 1, 0);
        if (bytes <= 0) {
            snprintf(log, sizeof(log), "Bras %d DECONNECTE", bras->id_bras);
            ecrire_log(log);
            break;
        }
        buffer[bytes] = '\0';
        
        memset(commande, 0, sizeof(commande));
        id1 = id2 = 0;
        sscanf(buffer, "%49s %d %d", commande, &id1, &id2);
        
        if (strcmp(commande, MSG_DEMANDE_DEUX) == 0) {
            int ok = demander_deux_outils(id1, id2, bras->id_bras);
            const char* reponse = ok ? MSG_OK : MSG_NON;
            send(bras->socket, reponse, strlen(reponse), 0);
            if (ok) {
                bras->outil1 = id1;
                bras->outil2 = id2;
                bras->outils_obtenus = 1;
            }
        }
        else if (strcmp(commande, MSG_LIBERE_DEUX) == 0) {
            liberer_deux_outils(bras->outil1, bras->outil2, bras->id_bras);
            bras->outils_obtenus = 0;
            send(bras->socket, MSG_OK, strlen(MSG_OK), 0);
        }
        else if (strcmp(commande, MSG_FIN) == 0) {
            if (bras->outils_obtenus) {
                liberer_deux_outils(bras->outil1, bras->outil2, bras->id_bras);
                bras->outils_obtenus = 0;
            }
            break;
        }
    }
    
    closesocket(bras->socket);
    free(bras);
    return NULL;
}

int main(void) {
    #ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("Erreur WSAStartup\n");
        return 1;
    }
    #endif
    
    SOCKET serveur_fd, client_fd;
    struct sockaddr_in serveur_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    
    init_outils();
    
    serveur_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (serveur_fd == INVALID_SOCKET) {
        perror("Erreur socket");
        return 1;
    }
    
    int opt = 1;
    setsockopt(serveur_fd, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));
    
    memset(&serveur_addr, 0, sizeof(serveur_addr));
    serveur_addr.sin_family = AF_INET;
    serveur_addr.sin_addr.s_addr = INADDR_ANY;
    serveur_addr.sin_port = htons(PORT);
    
    if (bind(serveur_fd, (struct sockaddr*)&serveur_addr, sizeof(serveur_addr)) < 0) {
        perror("Erreur bind");
        closesocket(serveur_fd);
        return 1;
    }
    
    if (listen(serveur_fd, MAX_CLIENTS) < 0) {
        perror("Erreur listen");
        closesocket(serveur_fd);
        return 1;
    }
    
    log_fichier = fopen("serveur.log", "w");
    if (!log_fichier) {
        perror("Erreur ouverture log");
        return 1;
    }
    
    printf("========================================\n");
    printf("  SERVEUR DE GESTION D'OUTILS\n");
    printf("  Port : %d  |  Outils : %d\n", PORT, NB_OUTILS);
    printf("  Anti-deadlock : ordre fixe\n");
    printf("========================================\n");
    ecrire_log("Serveur en ecoute...");
    
    int id_suivant = 1;
    
    while (1) {
        client_fd = accept(serveur_fd, (struct sockaddr*)&client_addr, &addr_len);
        if (client_fd == INVALID_SOCKET) {
            perror("Erreur accept");
            continue;
        }
        
        Bras* bras = (Bras*)malloc(sizeof(Bras));
        bras->socket = client_fd;
        bras->id_bras = id_suivant++;
        bras->outil1 = -1;
        bras->outil2 = -1;
        bras->outils_obtenus = 0;
        
        pthread_t tid;
        pthread_create(&tid, NULL, gerer_bras, bras);
        pthread_detach(tid);
    }
    
    fclose(log_fichier);
    closesocket(serveur_fd);
    
    #ifdef _WIN32
    WSACleanup();
    #endif
    
    return 0;
}
