/***********************************************************************************
 * @author Joshua Malburg
 * joshua.malburg@colorado.edu
 * Real-time Embedded Systems
 * ECEN5623 - Sam Siewert
 * @date 3Jul2020
 * Ubuntu 18.04 LTS and Jetbot
 ************************************************************************************
 *
 * @file prob4_heap.c
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
#define MAX_MSG_SIZE                  (sizeof(void *)+sizeof(int))
#define ERROR                         (-1)
#define SEND_THREAD_NUM 			        (0)
#define RECV_THREAD_NUM 			        (SEND_THREAD_NUM + 1)
#define IMG_SIZE                      (4096)

typedef struct {
  int threadIdx;    /* thread id */
  mqd_t *pMsgQueue; /* queue */
} threadParams_t;

/*---------------------------------------------------------------------------------*/
/* PRIVATE FUNCTIONS */
void *receiver(void *arg);
void *sender(void *arg);
void init_img_buffer(void);

/*---------------------------------------------------------------------------------*/
/* GLOBAL VARIABLES */
int gAbortTest = 0;
static char canned_msg[] = "God Bless America";
int headExampleFlag;
static char imagebuff[IMG_SIZE];

/*---------------------------------------------------------------------------------*/

int main(int argc, char *argv[])
{
  /* starting logging; use cat /var/log/syslog | grep prob4
   * to view messages */
  openlog("prob4_heap", LOG_PID | LOG_NDELAY | LOG_CONS, LOG_USER);
  syslog(LOG_INFO, ".");
  syslog(LOG_INFO, "..");
  syslog(LOG_INFO, "...");
  syslog(LOG_INFO, "logging started");

  init_img_buffer();
  
  /*---------------------------------------*/
  /* setup common message queue */
  /*---------------------------------------*/
  struct mq_attr mq_attr;
  mq_attr.mq_maxmsg = 100;
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
  if(pthread_create(&threads[RECV_THREAD_NUM], (void *)0, receiver, (void *)&threadParams) != 0) {
    syslog(LOG_ERR, "couldn't create thread#%d", RECV_THREAD_NUM);
  }

  /*----------------------------------------------*/
  /* run main */
  /* wait 10 ms then kill threads */
  /*----------------------------------------------*/
  usleep(100000);
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
  mq_unlink(SNDRCV_MQ);
  mq_close(mymq);
}

void *receiver(void *arg)
{
  char buffer[MAX_MSG_SIZE];
  void *buffptr = NULL;
  int prio;
  int nbytes;
  int waitFlag = 1;
  int id;
  
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
      memcpy(&buffptr, buffer, sizeof(void *));
      memcpy((void *)&id, &(buffer[sizeof(void *)]), sizeof(int));
      syslog(LOG_INFO, "%s received ptr msg 0x%X with priority = %d, length = %d, id = %d",__func__,
            buffptr, prio, nbytes, id);
            waitFlag = 0;
      syslog(LOG_INFO, "%s, contents of ptr = \n%s\n", __func__, (char *)buffptr);
      free(buffptr);
      syslog(LOG_INFO, "heap space memory freed\n");
    }
  }
  syslog(LOG_INFO, "%s exiting", __func__);
  return NULL;
}

void *sender(void*arg)
{
  char buffer[MAX_MSG_SIZE];
  void *buffptr = NULL;
  int id = 999;
  int prio = 30;

  /* get thread parameters */
  if(arg == NULL) {
    syslog(LOG_ERR, "invalid arg provided to %s", __func__);
    return NULL;
  }
  threadParams_t *threadParams = (threadParams_t *)arg;
  syslog(LOG_INFO, "%s started ...", __func__);

  /* send malloc'd message with priority=30 */
  buffptr = (void *)malloc(sizeof(imagebuff));
  
  /* copy imagebuff into malloc'ed frame buffer */
  memcpy(buffptr, imagebuff, IMG_SIZE);
  syslog(LOG_INFO, "Message to send = %s", (char *)buffptr);
  syslog(LOG_INFO, "%s sending %ld bytes", __func__, sizeof(buffptr) + sizeof(id));

  /* copy the frame buffer address and frame ID into the send buffer */
  memcpy(buffer, &buffptr, sizeof(void *));
  memcpy(&(buffer[sizeof(void *)]), (void *)&id, sizeof(int));

  /* send message with priority = prio */
  if(mq_send(*threadParams->pMsgQueue, buffer, MAX_MSG_SIZE, prio) == ERROR) {
    syslog(LOG_ERR, "%s, mq_send", __func__);
  } else {
    syslog(LOG_INFO, "%s, mq_send succeeded", __func__);
  }
  syslog(LOG_INFO, "%s exiting", __func__);
  return NULL;
}

void init_img_buffer(void)
{
  int i, j;
  char pixel = 'A';

  for(i=0;i<4096;i+=64) {
    pixel = 'A';
    for(j=i;j<i+64;j++) {
      imagebuff[j] = (char)pixel++;
    }
    imagebuff[j-1] = '\n';
  }
  imagebuff[4095] = '\0';
  imagebuff[63] = '\0';

  printf("buffer =\n%s", imagebuff);
}