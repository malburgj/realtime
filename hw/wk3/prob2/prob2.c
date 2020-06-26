/***********************************************************************************
 * @author Joshua Malburg
 * joshua.malburg@colorado.edu
 * Real-time Embedded Systems
 * ECEN5623 - Sam Siewert
 * @date 12Jun2020
 * Ubuntu 18.04 LTS and Jetbot
 ************************************************************************************
 *
 * @file prob2.c
 * @brief demo two RT threads sharing global data, protected by a MUTEX
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

/*---------------------------------------------------------------------------------*/
/* MACROS / TYPES / CONST */
#define NUM_THREADS                     (2)
#define TIMESPEC_TO_nSEC(time)	((((float)time.tv_sec) * 1.0e9) + (((float)time.tv_nsec)))
#define SUB_THREAD_NUM (0)
#define PUB_THREAD_NUM (SUB_THREAD_NUM + 1)

typedef struct {
  int threadIdx;        /* thread id */
  pthread_mutex_t *pMutex;        /* mutex */
} threadParams_t;

typedef struct {
  float accel_x;          /* vehicle X translational aceleration, m/s */
  float accel_y;          /* vehicle y translational acceleration, m/s */
  float accel_z;          /* vehicle z translational accelaration, m/s */
  float roll;             /* roll angle, radian */
  float pitch;            /* vehicle pitch angle, radian */
  float yaw;              /* vehicle yaw/heading angle, radian */
  uint64_t timestamp_ns;
} Attitude_t;

/*---------------------------------------------------------------------------------*/
/* PRIVATE FUNCTIONS */

int calc_dt(struct timespec *stop, struct timespec *start, struct timespec *delta_t);
int set_attr_policy(pthread_attr_t *attr, int policy, uint8_t priorityOffset);
int set_main_policy(int policy, uint8_t priorityOffset);
void print_scheduler(void);
void update_state(Attitude_t *data);

/*---------------------------------------------------------------------------------*/
/* GLOBAL VARIABLES */

uint8_t gAbortTest = 0;
Attitude_t data;

/*---------------------------------------------------------------------------------*/
/* FUNCTION DEFINITION */

void *subWorker(void *arg)
{
  if(arg == NULL) {
    printf("ERROR: invalid arg provided to %s\n", __func__);
    return NULL;
  }

  /* get thread parameters */
  threadParams_t *threadParams = (threadParams_t *)arg;
  if(threadParams->pMutex == NULL) {
    printf("ERROR: invalid mutex provided to %s\n", __func__);
    return NULL;
  }
  printf("%s started ...\n", __func__);

  Attitude_t local_data;
  memset(&local_data, 0, sizeof(local_data));

  struct timespec readTime;
  uint64_t prev_timestamp_ns = 0;
  while (!gAbortTest) {
    /* try to get lock; if locked, wait because its critical
     * we don't miss data; could also do trylock here if 
     * we can just use most recent data */
    if(pthread_mutex_lock(threadParams->pMutex) == 0) {
      /* copy data */
      local_data.pitch = data.pitch;
      local_data.timestamp_ns = data.timestamp_ns;

      /* unlock shared data */
      pthread_mutex_unlock(threadParams->pMutex);
    }

    /* do other work */
    /* dummyWorkFunction(); */

    /* for diagnstics */
    clock_gettime(CLOCK_MONOTONIC, &readTime);
    if(prev_timestamp_ns != local_data.timestamp_ns) {
      printf("new data received pitch: %d: w/ timestamp: %ld ns, at: %ld ns, delta_t: %ld ns\n", (int)data.pitch, 
      local_data.timestamp_ns, (uint64_t)TIMESPEC_TO_nSEC(readTime), 
      (uint64_t)TIMESPEC_TO_nSEC(readTime) - local_data.timestamp_ns);
    }

    prev_timestamp_ns = local_data.timestamp_ns;
  }
  printf("%s-%d exiting\n\r", __func__,threadParams->threadIdx);
  return NULL;
}

