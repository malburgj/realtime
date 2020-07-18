/***********************************************************************************
 * @author Joshua Malburg
 * joshua.malburg@colorado.edu
 * Real-time Embedded Systems
 * ECEN5623 - Sam Siewert
 * @date 12Jul2020
 * Ubuntu 18.04 LTS and Jetbot
 ************************************************************************************
 *
 * @file prob5.c
 * @brief image processing example
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

/* opencv headers */
#include <opencv2/core.hpp>     // Basic OpenCV structures (cv::Mat, Scalar)
#include <opencv2/imgproc.hpp>  // Gaussian Blur
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>  // OpenCV window I/O

#include <iostream> // for standard I/O
#include <string>   // for strings
#include <iomanip>  // for controlling float print precision
#include <sstream>  // string to number conversion

using namespace cv;
using namespace std;

/*---------------------------------------------------------------------------------*/
/* MACROS / TYPES / CONST */
#define NUM_THREADS         	        (2)
#define TIMESPEC_TO_MSEC(time)	      ((((float)time.tv_sec) * 1.0e3) + (((float)time.tv_nsec) * 1.0e-6))
#define CALC_DT_MSEC(newest, oldest)  (TIMESPEC_TO_MSEC(newest) - TIMESPEC_TO_MSEC(oldest))
#define MAX_MSG_SIZE                  (sizeof(cv::Mat))
#define ERROR                         (-1)
#define READ_THEAD_NUM 			          (0)
#define PROC_THEAD_NUM 			          (READ_THEAD_NUM + 1)
#define MAX_IMG_ROWS                  (480)
#define MAX_IMG_COLS                  (640)
#define MAX_ITERATIONS                (31)
#define FILTER_SIZE                   (31)
#define FILTER_SIGMA                  (2.0)

typedef enum {
  USE_GAUSSIAN_BLUR,
  USE_FILTER_2D,
  USE_SEP_FILTER_2D
} FilterType_e;

typedef struct {
  int threadIdx;              /* thread id */
  int cameraIdx;              /* index of camera */
  char msgQueueName[64];      /* message queue */
  unsigned int decimateFactor;
  FilterType_e filterMethod;
} threadParams_t;

/*---------------------------------------------------------------------------------*/
/* PRIVATE FUNCTIONS */
void *procImgTask(void *arg);
void *readImgTask(void *arg);

int set_attr_policy(pthread_attr_t *attr, int policy, uint8_t priorityOffset);
int set_main_policy(int policy, uint8_t priorityOffset);
void print_scheduler(void);

/*---------------------------------------------------------------------------------*/
/* GLOBAL VARIABLES */
int gAbortTest = 0;
const char *msgQueueName = "/image_mq";

/*---------------------------------------------------------------------------------*/

