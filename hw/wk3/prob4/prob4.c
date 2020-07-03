/***********************************************************************************
 * @author Joshua Malburg
 * joshua.malburg@colorado.edu
 * Real-time Embedded Systems
 * ECEN5623 - Sam Siewert
 * @date 3Jul2020
 * Ubuntu 18.04 LTS and Jetbot
 ************************************************************************************
 *
 * @file prob4.c
 * @brief demo message queue example
 *
 ************************************************************************************
 */

/*---------------------------------------------------------------------------------*/
/* INCLUDES */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <string.h>
#include <mqueue.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <unistd.h>
#include <syslog.h>

/*---------------------------------------------------------------------------------*/
/* MACROS / TYPES / CONST */
#define NUM_THREADS         	        (2)
#define TIMESPEC_TO_nSEC(time)	      ((((float)time.tv_sec) * 1.0e9) + (((float)time.tv_nsec)))
#define SNDRCV_MQ                     ("/send_receive_mq")
#define MAX_MSG_SIZE                  (128)
#define ERROR                         (-1)
#define SEND_THREAD_NUM 			        (0)
#define RECV_THREAD_NUM 			        (SEND_THREAD_NUM + 1)

typedef struct {
  int threadIdx;    /* thread id */
  mqd_t *pMsgQueue; /* queue */
} threadParams_t;

/*---------------------------------------------------------------------------------*/
/* PRIVATE FUNCTIONS */
void *receiver(void *arg);
void *sender(void *arg);

/*---------------------------------------------------------------------------------*/
/* GLOBAL VARIABLES */
int gAbortTest = 0;
static char canned_msg[] = "God Bless America";

/*---------------------------------------------------------------------------------*/

int main(int argc, char *argv[])
{
  /* starting logging; use dmesg | grep prob4 
   * to view messages */
  openlog("prob4", LOG_PID | LOG_NDELAY | LOG_CONS, LOG_USER);
  syslog(LOG_INFO, ".");
  syslog(LOG_INFO, "..");
  syslog(LOG_INFO, "...");
  syslog(LOG_INFO, "logging started");
  
  /*---------------------------------------*/
  /* setup common message queue */
  /*---------------------------------------*/
  struct mq_attr mq_attr;
  mq_attr.mq_maxmsg = 10;
  mq_attr.mq_msgsize = MAX_MSG_SIZE;
  mq_attr.mq_flags = 0;

  mqd_t mymq = mq_open(SNDRCV_MQ, O_CREAT|O_RDWR | O_NONBLOCK, S_IRWXU, &mq_attr);
  if(mymq == (mqd_t)ERROR) {
    perror("receiver mq_open");
    exit(-1);
  }

  /*---------------------------------------*/
  /* create threads */
  /*---------------------------------------*/
  pthread_t threads[NUM_THREADS];
  threadParams_t threadParams;
  threadParams.pMsgQueue = &mymq;

  threadParams.threadIdx = SEND_THREAD_NUM;
  if(pthread_create(&threads[SEND_THREAD_NUM], (void *)0, sender, (void *)&threadParams) != 0) {
    syslog(LOG_ERR, "couldn't create thread#%d", SEND_THREAD_NUM);
  }

  threadParams.threadIdx = RECV_THREAD_NUM;
  if(pthread_create(&threads[RECV_THREAD_NUM], (void *)0, receiver, (void *)&threadParams ) != 0) {
    syslog(LOG_ERR, "couldn't create thread#%d", RECV_THREAD_NUM);
  }

  /*----------------------------------------------*/
  /* run main */
  /* wait 10 ms then kill threads */
  /*----------------------------------------------*/
  usleep(10000);
  gAbortTest = 1;

  /*----------------------------------------------*/
  /* exiting */
  /*----------------------------------------------*/
  syslog(LOG_INFO, "%s waiting on threads...", __func__);
  for(uint8_t ind = 0; ind < NUM_THREADS; ++ind) {
    pthread_join(threads[ind], NULL);
  }
  syslog(LOG_INFO, "%s exiting, stopping log", __func__);
  syslog(LOG_INFO, "...");
  syslog(LOG_INFO, "..");
  syslog(LOG_INFO, ".");
  closelog();
}

void *receiver(void *arg)
{
  char buffer[MAX_MSG_SIZE];
  int prio;
  int nbytes;
  int waitFlag = 1;

  /* get thread parameters */
  if(arg == NULL) {
    syslog(LOG_INFO, "ERROR: invalid arg provided to %s", __func__);
    return NULL;
  }
  threadParams_t *threadParams = (threadParams_t *)arg;
  if(threadParams->pMsgQueue == NULL) {
    syslog(LOG_ERR, "invalid pMsgQueue provided to %s", __func__);
    return NULL;
  }
  syslog(LOG_INFO, "%s started ...", __func__);
  while(waitFlag && (!gAbortTest)) {
    /* read oldest, highest priority msg from the message queue */
    if((nbytes = mq_receive(*threadParams->pMsgQueue, buffer, MAX_MSG_SIZE, &prio)) == ERROR) {
      syslog(LOG_ERR, "%s - mq_receive", __func__);
    } else {
      buffer[nbytes] = '\0';
      syslog(LOG_INFO, "%s-%d, %s received with priority = %d, length = %d",__func__, threadParams->threadIdx,
            buffer, prio, nbytes);
            waitFlag = 0;
    }
  }
  syslog(LOG_INFO, "%s-%d exiting", __func__,threadParams->threadIdx);
  return NULL;
}

void *sender(void*arg)
{
  int prio = 30;

  /* get thread parameters */
  if(arg == NULL) {
    syslog(LOG_ERR, "invalid arg provided to %s", __func__);
    return NULL;
  }
  threadParams_t *threadParams = (threadParams_t *)arg;
  syslog(LOG_INFO, "%s started ...", __func__);

  /* send message with priority = prio */
  if(mq_send(*threadParams->pMsgQueue, canned_msg, sizeof(canned_msg), prio) == ERROR) {
    syslog(LOG_ERR, "%s - mq_send", __func__);
  } else {
    syslog(LOG_INFO, "%s - mq_send succeeded", __func__);
  }
  syslog(LOG_INFO, "%s-%d exiting", __func__,threadParams->threadIdx);
  return NULL;
}
