/***********************************************************************************
 * @author Joshua Malburg
 * joshua.malburg@colorado.edu
 * Real-time Embedded Systems
 * ECEN5623 - Sam Siewert
 * @date 28Jun2020
 * Ubuntu 18.04 LTS and Jetbot
 ************************************************************************************
 *
 * @file exam1p1.c
 * @brief sum numbers in different threads, use real time
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
#include <errno.h>
#include <string.h>
#include <time.h>

/*---------------------------------------------------------------------------------*/
/* MACROS / TYPES / CONST */
#define NUM_THREADS         	(3)

typedef struct {
  int threadIdx;        
  uint32_t sum;
  uint32_t startNum;
  uint32_t width;
} threadParams_t;

/*---------------------------------------------------------------------------------*/
/* PRIVATE FUNCTIONS */

int set_attr_policy(pthread_attr_t *attr, int policy, uint8_t priorityOffset);
int set_main_policy(int policy, uint8_t priorityOffset);
void print_scheduler(void);

/*---------------------------------------------------------------------------------*/
/* GLOBAL VARIABLES */

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
  printf("%s started ...\n\r", __func__);

  uint32_t sum = 0;
  for(uint32_t ind = 0; ind < threadParams->width; ++ind) {
    uint32_t num = ind + threadParams->startNum;
    sum += num;
  }
  threadParams->sum = sum;
  printf("%s-%d exiting, sum is: %d\n\r", __func__,threadParams->threadIdx, threadParams->sum);
  return NULL;
}

int main(int argc, char *argv[])
{
  pthread_t threads[NUM_THREADS];
  threadParams_t threadParams[NUM_THREADS];
  pthread_mutex_t attitude_mutex;
  pthread_attr_t thread_attr[NUM_THREADS];



  /* set scheduling policy of main and threads */
  print_scheduler();
  set_main_policy(SCHED_FIFO, 0);
  print_scheduler();

  /*----------------------------------------------*/
  /* create threads
  /*----------------------------------------------*/
  uint8_t i = 0;
  set_attr_policy(&thread_attr[i], SCHED_FIFO, i);
  threadParams[i].threadIdx = i;
  threadParams[i].startNum = i * 100;
  threadParams[i].width = 100;
  if(pthread_create(&threads[i], 
    &thread_attr[i],
    sumThread, (void *)&threadParams[i]) != 0) {
    printf("ERROR: couldn't create thread#%d\n\r", i);
  }

  ++i;
  set_attr_policy(&thread_attr[i], SCHED_FIFO, i);
  threadParams[i].threadIdx = i;
  threadParams[i].startNum = i * 100;
  threadParams[i].width = 100;
  if(pthread_create(&threads[i], 
    &thread_attr[i],
    sumThread, (void *)&threadParams[i]) != 0) {
    printf("ERROR: couldn't create thread#%d\n\r", i);
  }

  ++i;
  set_attr_policy(&thread_attr[i], SCHED_FIFO, i);
  threadParams[i].threadIdx = i;
  threadParams[i].startNum = i * 100;
  threadParams[i].width = 100;
  if(pthread_create(&threads[i], 
    &thread_attr[i],
    sumThread, (void *)&threadParams[i]) != 0) {
    printf("ERROR: couldn't create thread#%d\n\r", i);
  }

  printf("%s waiting on threads ... \n", __func__);
  for(uint8_t ind = 0;ind < NUM_THREADS; ++ind) {
    pthread_join(threads[ind], NULL);
  }

  uint32_t sum = 0;
  for(uint8_t ind = 0;ind < NUM_THREADS; ++ind) {
    sum += threadParams[ind].sum;
  }
  printf("%s exiting, sum of all threads is: %d\n\r", __func__, sum);
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