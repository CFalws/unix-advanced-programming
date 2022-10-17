#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

#define NUM_CHILD   5

// Global variables to be accessed by returnResources().
int shmid;
int *shmaddr;
sem_t *semP;
sem_t *semC;
sem_t *semS;

// Signal handler of SIGINT. It returns shared memory, named semaphores.
void returnResources(int signo) {
    if (signo == SIGINT) {
        if (shmctl(shmid, IPC_RMID, 0) < 0) {
            perror("Failed to delete shared memory");
            exit(-1);
        } else {
            printf("Successfully delete shared memory\n");
        }

        sem_unlink("pSem");
        sem_unlink("cSem");
        sem_unlink("sSem");
        exit(0);
    }
}

int main() {
    key_t key;

    key = ftok("bah1.c", 'R');
    if (key == (key_t)-1) {
        perror("ftok");
        exit(-1);
    }

    // Allocating shared memory for pot.
    shmid = shmget(key, sizeof(int), 0644 | IPC_CREAT | IPC_EXCL);
    if (shmid < 0) {
        perror("shmget");
        exit(-1);
    }

    // Attaching shared memory to the calling process. 
    shmaddr = (int *)shmat(shmid, (void *)NULL, 0);
    if (shmaddr == (int *)-1) {
        perror("shmat");
        exit(-1);
    }

    // Initializing pot as empty. 
    *shmaddr = 0;

    // Allocating named semaphores.
    // “sSem” for synchronizing between child processes.
    // Initialized to 1. Number of instances of pot is just 1. 
    semS = sem_open("sSem", O_CREAT | O_EXCL, 0644, (unsigned int)1);
    if (semS == SEM_FAILED) {
        perror("sem_open");
        exit(-1);
    }

    // “pSem” and “cSem” for synchronizing between child and parent process.
    // Both are initialized to 0. num(sSem) + num(pSem) + num(cSem) = 1.
    semP = sem_open("pSem", O_CREAT | O_EXCL, 0644, (unsigned int)0);
    if (semP == SEM_FAILED) {
        perror("sem_open");
        exit(-1);
    }

    semC = sem_open("cSem", O_CREAT | O_EXCL, 0644, (unsigned int)0);
    if (semC == SEM_FAILED) {
        perror("sem_open");
        exit(-1);
    }

    // Setting signal handler for SIGINT.
    signal(SIGINT, returnResources);

    pid_t childpid;
    for (int i = 0; i < NUM_CHILD; i++) {
        if ((childpid = fork()) < 0) {
            perror("fork");
            exit(-1);
         // Child process.
        } else if (childpid == 0) {
            while (1) {
                 // Only one process can enter. Others wait.
                sem_wait(semS);
                 // Gather one portion of honey.
                *shmaddr += 1;
                printf("Bee ID: %d Amount of honey: %d\n"
                        , (int)getpid(), *shmaddr);
                 // The pot is full. 
                if ((*shmaddr) == 20) {
                // Wakes bear up.
                    sem_post(semC);
                // waits until parent is done with shared memory.
                    sem_wait(semP);
                }

                // Pass shared memory to another child.
                sem_post(semS);
                // Generate random number ranging 0 ~ 900
                int randomNum = rand() % 901;
                // sleep ranomly between 100 ~ 1000 ms 
                usleep(1000 * (randomNum + 100));
            }
        } else {
	     // Parent executes nothing here. Just fork!
            ;
        }
    }

    // When pot is FULL, parent wakes up and takes all the honey. 
    // After changing value, Bear send signal to calling process.
    // Just wait again. 
    while (1) {
        sem_wait(semC);
        *shmaddr = 0;
        printf("The bear eats all the honey\n");
        sem_post(semP);
    }
