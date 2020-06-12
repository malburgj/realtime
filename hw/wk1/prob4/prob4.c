/***********************************************************************************
 * @author Joshua Malburg
 * joshua.malburg@colorado.edu
 * Real-time Embedded Systems
 * ECEN5623 - Sam Siewert
 * @date 12Jun2020
 * Ubuntu 18.04 LTS and Jetbot
 ************************************************************************************
 *
 * @file prob4.c
 * @brief demo two threads running fib. sequence
 *
 ************************************************************************************
 */

/*---------------------------------------------------------------------------------*/
/* INCLUDES */
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <sched.h>
#include <stdint.h>
#include <unistd.h>
#include <semaphore.h>

/*---------------------------------------------------------------------------------*/
/* MACROS / TYPES / CONST */
#define NUM_THREADS                     (2)
#define FIB_LIMIT_FOR_32_BIT            (46)

uint32_t idx, jdx;
uint32_t fib, fib0, fib1;

#define FIB_TEST(iterCnt) {     \
  for (idx = 0; idx < iterCnt; idx++)   \
  {                                     \
    fib0 = 0;                           \
    fib1 = 1;                           \
    fib = fib0 + fib1;                  \
    jdx = 0;                            \
    while (jdx < FIB_LIMIT_FOR_32_BIT)  \
    {                                   \
      fib0 = fib1;                      \
      fib1 = fib;                       \
      fib = fib0 + fib1;                \
      jdx++;                            \
    }                                   \
  }                                     \
}

#define TIMESPEC_TO_mSEC(time)	((((float)time.tv_sec) * 1.0e3) + (((float)time.tv_nsec) / 1.0e6))

typedef struct
{
  int threadIdx;        /* thread id */
  sem_t *pSem;          /* semaphone */
  uint32_t interations; /* number of times to calculate sequence */
  uint32_t tries;       /* number of sequence tries */
} threadParams_t;

/*---------------------------------------------------------------------------------*/
/* PRIVATE FUNCTIONS */

int calc_dt(struct timespec *stop, struct timespec *start, struct timespec *delta_t);

/*---------------------------------------------------------------------------------*/
/* GLOBAL VARIABLES */

uint8_t gAbortTest = 0;
uint32_t seqIterations = FIB_LIMIT_FOR_32_BIT;

/*---------------------------------------------------------------------------------*/
/* FUNCTION DEFINITION */

void *fibWorker(void *arg)
{
  if(arg == NULL) {
    printf("ERROR: invalid arg provided to %s", __func__);
    return NULL;
  }

  /* get thread parameters */
  threadParams_t *threadParams = (threadParams_t *)arg;
  if(threadParams->pSem == NULL) {
    printf("ERROR: invalid semaphore provided to %s", __func__);
  }

  while (!gAbortTest) {
    int cnt = 0;                /* number of FIB_TEST tries performed */
    struct timespec startTime;  /* temp variable for profiling */
    struct timespec endTime;    /* ditto, putting here create doesn't impact numbers */
    struct timespec delta_t;    /* time duration of try */

    printf("%s-%d waiting to start... \n", __func__, 
    threadParams->threadIdx);

    /* wait for semaphore to do work */
    sem_wait(threadParams->pSem);
    if(gAbortTest) {
      return NULL;
    }
    clock_gettime(CLOCK_REALTIME, &startTime);

    /* do thread work */
    while ((cnt < threadParams->tries)) {
      FIB_TEST(threadParams->interations);
      ++cnt;
    }
    clock_gettime(CLOCK_REALTIME, &endTime);

    /* calculate time stats */
    calc_dt(&endTime, &startTime, &delta_t);
    float dt = TIMESPEC_TO_mSEC(delta_t);
      
    printf("%s-%d delta time: %f msec\n", __func__, 
    threadParams->threadIdx, dt);
  }

  printf("%s-%d exiting\n", __func__,threadParams->threadIdx);
  return NULL;
}

int main(int argc, char *argv[])
{
  pthread_t threads[NUM_THREADS];
  threadParams_t threadParams[NUM_THREADS];
  sem_t threaSemas[NUM_THREADS];

  /*----------------------------------------------*/
  /* create threads and semaphores */
  /*----------------------------------------------*/
  int i = 0;
  sem_init(&(threaSemas[i]), 0, 0);
  threadParams[i].pSem = &(threaSemas[i]);
  threadParams[i].threadIdx = (i + 1) * 10;
  threadParams[i].interations = 100000;
  threadParams[i].tries = 5;
  pthread_create(&threads[i],               // pointer to thread descriptor
                 (void *)0,                 // use default attributes
                 fibWorker,                 // thread function entry point
                 (void *)&(threadParams[i]) // parameters to pass in
  );

  ++i;
  sem_init(&(threaSemas[i]), 0, 0);
  threadParams[i].pSem = &(threaSemas[i]);
  threadParams[i].threadIdx = (i + 1) * 10;
  threadParams[i].interations = 100000;
  threadParams[i].tries = 10;
  pthread_create(&threads[i], (void *)0, fibWorker, (void *)&(threadParams[i]));

  /*----------------------------------------------*/
  /* run sequencer */
  /*----------------------------------------------*/
  struct timespec startTime;  /* temp variable for profiling */
  clock_gettime(CLOCK_REALTIME, &startTime);
  printf("starting sequencer at: %f msec\n\n", TIMESPEC_TO_mSEC(startTime));
  sem_post(&(threaSemas[0]));
  usleep(20000);
  sem_post(&(threaSemas[1]));
  usleep(20000);

  /*----------------------------------------------*/
  /* kill and clean up threads */
  /*----------------------------------------------*/
  printf("%s killing threads\n", __func__);
  gAbortTest = 1;

  /* give semaphore one last time in case
   * sem_wait() is blocking */
  sem_post(&(threaSemas[0]));
  sem_post(&(threaSemas[1]));
  printf("%s waiting ... \n", __func__);
  for (i = 0; i < NUM_THREADS; ++i) {
    pthread_join(threads[i], NULL);
  }

  /* don't forget to clean up these too! */
  for(uint8_t ind = 0;ind < NUM_THREADS; ++ind) {
    sem_destroy(&(threaSemas[i]));
  }
  printf("exiting\n");
}

int calc_dt(struct timespec *stop, struct timespec *start, struct timespec *delta_t)
{
  int dt_sec=stop->tv_sec - start->tv_sec;
  int dt_nsec=stop->tv_nsec - start->tv_nsec;

  if(dt_sec >= 0) {
    if(dt_nsec >= 0) {
      delta_t->tv_sec=dt_sec;
      delta_t->tv_nsec=dt_nsec;
    }
    else {
      delta_t->tv_sec=dt_sec-1;
      delta_t->tv_nsec= 1e9 + dt_nsec;
    }
  }
  else {
    if(dt_nsec >= 0) {
      delta_t->tv_sec=dt_sec;
      delta_t->tv_nsec=dt_nsec;
    }
    else {
      delta_t->tv_sec=dt_sec-1;
      delta_t->tv_nsec=1e9 + dt_nsec;
    }
  }
  return 0;
}