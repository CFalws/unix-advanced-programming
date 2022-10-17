#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

// this structure is for balance, condition variable and mutex.
typedef struct {

    int balance;
    pthread_mutex_t *lock;
    pthread_cond_t *isEnough;

} Balance;

// pointer to Balance instance is located in datasegment.
Balance *myBalance;
int retVal;

// initializing Balance structure
void balanceInit(void) {

    // all the data of Balance instance is located in heap.
    myBalance = (Balance *)malloc(sizeof(Balance));

    myBalance->balance = 0;

    myBalance->lock = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));

    retVal = pthread_mutex_init(myBalance->lock, NULL);
    if (retVal) {
        fprintf(stderr, "pthread_mutex_init error\n");
        exit(-1);
    }

    myBalance->isEnough = (pthread_cond_t *)malloc(sizeof(pthread_cond_t));

    retVal = pthread_cond_init(myBalance->isEnough, NULL);
    if (retVal) {
        fprintf(stderr, "pthread_cond_init error\n");
        exit(-1);
    }
}

// returning all the resources as mutex, conditional var, myBalance…
// destroy should be executed before calling free().
void balanceDelete(void) {
    retVal = pthread_mutex_destroy(myBalance->lock);
    if (retVal) {
        fprintf(stderr, "pthread_mutex_destroy error\n");
        exit(-1);
    }

    free(myBalance->lock);

    retVal = pthread_cond_destroy(myBalance->isEnough);
    if (retVal) {
        fprintf(stderr, "pthread_cond_destroy error\n");
        exit(-1);
    }

    free(myBalance->isEnough);

    free(myBalance);
}

// thread for deposit
void *depositThread(void *arg) {
    while (1) {
     
        // making random number between 1 ~ 10.
        int amount = rand() % 10 + 1;
	 
        // acquire lock for accessing shared resource.
        pthread_mutex_lock(myBalance->lock);

        myBalance->balance += amount;

        printf("deposit %d\t\t\t\t\t%d\n", amount, myBalance->balance);
  
        // pthread_cond_signal is ok concerning the relative location with
      	// pthread_mutex_unlock
        pthread_cond_signal(myBalance->isEnough);

        pthread_mutex_unlock(myBalance->lock);
	      // after deposit, sleep is called to allow withdraw.
	      sleep(1);
    }

    return NULL;
}

// thread for withdraw.
void *withdrawThread(void *arg) {
    while (1) {
        int amount = rand() % 10 + 1;

        pthread_mutex_lock(myBalance->lock);

        // if balance is less than amount to withdraw, it should wait signal
      	// which is sent after deposit
      	// while is used to recheck whether the balance is enough. 
        while (myBalance->balance < amount) {
            printf("\t\twait for deposit\n");
            pthread_cond_wait(myBalance->isEnough, myBalance->lock);
        }
      
        // balance is enough so can withdraw. 
        myBalance->balance -= amount;

        printf("\t\t\twithdraw %d\t\t%d\n", amount, myBalance->balance);
      	// all done, unlock is executed.
        pthread_mutex_unlock(myBalance->lock);
    }

    return NULL;
}

int main() {
    pthread_t deposit, withdraw;

    balanceInit();

    printf("deposit\t\twithdraw\t\tbalance\n");
 
    // create threads
    retVal = pthread_create(&deposit, NULL, depositThread, NULL);
    if (retVal) {
        fprintf(stderr, "pthread_create error\n");
        exit(-1);
    }

    retVal = pthread_create(&withdraw, NULL, withdrawThread, NULL);
    if (retVal) {
        fprintf(stderr, "pthread_create error\n");
        exit(-1);
    }
    
    // wait for threads to exit.
    retVal = pthread_join(deposit, NULL);
    if (retVal) {
        fprintf(stderr, "pthread_join error\n");
        exit(-1);
    }
    
    retVal = pthread_join(withdraw, NULL);
    if (retVal) {
        fprintf(stderr, "pthread_join error\n");
        exit(-1);
    }

    balanceDelete();

    return 0;
}
