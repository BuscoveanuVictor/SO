#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>
#include <errno.h>

sem_t doctor_sem;

typedef struct {
    int id;         
    bool ocupat;     
} Doctor;

typedef struct {
    int id;
    int sosire;
    int asteptare;
    int consulta;
    int doctor;
} Pacient;

typedef struct {
    Doctor *doctor;
    int disponibilitate;
    int limita_timp;    
} ListaDoctori;

// Parametrii pentru fiecare pacient - definim tipul args
typedef struct {
    Pacient pacient;
    ListaDoctori *lista_doctori;
    pthread_mutex_t *doctor_mutex;
    sem_t *doctor_sem;
} args;


    
void *patient_thread(void *arg) {
    args *p = (args *)arg;
    
    // Se 'blocheaza functia' pana la sosirea pacientului
    sleep(p->pacient.sosire);
    printf("Pacientul %d a sosit la momentul %d - asteapta.\n", p->pacient.id, p->pacient.sosire);

    // Salvez timpul de inceput al asteptarii
    time_t start_wait = time(NULL);

    // Asteptam pana cand un doctor devine disponibil
    sem_wait(p->doctor_sem);
    // Gata s-a eliberat un doctor disponibil, trebuie sa ma duc sa-l gasesc

     
    //Acum blochez accesul la lista de doctori pentru a-l cauta pe cel disponibil
    pthread_mutex_lock(p->doctor_mutex);

    //Caut sa gasesc un doctor liber
    int doctor_id = -1;
    for (int i = 0; i < p->lista_doctori->disponibilitate; i++) {
        if (!p->lista_doctori->doctor[i].ocupat) { 
            doctor_id = p->lista_doctori->doctor[i].id;
            p->lista_doctori->doctor[i].ocupat = true;
            break;
        }
    }

    // Deblochez accesul la lista de doctori
    pthread_mutex_unlock(p->doctor_mutex);

    // Inregistrez timpul de final al asteptarii
    time_t end_wait = time(NULL);

    // Calculez timpul de asteptare al pacientului
    int waiting_time = (int)(end_wait - start_wait);

    // Simulez timpul de consultare
    int consult_time = rand() % p->lista_doctori->limita_timp + 1;
    printf("Pacientul %d este consultat de doctorul %d pentru %d minute.\n", p->pacient.id, doctor_id, consult_time);
    sleep(consult_time);

    // Dupa ce am terminat de consultat eliberez doctorul
    pthread_mutex_lock(p->doctor_mutex);
    for (int i = 0; i < p->lista_doctori->disponibilitate; i++) {
        if (p->lista_doctori->doctor[i].id == doctor_id) {
            p->lista_doctori->doctor[i].ocupat = false;
            break;
        }
    }
    pthread_mutex_unlock(p->doctor_mutex);

    // Semnalez ca un doctor este liber
    sem_post(&doctor_sem);

    int *result = malloc(sizeof(int));
    *result = waiting_time;
    return (void*)result;
}

int main() {
    int doctori_n, pacienti_n, timp_maxim, timp_simulare;
    pthread_mutex_t doctor_mutex;
    
 
    printf("Introduceti numarul de doctori: ");
    scanf("%d", &doctori_n);
    printf("Introduceti numarul de pacienti: ");
    scanf("%d", &pacienti_n);
    printf("Introduceti timpul maxim de consultatie: ");
    scanf("%d", &timp_maxim);
    printf("Introduceti momentul maxim de sosire: ");
    scanf("%d", &timp_simulare);

    // Initializam semaforul si mutexul
    if (sem_init(&doctor_sem, 0, doctori_n) != 0) {
        perror ( NULL );
        return errno;
    }
    if (pthread_mutex_init(&doctor_mutex, NULL) != 0) {
        perror ( NULL );
        return errno ;
    }

    // Initializam lista de doctori
    ListaDoctori *lista_doctori = malloc(sizeof(ListaDoctori));
    lista_doctori->doctor = malloc((doctori_n + 1) * sizeof(Doctor));
    lista_doctori->disponibilitate = doctori_n;
    lista_doctori->limita_timp = timp_maxim;

    // Initializam doctorii
    for (int i = 0; i < doctori_n; i++) {
        lista_doctori->doctor[i].id = i + 1;
        lista_doctori->doctor[i].ocupat = false;
    }

    // Cream thread urile(pacientii)
    pthread_t *pacienti = malloc(pacienti_n * sizeof(pthread_t));
    args *args = malloc(pacienti_n * sizeof(args));

    // Initializam pacientii
    srand(time(NULL));
    for (int i = 0; i < pacienti_n; i++) {
        args[i].pacient.id = i + 1;
        args[i].pacient.sosire = rand() % timp_simulare;
        args[i].pacient.consulta = rand() % timp_maxim;
        args[i].pacient.doctor = -1;
        args[i].lista_doctori = lista_doctori;
        args[i].doctor_mutex = &doctor_mutex;
        args[i].doctor_sem = &doctor_sem;
        
        pthread_create(&pacienti[i], NULL, patient_thread, &args[i]);
    }

    int timp_asteptare_pacienti[pacienti_n];
    void *rezultat;
    // Asteptam terminarea pacientilor
    for (int i = 0; i < pacienti_n; i++) {
        pthread_join(pacienti[i], &rezultat);
        timp_asteptare_pacienti[i] = *(int*)rezultat;
        free(rezultat);
    }


    // // Mai intai eliberam resursele de sincronizare
    sem_destroy(&doctor_sem);
    pthread_mutex_destroy(&doctor_mutex);

    // Apoi eliberam memoria in ordine inversa alocarii
    free(lista_doctori->doctor);
    free(lista_doctori);
    free(args);
    free(pacienti);

    return 0;
}
