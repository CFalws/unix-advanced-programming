#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>

#define NUM_PHILOSOPHER 5
#define NUM_CHOPSTICK   5

// Right chopstick number = philosopherID.
#define RIGHT           0

// Left chopstick number = 1 + philosopherID.
#define LEFT            1

int ret_count;

typedef struct {

    bool available[NUM_CHOPSTICK];
    pthread_mutex_t *mut;
    // each freed element is for release of each chopstick.
    pthread_cond_t *freed[NUM_CHOPSTICK];

} Chopstick;

// pointer to chopstick is located in datasegment.
Chopstick *chopstick;

// this should be passed to each thread, so every thread could see ‘my ID’.
// ‘my id’ is indeed. Because it tells which chopstick ‘I’ should use.
// this is also declared in global. 
int philosopherIDArr[5] = {0, 1, 2, 3, 4};

void chopstickInit(void);
void chopstickDelete(void);
void think(void);
void pickupChopstick(int philosopherID);
void eat(void);
void putdownChopstick(int phiosopherID);

// Threads
void *philosopher(void *arg);


void chopstickInit(void) {
    int i;

	// instace of chopstick is located in heap.
    chopstick = (Chopstick *)malloc(sizeof(Chopstick));
    for(i = 0; i < NUM_CHOPSTICK; i++) {
		// every chopstick is available at the first moment.
        chopstick->available[i] = true;
    }

    chopstick->mut = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
    ret_count = pthread_mutex_init(chopstick->mut, NULL);
    if (ret_count) {
        fprintf(stderr, "Error: return from pthread_mutex_init() is %d.\n",ret_count);
        exit(-1);
    }

    for(i = 0; i < NUM_CHOPSTICK; i++) {
        chopstick->freed[i] = (pthread_cond_t *) malloc(sizeof(pthread_cond_t));
        pthread_cond_init(chopstick->freed[i], NULL);
    }
}

// delete all the resources allocated in heap. Chopstick->mut, 
// chopstick->freed, chopstick.
void chopstickDelete(void) {
    int i;

    ret_count = pthread_mutex_destroy(chopstick->mut);
    if (ret_count) {
        fprintf(stderr, "Error: return from pthread_mutex_destroy() is %d.\n",
                ret_count);
        exit(-1);
    }

    free(chopstick->mut);

    for(i = 0; i < NUM_CHOPSTICK; i++) {
        pthread_cond_destroy(chopstick->freed[i]);
        free(chopstick->freed[i]);
    }

    free(chopstick);
}



void think(void) {
    int i;

    for(i = 0; i < 1e9; i++) ;
}

// this function acquires chopsticks. 
void pickupChopstick(int philosopherID) {
    
	// to synchronize availability of  chopsticks, mutex is locked.
    pthread_mutex_lock(chopstick->mut);
    
	// if both right and left chopsticks are free, philoshpher can take all
    // chopsticks. If not, it waits for the signal. 
    // while is used, because it should recheck after mutex is locked. 
    // if ‘if () ~’ is used, there can be synchronization problem.  
    while (!(chopstick->available[(philosopherID + RIGHT) % 5])
        || !(chopstick->available[(philosopherID + LEFT) % 5])) {
        if (!(chopstick->available[(philosopherID + RIGHT) % 5]))
            pthread_cond_wait(chopstick->freed[(philosopherID + RIGHT) % 5],
                            chopstick->mut);
        if (!(chopstick->available[(philosopherID + LEFT) % 5]))
            pthread_cond_wait(chopstick->freed[(philosopherID + LEFT) % 5],
                            chopstick->mut);
    }
    
	// it should change the state of using chopstick to unavailable.
    chopstick->available[(philosopherID + RIGHT) % 5] = false;
    chopstick->available[(philosopherID + LEFT) % 5] = false;

    printf("%d got chopsticks\n", philosopherID);
    
	// it release mutex.
    pthread_mutex_unlock(chopstick->mut);
}

// eating procedure.
void eat(void) {
    int i;

    for(i = 0; i < 1e9; i++) ;
}

// releasing of chopsticks procedure.
void putdownChopstick(int philosopherID) {

	// to change the state of shared resources, it should acquire lock.
    pthread_mutex_lock(chopstick->mut);
    
	// releasing chopsticks.
    chopstick->available[(philosopherID + RIGHT) % 5] = true;
    chopstick->available[(philosopherID + LEFT) % 5] = true;

    printf("%d release chopsticks\n", philosopherID);
    
	// control to shared resources done.
    pthread_mutex_unlock(chopstick->mut);

    // wake waiting threads up to check if they can acquire chopsticks needed.
	pthread_cond_signal(chopstick->freed[(philosopherID + RIGHT) % 5]);
    pthread_cond_signal(chopstick->freed[(philosopherID + LEFT) % 5]);
}

void *philosopher(void *arg) {
    // arg is int * which points to philosopherIDArr[i].
    int *ptrID = (int *)arg;
    while(1) {
        think();
        pickupChopstick(*ptrID);
        eat();
        putdownChopstick(*ptrID);
    }

    return NULL;
}

int main() {
    int i;
    pthread_t philosopher_t[NUM_PHILOSOPHER];

    chopstickInit();

    // thread creation. return value is better in global.
    for(i = 0; i < NUM_PHILOSOPHER; i++) {
		// creating threads.
        ret_count = pthread_create(&philosopher_t[i], NULL, philosopher, (void *)&philosopherIDArr[i]);
        if (ret_count) {
            fprintf(stderr, "Error: return from pthread_create() is %d.\n", ret_count);
            exit(-1);
        }
    }

    for(i = 0; i < NUM_PHILOSOPHER; i++) {
		// waiting for threads to exit
        ret_count = pthread_join(philosopher_t[i], NULL);
        if (ret_count) {
            fprintf(stderr, "Error: return from pthread_join() is %d.\n", ret_count);
            exit(-1);
        }
    }
    
	// free resources allocated.
    chopstickDelete();

    return 0;
}
