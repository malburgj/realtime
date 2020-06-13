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
int set_attr_policy(pthread_attr_t *attr, int policy, uint8_t priorityOffset);
int set_main_policy(int policy, uint8_t priorityOffset);

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
  sem_t thread_sem[NUM_THREADS];
  pthread_attr_t thread_attr[NUM_THREADS];

  /* set scheduling policy of main */  
  set_main_policy(SCHED_FIFO, 0);

  /*----------------------------------------------*/
  /* create threads and semaphores ...
   * set thread attributes (scheduling) too! */
  /*----------------------------------------------*/
  int i = 0;
  sem_init(&(thread_sem[i]), 0, 0);
  set_attr_policy(&thread_attr[i], SCHED_FIFO, i + 1);
  threadParams[i].pSem = &(thread_sem[i]);
  threadParams[i].threadIdx = (i + 1) * 10;
  threadParams[i].interations = 100000;
  threadParams[i].tries = 5;
  pthread_create(&threads[i],               // pointer to thread descriptor
                 &thread_attr[i],           // set custom attributes!
                 fibWorker,                 // thread function entry point
                 (void *)&(threadParams[i]) // parameters to pass in
  );

  ++i;
  sem_init(&(thread_sem[i]), 0, 0);
  set_attr_policy(&thread_attr[i], SCHED_FIFO, i + 1);
  threadParams[i].pSem = &(thread_sem[i]);
  threadParams[i].threadIdx = (i + 1) * 10;
  threadParams[i].interations = 100000;
  threadParams[i].tries = 10;
  pthread_create(&threads[i], &thread_attr[i], fibWorker, (void *)&(threadParams[i]));

  /*----------------------------------------------*/
  /* run sequencer */
  /*----------------------------------------------*/
  struct timespec startTime;  /* temp variable for profiling */
  clock_gettime(CLOCK_REALTIME, &startTime);
  printf("starting sequencer at: %f msec\n\n", TIMESPEC_TO_mSEC(startTime));
  sem_post(&(thread_sem[0])), sem_post(&(thread_sem[1]));

  usleep(20000); sem_post(&(thread_sem[0]));
  usleep(20000); sem_post(&(thread_sem[0]));
  usleep(10000); sem_post(&(thread_sem[1]));
  usleep(10000); sem_post(&(thread_sem[0]));
  usleep(20000); sem_post(&(thread_sem[0]));
  usleep(20000);

  /*----------------------------------------------*/
  /* kill and clean up threads */
  /*----------------------------------------------*/
  printf("%s killing threads\n", __func__);
  gAbortTest = 1;

  printf("%s waiting ... \n", __func__);
  for(uint8_t ind = 0;ind < NUM_THREADS; ++ind) {
    /* give semaphore one last time in case
     * sem_wait() is blocking */
    sem_post(&(thread_sem[ind]));
    pthread_join(threads[ind], NULL);
  }

  /* don't forget to clean up these too! */
  for(uint8_t ind = 0;ind < NUM_THREADS; ++ind) {
    sem_destroy(&(thread_sem[ind]));
    pthread_attr_destroy(&(thread_attr[ind]));
  }
  printf("%s exiting\n", __func__);
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

void print_scheduler(void)
{
  switch (sched_getscheduler(getpid()))
  {
  case SCHED_FIFO:
    printf("Pthread Policy is SCHED_FIFO\n");
    break;
  case SCHED_OTHER:
    printf("Pthread Policy is SCHED_OTHER\n");
    break;
  case SCHED_RR:
    printf("Pthread Policy is SCHED_OTHER\n");
    break;
  default:
    printf("Pthread Policy is UNKNOWN\n");
  }
}

int set_attr_policy(pthread_attr_t *attr, int policy, uint8_t priorityOffset)
{
  int max_prio;
  int min_prio;
  struct sched_param param;
  int rtnCode;

  if(policy < 0) {
    printf("ERROR: invalid policy #: %d\n", policy);
    perror("setSchedPolicy");
    // SCHED_OTHER     --> 0
    // SCHED_FIFO      --> 1
    // SCHED_RR        --> 2
    // SCHED_BATCH     --> 3
    // SCHED_ISO       --> 4
    // SCHED_IDLE      --> 5
    // SCHED_DEADLINE  --> 6
    return -1;
  }
  else if(attr == NULL) {
    return -1;
  }

  /* set attribute structure */
  rtnCode |= pthread_attr_init(attr);
  rtnCode |= pthread_attr_setinheritsched(attr, PTHREAD_EXPLICIT_SCHED);
  rtnCode |= pthread_attr_setschedpolicy(attr, policy);

  param.sched_priority = sched_get_priority_max(policy) - priorityOffset;
  rtnCode |= pthread_attr_setschedparam(attr, &param);
  if (rtnCode) {
    printf("ERROR: set_attr_policy");
    return -1;
  }
  return 0;
}

int set_main_policy(int policy, uint8_t priorityOffset)
{
  int rtnCode;
  int max_prio;
  int min_prio;
  struct sched_param param;
  pthread_attr_t attr;

  if(policy < 0) {
    printf("ERROR: invalid policy #: %d\n", policy);
    perror("setSchedPolicy");
    return -1;
  }

  /* this sets the policy/priority for our process */
  rtnCode = sched_getparam(getpid(), &param);
  if (rtnCode) {
    printf("ERROR: sched_setscheduler rc is %d\n", rtnCode);
    perror("sched_setscheduler");
    return -1;
  }

  /* update scheduler */
  param.sched_priority = sched_get_priority_max(policy) - priorityOffset;
  rtnCode = sched_setscheduler(getpid(), policy, &param);
  if (rtnCode) {
    printf("ERROR: sched_setscheduler rc is %d\n", rtnCode);
    perror("sched_setscheduler");
    return -1;
  }
  return 0;
}