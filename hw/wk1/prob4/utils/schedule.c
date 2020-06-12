/***********************************************************************************
 * @author Joshua Malburg
 * joshua.malburg@colorado.edu
 * Real-time Embedded Systems
 * ECEN5623 - Sam Siewert
 * @date 12Jun2020
 * Ubuntu 18.04 LTS and Jetbot
 ************************************************************************************
 *
 * @file schedule.c
 * @brief schedule helper functions
 *
 ************************************************************************************
 */

#include <sys/time.h>
#include <sys/types.h>
#include <stdio.h>
#include <pthread.h>
#include <sched.h>
#include <stdlib.h>
#include <signal.h>

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

void setSchedPolicy(pthread_attr_t *attr, int policy)
{
  int rc, scope;
  int rt_max_prio;
  int rt_min_prio;
  struct sched_param rt_param;

  print_scheduler();

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
    return;
  }
  else if(attr == NULL) {
    return;
  }

  pthread_attr_init(attr);
  pthread_attr_setinheritsched(attr, PTHREAD_EXPLICIT_SCHED);
  pthread_attr_setschedpolicy(attr, policy);

  rt_max_prio = sched_get_priority_max(policy);
  rt_min_prio = sched_get_priority_min(policy);

  rc = sched_getparam(getpid(), &rt_param);
  rt_param.sched_priority = rt_max_prio;

  rc = sched_setscheduler(getpid(), policy, &rt_param);

  if (rc) {
    printf("ERROR: sched_setscheduler rc is %d\n", rc);
    perror("sched_setscheduler");
  }
  else {
    printf("SCHED_POLICY SET: sched_setscheduler rc is %d\n", rc);
  }
  print_scheduler();

  printf("min prio = %d, max prio = %d\n", rt_min_prio, rt_max_prio);
  pthread_attr_getscope(attr, &scope);

  if (scope == PTHREAD_SCOPE_SYSTEM)
    printf("PTHREAD SCOPE SYSTEM\n");
  else if (scope == PTHREAD_SCOPE_PROCESS)
    printf("PTHREAD SCOPE PROCESS\n");
  else
    printf("PTHREAD SCOPE UNKNOWN\n");

  rt_param.sched_priority = rt_max_prio - 1;
  pthread_attr_setschedparam(attr, &rt_param);
}