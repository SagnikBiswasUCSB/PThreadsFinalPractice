#include "cs140barrier.h"
#include <pthread.h>

/*
 * Initialize the barrier structure.
 */
int cs140barrier_init(cs140barrier *bstate, int total_nthread) {
    if (pthread_mutex_init(&bstate->barrier_mutex, NULL) != 0)
        return -1;
    if (pthread_cond_init(&bstate->barrier_cond, NULL) != 0) {
        pthread_mutex_destroy(&bstate->barrier_mutex);
        return -1;
    }
    bstate->total_nthread = total_nthread;
    bstate->arrive_nthread = total_nthread;
    bstate->odd_round = False;  // global sense starts as False
    return 0;
}

/*
 * Sense reversal barrier wait function.
 */
int cs140barrier_wait(cs140barrier *bstate) {
    // Each thread maintains its own thread-local sense.
    // __thread ensures that each thread has its own copy of local_sense.
    static __thread boolean local_sense = True;
    // Toggle the local sense for this barrier invocation.
    local_sense = (local_sense == True) ? False : True;

    pthread_mutex_lock(&bstate->barrier_mutex);

    // Mark that one more thread has reached the barrier.
    bstate->arrive_nthread--;
    if (bstate->arrive_nthread == 0) {
        // This is the last thread to arrive.
        // Reset the counter for the next round.
        bstate->arrive_nthread = bstate->total_nthread;
        // Flip the global sense to signal that this round is complete.
        bstate->odd_round = local_sense;
        // Wake up all the threads waiting on the condition variable.
        pthread_cond_broadcast(&bstate->barrier_cond);
        pthread_mutex_unlock(&bstate->barrier_mutex);
        return 1;  // Optionally indicate that this thread was the last.
    } else {
        // Not the last thread: wait until the global sense matches our local sense.
        while (bstate->odd_round != local_sense) {
            pthread_cond_wait(&bstate->barrier_cond, &bstate->barrier_mutex);
        }
        pthread_mutex_unlock(&bstate->barrier_mutex);
        return 0;
    }
}

/*
 * Destroy the barrier's mutex and condition variable.
 */
int cs140barrier_destroy(cs140barrier *bstate) {
    int ret1 = pthread_mutex_destroy(&bstate->barrier_mutex);
    int ret2 = pthread_cond_destroy(&bstate->barrier_cond);
    return (ret1 == 0 && ret2 == 0) ? 0 : -1;
}