void *pubWorker(void *arg)
{
  if(arg == NULL) {
    printf("ERROR: invalid arg provided to %s\n\r", __func__);
    return NULL;
  }

  /* get thread parameters */
  threadParams_t *threadParams = (threadParams_t *)arg;
  if(threadParams->pMutex == NULL) {
    printf("ERROR: invalid mutex provided to %s\n\r", __func__);
    return NULL;
  }
  printf("%s started ...\n\r", __func__);

  while (!gAbortTest) {
    struct timespec writeTime;
    Attitude_t local_data;

    /* calculate vehicle attitude */
    update_state(&local_data);

    /* this is an example of using trylock; if for some reason
     * we can't get lock, just cycle back around and calculate
     * new state data; if we were to allow block here, the 
     * data would be stale when sent, we don't want that */
    if(pthread_mutex_lock(threadParams->pMutex) == 0) {
      data.accel_x = local_data.accel_x;
      data.accel_y = local_data.accel_y;
      data.accel_z = local_data.accel_z;
      data.pitch = local_data.pitch;
      data.roll = local_data.roll;
      data.yaw = local_data.yaw;
      clock_gettime(CLOCK_MONOTONIC, &writeTime);
      data.timestamp_ns = TIMESPEC_TO_nSEC(writeTime);
      pthread_mutex_unlock(threadParams->pMutex);
    }
    usleep(1e3);
  }
  printf("%s-%d exiting\n\r", __func__,threadParams->threadIdx);
  return NULL;
}

int main(int argc, char *argv[])
{
  pthread_t threads[NUM_THREADS];
  threadParams_t threadParams;
  pthread_mutex_t attitude_mutex;
  pthread_attr_t thread_attr;

  pthread_mutex_init(&attitude_mutex, NULL);

  /* set scheduling policy of main and threads */
  print_scheduler();
  set_main_policy(SCHED_FIFO, 0);
  set_attr_policy(&thread_attr, SCHED_FIFO, 1);
  print_scheduler();

  /*----------------------------------------------*/
  /* create threads and semaphores ...
   * set thread attributes (scheduling) too! */
  /*----------------------------------------------*/
  threadParams.pMutex = &attitude_mutex;
  threadParams.threadIdx = PUB_THREAD_NUM;
  if(pthread_create(&threads[PUB_THREAD_NUM], 
    //(void *)0,
    &thread_attr, 
    pubWorker, (void *)&threadParams) != 0) {
    printf("ERROR: couldn't create thread#%d\n\r", PUB_THREAD_NUM);
  }

  threadParams.pMutex = &attitude_mutex;
  threadParams.threadIdx = SUB_THREAD_NUM;
  if(pthread_create(&threads[SUB_THREAD_NUM], 
    //(void *)0,
    &thread_attr, 
    subWorker, (void *)&threadParams ) != 0) {
    printf("ERROR: couldn't create thread#%d\n\r", SUB_THREAD_NUM);
  }

  /*----------------------------------------------*/
  /* run main */
  /*----------------------------------------------*/
  /* just run for X seconds then kill
   * all threads */
  usleep(10e3);
  gAbortTest = 1;

  printf("%s waiting on threads ... \n", __func__);
  for(uint8_t ind = 0;ind < NUM_THREADS; ++ind) {
    pthread_join(threads[ind], NULL);
  }

  /* don't forget to clean up these too! */
  pthread_mutex_destroy(&attitude_mutex);
  pthread_attr_destroy(&thread_attr);

  printf("%s exiting\n", __func__);
}

int calc_dt(struct timespec *stop, struct timespec *start, struct timespec *delta_t)
{
  int dt_sec = stop->tv_sec - start->tv_sec;
  int dt_nsec = stop->tv_nsec - start->tv_nsec;

  if(dt_sec >= 0) {
    if(dt_nsec >= 0) {
      delta_t->tv_sec = dt_sec;
      delta_t->tv_nsec = dt_nsec;
    }
    else {
      delta_t->tv_sec = dt_sec - 1;
      delta_t->tv_nsec = 1e9 + dt_nsec;
    }
  }
  else {
    if(dt_nsec >= 0) {
      delta_t->tv_sec = dt_sec;
      delta_t->tv_nsec = dt_nsec;
    }
    else {
      delta_t->tv_sec = dt_sec-1;
      delta_t->tv_nsec = 1e9 + dt_nsec;
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

void update_state(Attitude_t *data)
{
  static uint32_t cnt;
  if(data == NULL) {
    return;
  }
  ++cnt;

  data->accel_x = 0.0f;
  data->accel_y = 0.0f;
  data->accel_z = 0.0f;
  data->pitch = (float)cnt;
  data->roll = 0.0f;
  data->yaw = 0.0f;
}