int main(int argc, char *argv[])
{
  /* starting logging; use cat /var/log/syslog | grep prob4
   * to view messages */
  openlog("prob5", LOG_PID | LOG_NDELAY | LOG_CONS, LOG_USER);
  syslog(LOG_INFO, ".");
  syslog(LOG_INFO, "..");
  syslog(LOG_INFO, "...");
  syslog(LOG_INFO, "logging started");

  if (argc < 3) {
    syslog(LOG_ERR, "incorrect number of arguments provided");
    cout  << "incorrect number of arguments provided\n\n"
          << "Usage: prob5 [decimation factor] [filter type]\n"
          << "decimation factor[0 = original size, 1 = half size, 2 = quarter size]\n"
          << "filter type [0 = gaussionBlur(), 1 = filter2D(), 2 = sepFilter2D()\n";
    return -1;
  }
  
  /*---------------------------------------*/
  /* setup common message queue */
  /*---------------------------------------*/

  /* ensure MQs properly cleaned up before starting */
  mq_unlink(msgQueueName);
  if(remove(msgQueueName) == -1 && errno != ENOENT) {
    syslog(LOG_ERR, "couldn't clean queue");
    return -1;
  }
	  
  struct mq_attr mq_attr;
  memset(&mq_attr, 0, sizeof(struct mq_attr));
  mq_attr.mq_maxmsg = 10;
  mq_attr.mq_msgsize = MAX_MSG_SIZE;
  mq_attr.mq_flags = 0;

  /* create queue here to allow main to do clean up */
  mqd_t mymq = mq_open(msgQueueName, O_CREAT, S_IRWXU, &mq_attr);
  if(mymq == (mqd_t)ERROR) {
    syslog(LOG_ERR, "couldn't create queue");
    return -1;
  }

  


  /*----------------------------------------------*/
  /* set scheduling policy of main and threads */
  /*----------------------------------------------*/
  print_scheduler();
  set_main_policy(SCHED_FIFO, 0);
  print_scheduler();
  pthread_attr_t thread_attr;
  set_attr_policy(&thread_attr, SCHED_FIFO, 0);

  /*---------------------------------------*/
  /* create threads */
  /*---------------------------------------*/
  pthread_t threads[NUM_THREADS];
  threadParams_t threadParams;
  strcpy(threadParams.msgQueueName, msgQueueName);
  if((threadParams.decimateFactor = atoi(argv[1])) > 2) {
    syslog(LOG_ERR, "invalid decimation factor provided");
    cout  << "invalid decimation factor provided\n\n"
          << "Usage: prob5 [decimation factor] [filter type]\n"
          << "decimation factor[0 = original size, 1 = half size, 2 = quarter size]\n"
          << "filter type [0 = gaussionBlur(), 1 = filter2D(), 2 = sepFilter2D()\n";
    return -1;
  } else {
    threadParams.decimateFactor = pow(2.0, threadParams.decimateFactor);
  }
  if((threadParams.filterMethod = (FilterType_e)atoi(argv[2])) > 2) {
    syslog(LOG_ERR, "invalid filter type provided");
    cout  << "invalid filter type provided\n\n"
          << "Usage: prob5 [decimation factor] [filter type]\n"
          << "decimation factor[0 = original size, 1 = half size, 2 = quarter size]\n"
          << "filter type [0 = gaussionBlur(), 1 = filter2D(), 2 = sepFilter2D()\n";
    return -1;
  }

  syslog(LOG_INFO, "decimation factor: %d", threadParams.decimateFactor);
  syslog(LOG_INFO, "filter type: %d", threadParams.filterMethod);

  threadParams.cameraIdx = 0;
  threadParams.threadIdx = READ_THEAD_NUM;
  if(pthread_create(&threads[READ_THEAD_NUM], &thread_attr, readImgTask, (void *)&threadParams) != 0) {
    syslog(LOG_ERR, "couldn't create thread#%d", READ_THEAD_NUM);
  }

  threadParams.threadIdx = PROC_THEAD_NUM;
  if(pthread_create(&threads[PROC_THEAD_NUM], &thread_attr, procImgTask, (void *)&threadParams) != 0) {
    syslog(LOG_ERR, "couldn't create thread#%d", PROC_THEAD_NUM);
  }

  /*----------------------------------------------*/
  /* run main */
  /* wait 10 sec kill threads */
  /*----------------------------------------------*/
  sleep(10);
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
  mq_unlink(msgQueueName);
  mq_close(mymq);
}

void *procImgTask(void *arg)
{
  Mat inputImg;
  unsigned int prio;
  int nbytes;
  int id;
  struct timespec prevTime, readTime, procTime;
  int cnt = 0;
  
  /* get thread parameters */
  if(arg == NULL) {
    syslog(LOG_ERR, "ERROR: invalid arg provided to %s", __func__);
    return NULL;
  }
  threadParams_t threadParams = *(threadParams_t *)arg;

  /* open handle to queue */
  mqd_t msgQueue = mq_open(threadParams.msgQueueName, O_RDONLY, 0666, NULL);
  if(msgQueue == -1) {
    syslog(LOG_ERR, "%s couldn't open queue", __func__);
    cout << __func__<< " couldn't open queue" << endl;
    return NULL;
  }

  Mat kern1D = getGaussianKernel(FILTER_SIZE, FILTER_SIGMA, CV_32F);
  Mat kern2D = kern1D * kern1D.t();

  syslog(LOG_INFO, "%s started ...", __func__);
  float cumTime = 0.0f;
  float cumProcTime = 0.0f;
  const float deadline_ms = 70.0f;
  float maxJitter_ms = 0.0f;
  float cumJitter_ms;
  clock_gettime(CLOCK_MONOTONIC, &prevTime);
  while(!gAbortTest) {
    /* read oldest, highest priority msg from the message queue */
    if(mq_receive(msgQueue, (char *)&inputImg, MAX_MSG_SIZE, &prio) < 0) {
      /* don't print if queue was empty */
      if(errno != EAGAIN) {
        syslog(LOG_ERR, "%s error with mq_receive, errno: %d [%s]", __func__, errno, strerror(errno));
      }
    } else {
      /* process image */
      clock_gettime(CLOCK_MONOTONIC, &procTime);
      if(threadParams.filterMethod == USE_GAUSSIAN_BLUR) {
        GaussianBlur(inputImg, inputImg, Size(FILTER_SIZE, FILTER_SIZE), FILTER_SIGMA);
      } else if (threadParams.filterMethod == USE_FILTER_2D) {
        filter2D(inputImg, inputImg, CV_8U, kern2D);
      } else {
        sepFilter2D(inputImg, inputImg, CV_8U, kern1D, kern1D);
      }
      clock_gettime(CLOCK_MONOTONIC, &readTime);
      if(cnt > 0) {
        cumProcTime += CALC_DT_MSEC(readTime, procTime);
        cumTime += CALC_DT_MSEC(readTime, prevTime);
        cumJitter_ms += deadline_ms - CALC_DT_MSEC(readTime, prevTime);
        if (CALC_DT_MSEC(readTime, prevTime) > deadline_ms) {
          syslog(LOG_ERR, "deadline missed: %f", CALC_DT_MSEC(readTime, prevTime));
        }
      }
      ++cnt;
      prevTime = readTime;
    }
  }
  /* ignore first frame */
  syslog(LOG_INFO, "avg frame time: %f msec",cumTime / (cnt - 1));
  syslog(LOG_INFO, "avg proc time: %f msec", cumProcTime / (cnt - 1));
  syslog(LOG_INFO, "avg jitter: %f msec", cumJitter_ms / (cnt - 1));
  
  /* save am image for comparison later */
  char filename[80];
  sprintf(filename,"filt%d_Size%d.jpg",threadParams.filterMethod, threadParams.decimateFactor);
  imwrite(filename, inputImg);

  syslog(LOG_INFO, "%s exiting", __func__);
  mq_close(msgQueue);
  return NULL;
}

