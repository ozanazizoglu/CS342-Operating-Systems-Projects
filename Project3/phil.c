#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/signal.h>
#include <signal.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <pthread.h>

#define PHILS 5

int eat, think; //eat and thinking times generated randomly.

typedef struct critical{
    enum {THINKING, HUNGRY, EATING} state[PHILS];
    pthread_mutex_t mutex; 
    pthread_cond_t condition[PHILS]; //phils will wait using this condition
    int eat[PHILS];
    int think[PHILS];

}critical; 

critical *shared;

/*  --------- Slides Code for test-------------
     void test (int i){
       if ((state[(i + 4) % 5] != EATING) && (state[i] == HUNGRY) && (state[(i + 1) % 5] != EATING)){
          state[i] = EATING;
          cond[i].signal();
        }
    }
*/
void test(int i){
    //check if neighbours are eating, if not eat yourself
    if (((shared->state)[(i + 4) % 5] != EATING) && ((shared->state)[i] == HUNGRY) && ((shared->state)[(i + 1) % 5] != EATING)){
        (shared->state)[i] = EATING;
        pthread_cond_broadcast(&(shared->condition)[i]); 
    }
}

/*  --------- Slides Code for putdown -------------
     void putdown(int i){
       state[i] = THINKING;
       test((i + 4) % 5); check if i can grab left philo's fork 
       test((i + 1) % 5); check if i can grab right philo's fork 
    }
*/
void putdown(int i){
    pthread_mutex_lock(&(shared->mutex)); 
    // CRITICAL SECTION BEGIN 
    (shared->state)[i] = THINKING; //eating finished, back to thinking
    printf("Philosopher %d finished eating\n", i);
    /* signal the left and right philosophers so they know you're finished eating */
    test( (i+4) % 5 );
    test( (i+1) % 5 );

    // CRITICAL SECTION END 
    pthread_mutex_unlock(&(shared->mutex)); 
}

/*  --------- Slides Code for pickup-------------
     void pickup(int i){
       state[i] = HUNGRY; im hungry gib food
       test(i); 
       if(state[i] != EATING ){
           cond[i].wait();
       } 
    }
*/
void pickup(int i){
    pthread_mutex_lock(&shared->mutex); 

    // CRITICAL SECTION BEGIN 
    (shared->state)[i] = HUNGRY;
    test(i);

    while((shared->state)[i] != EATING){
        pthread_cond_wait(&(shared->condition)[i], &(shared->mutex));
    }
    printf("Philosopher %d started eating\n", i);
    // CRITICAL SECTION END
    pthread_mutex_unlock((&shared->mutex)); 
}


static void* phil_thread(void *arg){

    int index = (int) ((long) arg); 
    
    while(1){
    	sleep(shared->think[index]);   // think for "think" seconds 
    	pickup(index);  // attempt to pickup, might suspend the thread if there is no available chopsticks
    	sleep(shared->eat[index]);     // if pickup is successful, simulate eating for "eat" seconds  
    	putdown(index); // putdown the sticks, finish eating go back thinking 
    }
    pthread_exit (NULL);
}

int main(int argc, char** argv){

    int think, eat;
    srand(time(NULL)); //seed for the random function


    pthread_t tids[PHILS]; // philosopher tids
    shared = (critical *) malloc(sizeof (critical));
    
    for(int i = 0; i < PHILS; i++){
        (shared->state)[i] = THINKING; // set every philosopher to thinking 
        think = (rand() % 10) + 1; 
        eat   = (rand() % 5) + 1;
        (shared->eat)[i] = eat;
        (shared->think)[i] = think;
    }

    pthread_mutex_init(&(shared->mutex), NULL); // initialize the mutex 
    
    // initialize condition variables
    for(int i = 0; i< PHILS; i++){
        pthread_cond_init(&(shared->condition)[i], NULL);
    }
    
    // create philosopher threads
    for(int i = 0; i< PHILS; i++){
        pthread_create(&tids[i], NULL, phil_thread, (void *) (long) i); 
    }

    //wait for philosopher threads to terminate
    for(int i =0; i < 5; i++){
        pthread_join(tids[i], NULL);
    } 

    free(shared); //release malloc'd memory
    
    /* destroy mutexes */
    pthread_mutex_destroy(&shared->mutex);
    for(int i =0; i < PHILS; i++){
        pthread_cond_destroy(&(shared->condition)[i]);
    }

    exit(0); 
}
