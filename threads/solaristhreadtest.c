#include <thread.h>
#include <sys/time.h>
#include <stdio.h>
#include <errno.h>

int iterations;
int threads;
int still_running;
mutex_t mutex = DEFAULTMUTEX;
cond_t cond = DEFAULTCV;

void usage() {
    printf("Usage: threadtest num_iterations num_threads\n");
    exit(1);
}

void *timewaster(void *arg) {
    int i;
    for (i = 0; i < iterations; i++);
    mutex_lock(&mutex);
    still_running--;
    cond_signal(&cond);
    mutex_unlock(&mutex);
    return NULL;
}

int main(int argc, char *argv[]) {
    struct timeval starttime, endtime;
    int i;
    if (argc != 3)
	usage();
    gettimeofday(&starttime, NULL);
    if (sscanf(argv[1], "%d", &iterations) != 1 || iterations < 1)
	usage();
    if (sscanf(argv[2], "%d", &threads) != 1 || threads < 1)
	usage();

    still_running = threads;
    for (i = 0; i < threads; i++) {
	int status;
	pthread_t thread_id;
	status = thr_create(NULL, 0, timewaster, NULL, THR_BOUND, &thread_id);
	if (status  != 0) {
	    printf("Thread creation failed, status=%d\n", status);
	    exit(2);
	}
    }

    mutex_lock(&mutex);
    while (still_running != 0) {
	printf("Still %d thread%s running\n", still_running, still_running == 1 ? "" : "s");
	cond_wait(&cond, &mutex);
    }
    mutex_unlock(&mutex);
    gettimeofday(&endtime, NULL);
    if (endtime.tv_usec < starttime.tv_usec) {
	endtime.tv_sec--;
	endtime.tv_usec += 1000000;
    }
    printf("Done in %d.%06u seconds\n", endtime.tv_sec - starttime.tv_sec,
			endtime.tv_usec - starttime.tv_usec);
    return 0;
}