void *readImgTask(void*arg)
{
  unsigned int cnt = 0;
  unsigned int prio = 30;
  Mat readImg;
  struct timespec expireTime;
  struct timespec startTime;

  /* get thread parameters */
  if(arg == NULL) {
    syslog(LOG_ERR, "invalid arg provided to %s", __func__);
    return NULL;
  }
  threadParams_t threadParams = *(threadParams_t *)arg;

  /* open handle to queue */
  mqd_t msgQueue = mq_open(threadParams.msgQueueName,O_WRONLY, 0666, NULL);
  if(msgQueue == -1) {
    syslog(LOG_ERR, "%s couldn't open queue", __func__);
    cout << __func__<< " couldn't open queue" << endl;
    return NULL;
  }

  /* open camera stream */
  VideoCapture cam;
  if(!cam.open(threadParams.cameraIdx)) {
    syslog(LOG_ERR, "couldn't open camera");
    cout << "couldn't open camera" << endl;
    return NULL;
  } else {
    cam.set(CAP_PROP_FRAME_WIDTH, 640 / threadParams.decimateFactor);
    cam.set(CAP_PROP_FRAME_HEIGHT, 480 / threadParams.decimateFactor);
    cout  << "cam size (HxW): " << cam.get(CAP_PROP_FRAME_WIDTH)
          << " x " << cam.get(CAP_PROP_FRAME_HEIGHT) << endl;
  }

  syslog(LOG_INFO, "%s started ...", __func__);
  clock_gettime(CLOCK_MONOTONIC, &startTime);
  while((!gAbortTest) && (cnt < MAX_ITERATIONS)) {
    /* read image from video */
    cam >> readImg;

    /* try to insert image but don't block if full
     * so that we loop around and just get the newest */
    clock_gettime(CLOCK_MONOTONIC, &expireTime);
    if(mq_timedsend(msgQueue, (const char *)&readImg, MAX_MSG_SIZE, prio, &expireTime) != 0) {
      /* don't print if queue was empty */
      if(errno != ETIMEDOUT) {
        syslog(LOG_ERR, "%s error with mq_send, errno: %d [%s]", __func__, errno, strerror(errno));
      }
      cout << __func__ << " error with mq_send, errno: " << errno << " [" << strerror(errno) << "]" << endl;
    } else {
      ++cnt;
    }
  }
  gAbortTest = 1;
  syslog(LOG_INFO, "%s exiting", __func__);
  mq_close(msgQueue);
  return NULL;
}

void print_scheduler(void)
{
  switch (sched_getscheduler(getpid()))
  {
  case SCHED_FIFO:
    printf("Pthread Policy is SCHED_FIFO\n");
    syslog(LOG_INFO, "Pthread Policy is SCHED_FIFO");
    break;
  case SCHED_OTHER:
    printf("Pthread Policy is SCHED_OTHER\n");
    syslog(LOG_INFO, "Pthread Policy is SCHED_OTHER");
    break;
  case SCHED_RR:
    printf("Pthread Policy is SCHED_RR\n");
    syslog(LOG_INFO, "Pthread Policy is SCHED_RR");
    break;
  default:
    printf("Pthread Policy is UNKNOWN\n");
    syslog(LOG_INFO, "Pthread Policy is UNKNOWN");
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