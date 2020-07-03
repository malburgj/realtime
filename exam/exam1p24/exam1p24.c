/***********************************************************************************
 * @author Joshua Malburg
 * joshua.malburg@colorado.edu
 * Real-time Embedded Systems
 * ECEN5623 - Sam Siewert
 * @date 28Jun2020
 * Ubuntu 18.04 LTS and Jetbot
 ************************************************************************************
 *
 * @file exam1p24.c
 * @brief sum numbers in different threads, use real time
 *
 * run command: ./exam1p24
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
#include <errno.h>
#include <string.h>
#include <time.h>

/*---------------------------------------------------------------------------------*/
/* MACROS / TYPES / CONST */
#define NUM_THREADS         	(4)
#define LAST_NUM_TO_CALC      (400)
#define ARRAY_LEN             (LAST_NUM_TO_CALC + 1)

typedef struct {
  int threadIdx;        
  uint32_t sum;
  uint32_t startNum;
  uint32_t endNum;
} threadParams_t;

/*---------------------------------------------------------------------------------*/
/* PRIVATE FUNCTIONS */

int set_attr_policy(pthread_attr_t *attr, int policy, uint8_t priorityOffset);
int set_main_policy(int policy, uint8_t priorityOffset);
void print_scheduler(void);

/*---------------------------------------------------------------------------------*/
/* GLOBAL VARIABLES */
uint16_t numSequence[ARRAY_LEN];
pthread_mutex_t gMutex;

/*---------------------------------------------------------------------------------*/
/* FUNCTION DEFINITION */

void *sumThread(void *arg)
{
  if(arg == NULL) {
    printf("ERROR: invalid arg provided to %s\n\r", __func__);
    return NULL;
  }

  /* get thread parameters */
  threadParams_t *threadParams = (threadParams_t *)arg;
  printf("%s-%d start value: %d, stop value: %d\n\r", __func__, threadParams->threadIdx, threadParams->startNum, threadParams->endNum);

  for(uint32_t ind = threadParams->startNum; ind <= threadParams->endNum; ++ind) {
    

    pthread_mutex_lock(&gMutex);

    /* if the number is already crossed out (i.e. 0), 
     * skip because its not a prime */
    if(numSequence[ind]){
      uint32_t num = ind;
      while(num <= LAST_NUM_TO_CALC) {
        num += ind;
        numSequence[num] = 0;
      }
    }
    pthread_mutex_unlock(&gMutex);
  }
  return NULL;
}

int main(int argc, char *argv[])
{
  pthread_t threads[NUM_THREADS];
  threadParams_t threadParams[NUM_THREADS];
  pthread_mutex_t attitude_mutex;
  pthread_attr_t thread_attr[NUM_THREADS];

  pthread_mutex_init(&gMutex, NULL);

  /* preload bin with a 1 (i.e. is a prime) */
  for(int ind = 0; ind < LAST_NUM_TO_CALC; ++ind) {
    numSequence[ind] = 1;
  }
  /* 0 and 1 are not primes by definition */
  numSequence[0] = 0;
  numSequence[1] = 0;

  /*----------------------------------------------*/
  /* create threads
  /*----------------------------------------------*/
  uint8_t i = 0;
  threadParams[i].threadIdx = i;
  threadParams[i].startNum = i * 100 + 1;
  threadParams[i].endNum = threadParams[i].startNum + 99;
  if(pthread_create(&threads[i], 
    (void *)0,
    sumThread, (void *)&threadParams[i]) != 0) {
    printf("ERROR: couldn't create thread#%d\n\r", i);
  }

  ++i;
  threadParams[i].threadIdx = i;
  threadParams[i].startNum = i * 100 + 1;
  threadParams[i].endNum = threadParams[i].startNum + 99;
  if(pthread_create(&threads[i], 
    (void *)0,
    sumThread, (void *)&threadParams[i]) != 0) {
    printf("ERROR: couldn't create thread#%d\n\r", i);
  }

  ++i;
  threadParams[i].threadIdx = i;
  threadParams[i].startNum = i * 100 + 1;
  threadParams[i].endNum = threadParams[i].startNum + 99;
  if(pthread_create(&threads[i], 
    (void *)0,
    sumThread, (void *)&threadParams[i]) != 0) {
    printf("ERROR: couldn't create thread#%d\n\r", i);
  }

  ++i;
  threadParams[i].threadIdx = i;
  threadParams[i].startNum = i * 100 + 1;
  threadParams[i].endNum = threadParams[i].startNum + 99;
  if(pthread_create(&threads[i], 
    (void *)0,
    sumThread, (void *)&threadParams[i]) != 0) {
    printf("ERROR: couldn't create thread#%d\n\r", i);
  }

  printf("%s waiting on threads ... \n", __func__);
  for(uint8_t ind = 0;ind < NUM_THREADS; ++ind) {
    pthread_join(threads[ind], NULL);
  }

  /* cycle through again and find numbers that haven't be excluded */
  printf("found prime numbers:"); 
  uint16_t count = 0;
  for(uint32_t ind = 0; ind < ARRAY_LEN; ++ind) {
    if(numSequence[ind]) {
      count += numSequence[ind];
      printf("\t%d", ind);
    }
  }
  printf("\n\r");
  printf("%s exiting, count of prime numbers found: %d\n\r", __func__, count);
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
  int rtnCode = 0;

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
    printf("ERROR: set_attr_policy, errno: %s\n", strerror(errno));
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
    printf("ERROR: sched_getparam (in set_main_policy)  rc is %d, errno: %s\n", rtnCode, strerror(errno));
    return -1;
  }

  /* update scheduler */
  param.sched_priority = sched_get_priority_max(policy) - priorityOffset;
  rtnCode = sched_setscheduler(getpid(), policy, &param);
  if (rtnCode) {
    printf("ERROR: sched_setscheduler (in set_main_policy) rc is %d, errno: %s\n", rtnCode, strerror(errno));
    return -1;
  }
  return 0;
}