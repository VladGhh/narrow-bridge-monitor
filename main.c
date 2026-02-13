#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "monitor.h"
#include "condition.h"

/* --- CONSTANTE --- */
#define NEUTRU 0
#define NORD   1
#define SUD    2

#define LIMITA 5   // Limita de înfometare
#define CAPACITATE_MAX 3

/* --- STRUCTURA MONITORULUI --- */
typedef struct {
    Monitor mon;
    Condition coada_nord;
    Condition coada_sud;

    // Variabile de stare
    int nr_masini;         // Câți sunt pe pod
    int directie;          // 0=Neutru, 1=Nord, 2=Sud

    // Variabile Starvation
    int consecutive;       // Câte au trecut în direcția curentă
    int asteapta_nord;     // Câți așteaptă la coada Nord
    int asteapta_sud;      // Câți așteaptă la coada Sud
} PodMonitor;

PodMonitor pod;

/* --- 1. Inițializare --- */
void Pod_Init(PodMonitor *p) {
    monitor_init(&p->mon);
    condition_init(&p->coada_nord);
    condition_init(&p->coada_sud);

    p->nr_masini = 0;
    p->directie = NEUTRU;
    p->consecutive = 0;
    p->asteapta_nord = 0;
    p->asteapta_sud = 0;
}

/* --- 2. Proceduri pentru NORD --- */

void Intra_Nord(PodMonitor *p) {
    monitor_enter(&p->mon);

    p->asteapta_nord++; // Mă anunț că am ajuns la barieră

    // Condiția de așteptare din pseudocod:
    // 1. Pod plin
    // 2. Directie opusa (SUD)
    // 3. (Starvation) Directie e NORD + am depasit limita + cineva asteapta la SUD
    while (p->nr_masini == CAPACITATE_MAX ||
           p->directie == SUD ||
           (p->directie == NORD && p->consecutive >= LIMITA && p->asteapta_sud > 0)) {

        condition_wait(&p->coada_nord, &p->mon);
    }

    // Am primit verde!
    p->asteapta_nord--;
    p->nr_masini++;
    p->directie = NORD;
    p->consecutive++;

    printf("[INTRA NORD] Pod: %d | Tura: %d | Waiting Sud: %d\n",
           p->nr_masini, p->consecutive, p->asteapta_sud);

    monitor_exit(&p->mon);
}

void Iese_Nord(PodMonitor *p) {
    monitor_enter(&p->mon);

    p->nr_masini--;
    printf("[IESE  NORD] Pod: %d\n", p->nr_masini);

    if (p->nr_masini == 0) {
        p->directie = NEUTRU;
        p->consecutive = 0;

        // Logica de wake-up din pseudocod:
        // Prioritate celor de la SUD dacă așteaptă
        if (p->asteapta_sud > 0) {
            printf("   -> Pod liber! Prioritate SUD (Broadcast)\n");
            condition_broadcast(&p->coada_sud);
        } else {
            // Dacă nu e nimeni la Sud, dăm voie celor de la Nord
            condition_broadcast(&p->coada_nord);
        }
    } else {
        // Podul nu e gol, dar s-a eliberat un loc.
        // Mai chem un coleg de la NORD doar dacă nu e cazul de cedare (starvation)
        if (p->consecutive < LIMITA || p->asteapta_sud == 0) {
            condition_signal(&p->coada_nord);
        }
    }

    monitor_exit(&p->mon);
}

/* --- 3. Proceduri pentru SUD (Oglindă) --- */

void Intra_Sud(PodMonitor *p) {
    monitor_enter(&p->mon);

    p->asteapta_sud++;

    // Astept daca: plin SAU directie e Nord SAU (directie e Sud dar am depasit limita SI asteapta Nord)
    while (p->nr_masini == CAPACITATE_MAX ||
           p->directie == NORD ||
           (p->directie == SUD && p->consecutive >= LIMITA && p->asteapta_nord > 0)) {

        condition_wait(&p->coada_sud, &p->mon);
    }

    p->asteapta_sud--;
    p->nr_masini++;
    p->directie = SUD;
    p->consecutive++;

    printf("[INTRA SUD ] Pod: %d | Tura: %d | Waiting Nord: %d\n",
           p->nr_masini, p->consecutive, p->asteapta_nord);

    monitor_exit(&p->mon);
}

void Iese_Sud(PodMonitor *p) {
    monitor_enter(&p->mon);

    p->nr_masini--;
    printf("[IESE  SUD ] Pod: %d\n", p->nr_masini);

    if (p->nr_masini == 0) {
        p->directie = NEUTRU;
        p->consecutive = 0;

        // Invers față de Nord: prioritate Nord dacă așteaptă
        if (p->asteapta_nord > 0) {
            printf("   -> Pod liber! Prioritate NORD (Broadcast)\n");
            condition_broadcast(&p->coada_nord);
        } else {
            condition_broadcast(&p->coada_sud);
        }
    } else {
        if (p->consecutive < LIMITA || p->asteapta_nord == 0) {
            condition_signal(&p->coada_sud);
        }
    }

    monitor_exit(&p->mon);
}

/* --- 4. Main pentru Testare --- */

void* car_thread(void* arg) {
    int dir = *(int*)arg;
    free(arg);

    if (dir == NORD) {
        Intra_Nord(&pod);
        usleep(500000); // 0.5 secunde pe pod
        Iese_Nord(&pod);
    } else {
        Intra_Sud(&pod);
        usleep(500000);
        Iese_Sud(&pod);
    }
    return NULL;
}

int main() {
    pthread_t t[20];
    Pod_Init(&pod);

    // SCENARIU TEST:
    // Lansăm 10 mașini NORD rapid
    // După puțin timp lansăm 5 mașini SUD
    // Vedem dacă Nordul se oprește la 5 ca să lase Sudul.

    printf("--- START SIMULARE ---\n");

    for (int i = 0; i < 20; i++) {
        int* dir = malloc(sizeof(int));

        // Primele 8 Nord, apoi 5 Sud, apoi restul Nord
        if (i < 8) *dir = NORD;
        else if (i < 13) *dir = SUD;
        else *dir = NORD;

        if (*dir == NORD) printf("Lansat masina NORD\n");
        else              printf("Lansat masina SUD\n");

        pthread_create(&t[i], NULL, car_thread, dir);

        // Delay mic ca să intre în ordine, dar destul de rapid să creeze coadă
        usleep(100000);
    }

    for (int i = 0; i < 20; i++) {
        pthread_join(t[i], NULL);
    }

    monitor_destroy(&pod.mon);
    condition_destroy(&pod.coada_nord);
    condition_destroy(&pod.coada_sud);
    return 0;
}