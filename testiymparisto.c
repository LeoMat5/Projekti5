#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/sysinfo.h>

// Globaalit
// pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
// pthread_cond_t c = PTHREAD_COND_INITIALIZER;

////////////////////////////////////////////////////////////////
//////////////////////////// HUOM! /////////////////////////////
////////////////////////////////////////////////////////////////
// TÄTÄ TESTIYMPÄRISTÖÄ KÄYTETÄÄN VAIN TEORIASSA OPETETTUJEN  //
// MINULLE C-KIELESSÄ UUSIEN ASIOIDEN TESTAAMISEEN.           //
// TÄMÄ EI OLE TARKOITETTU ARVIOITAVAKSI!                     //
////////////////////////////////////////////////////////////////

// https://pages.cs.wisc.edu/%7Eremzi/OSTEP/threads-intro.pdf
// https://pages.cs.wisc.edu/%7Eremzi/OSTEP/threads-cv.pdf
// https://pages.cs.wisc.edu/%7Eremzi/OSTEP/threads-api.pdf


// void *tWorker(void *arg) {
//     printf("Worker createted\n");

//     for (int i = 0; i < 5; i++) {
//         printf("Worker is working");
//     }

//     printf("Worker has completed its work");

//     thr_exit();
//     return NULL;
// }

void *tWorker(void *arg) { // Void pointteri
    int iWorkerID = *(int *) arg; // https://pages.cs.wisc.edu/%7Eremzi/OSTEP/threads-api.pdf sivu 3
    printf("Worker %d started to work\n", iWorkerID);
    return(NULL);
}

int main(int argc, char *argv[]) {
    printf("\n//////////////// NEW PROGRAM RUN ////////////////\n");
    // Alustus //
    int iAmountOfAvailableProcessors = get_nprocs();
    int iAmountOfWorkers = 5; // Tai iAmountOfAvailableProcessors 
    int iWorkerID[iAmountOfWorkers];

    // pthread_t tQueen;
    pthread_t workers[iAmountOfWorkers];

    for (int i = 0; i < iAmountOfWorkers; i++) { // Luodaan työntekijöille threadit
        iWorkerID[i] = i;
        pthread_create(&workers[i], NULL, tWorker, &iWorkerID[i]);
    }

    for (int i = 0; i < iAmountOfWorkers; i++) { // Yhdistetään työntekijöiden tulokset (odottava toiminnallisuus) https://pages.cs.wisc.edu/%7Eremzi/OSTEP/threads-intro.pdf sivu 2
        pthread_join(workers[i], NULL);
    }


    // Testitulosteet //
    printf("The amount of available processors in the system is: %d\n", iAmountOfAvailableProcessors);
    printf("\n////////////// END OF PROGRAM RUN //////////////\n\n");
    return(0);
}