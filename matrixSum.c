/* matrix summation using pthreads

   features: uses a barrier; the Worker[0] computes
             the total sum from partial sums computed by Workers
             and prints the total sum to the standard output

   usage under Linux:
     gcc matrixSum.c -lpthread
     a.out size numWorkers

*/
#ifndef _REENTRANT 
#define _REENTRANT 
#endif 
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <sys/time.h>
#define MAXSIZE 10000  /* maximum matrix size */
#define MAXWORKERS 10   /* maximum number of workers */

typedef struct __result{
  unsigned int min;
  int max;
  int sum;
} result;

pthread_mutex_t barrier;  /* mutex lock for the barrier */
pthread_cond_t go;        /* condition variable for leaving */
int numWorkers;           /* number of workers */ 
int numArrived = 0;       /* number who have arrived */

/* a reusable counter barrier */
void Barrier() {
  pthread_mutex_lock(&barrier);
  numArrived++;
  if (numArrived == numWorkers) {
    numArrived = 0;
    pthread_cond_broadcast(&go);
  } else
    pthread_cond_wait(&go, &barrier);
    pthread_mutex_unlock(&barrier);
}

/* timer */
double read_timer() {
    static bool initialized = false;
    static struct timeval start;
    struct timeval end;
    if( !initialized )
    {
        gettimeofday( &start, NULL );
        initialized = true;
    }
    gettimeofday( &end, NULL );
    return (end.tv_sec - start.tv_sec) + 1.0e-6 * (end.tv_usec - start.tv_usec);
}

double start_time, end_time; /* start and end times */
int size, stripSize;  /* assume size is multiple of numWorkers */
int sums[MAXWORKERS]; /* partial sums */
unsigned int minimis[MAXWORKERS];
int maximis[MAXWORKERS];
int matrix[MAXSIZE][MAXSIZE]; /* matrix */
pthread_t workerid[MAXWORKERS];
void *Worker(void *);

/* read command line, initialize, and create threads */
int main(int argc, char *argv[]) {
  int i, j;
  long l; /* use long in case of a 64-bit system */
  pthread_attr_t attr;

  /* set global thread attributes */
  pthread_attr_init(&attr);
  pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);

  /* initialize mutex and condition variable */
  pthread_mutex_init(&barrier, NULL);
  pthread_cond_init(&go, NULL);

  /* read command line args if any */
  size = (argc > 1)? atoi(argv[1]) : MAXSIZE;
  numWorkers = (argc > 2)? atoi(argv[2]) : MAXWORKERS;
  if (size > MAXSIZE) size = MAXSIZE;
  if (numWorkers > MAXWORKERS) numWorkers = MAXWORKERS;
  stripSize = size/numWorkers;

  /* initialize the matrix */
  srand(time(NULL));
  for (i = 0; i < size; i++) {
	  for (j = 0; j < size; j++) {
	    matrix[i][j] = rand() % 99;//rand()%99;
	  }
  }

  /* print the matrix */
#ifdef DEBUG
  for (i = 0; i < size; i++) {
	  printf("[ ");
	  for (j = 0; j < size; j++) {
	    printf(" %d", matrix[i][j]);
	  }
	  printf(" ]\n");
  }
#endif

  /* do the parallel work: create the workers */
  start_time = read_timer();
  for (l = 0; l < numWorkers; l++)
    pthread_create(&workerid[l], &attr, Worker, (void *) l);
  pthread_exit(NULL);
}

/* Each worker sums the values in one strip of the matrix.
   After a barrier, worker(0) computes and prints the total */
void *Worker(void *arg) {
  long myid = (long) arg;
  int i, j, first, last;
  result *res = malloc(sizeof(result));
#ifdef DEBUG
  printf("worker %d (pthread id %d) has started\n", myid, pthread_self());
#endif

  /* determine first and last rows of my strip */
  first = myid*stripSize;
  last = (myid == numWorkers - 1) ? (size - 1) : (first + stripSize - 1);

  /* sum values in my strip */
  int total;
  total = 0;
  /* minimum and maximum values */
  unsigned int minimi;
  int maximi;
  minimi = 0;
  maximi = 0;
  minimi = minimi - 1;

  /******************************/
  for (i = first; i <= last; i++){
    for (j = 0; j < size; j++){
      total += matrix[i][j];
      if(maximi < matrix[i][j])
	maximi = matrix[i][j];
      if(minimi > matrix[i][j])
        minimi = matrix[i][j];
    }
  }
  /*return package*/
  res->sum = total;
  res->max = maximi;
  res->min = minimi;
#ifdef DEBUG
  printf("maxmi for thread %d is %d or %d\n", myid, maximi, res->max);
  printf("minimi for thread %d is %u or %d\n", myid, minimi, res->min);
  printf("total for thread %d is %d or %d\n", myid, total, res->sum);
#endif
  //sums[myid] = total;
  //maximis[myid] = maximi;
  //minimis[myid] = minimi;

  /* Barrier */
 // Barrier();
  if (myid == 0) {
    total = 0;
    result *temp;
    for(i = 1; i < numWorkers; i++){
       pthread_join(workerid[i], &temp);
       if(minimi > temp->min)
         minimi = temp->min;
       if(maximi < temp->max)
         maximi = temp->max;
       total += temp->sum;
#ifdef DEBUG
         printf("maxmini for thread %d is %d or %d in iteration %d\n", myid, maximi, temp->max, i);
         printf("minimi for thread %d is %u or %d in iteration %d\n", myid, minimi, temp->min, i);
         printf("total for thread %d is %d or %d in iteration %d\n", myid, total, temp->sum, i);
#endif
    }
    /*
    for (i = 0; i < numWorkers; i++){
      *total += sums[i];
      printf("Current minimi %u Current maximi %d\n", minimis[i], maximis[i]); 
      if(*minimi > minimis[i])
        *minimi = minimis[i];
      if(*maximi < maximis[i])
        *maximi = maximis[i];
    }
    */
    /* get end time */
    end_time = read_timer();
    /* print results */
    printf("The total is %d\n", total);
    printf("The maximi is %d The minimi is %u\n", maximi, minimi);
    printf("The execution time is %g sec\n", end_time - start_time);
    free(res);
  }else{
    pthread_exit(res);
  }
}
