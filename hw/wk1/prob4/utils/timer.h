/***********************************************************************************
 * @author Josh Malburg
 * joshua.malburg@colorado.edu
 * Advanced Embedded Software Development
 * ECEN5013 - Rick Heidebrecht
 * @date March 17, 2019
 * arm-linux-gnueabi (Buildroot)
 * gcc (Ubuntu)
 ************************************************************************************
 *
 * @file timer_helper.h
 *
 ************************************************************************************
 */

#ifndef CMN_TIMER_H
#define CMN_TIMER_H

#include <stdint.h>
#include <signal.h>

/**
 * @brief generic setup of posix timer
 * 
 * @param pSet pointer to set struct
 * @param pTimer pointer to timer
 * @param signum signal number
 * @param pRate timing of timer
 * @return int8_t  success of operation
 */
int8_t setupTimer(sigset_t *pSet, timer_t *pTimer, int signum, const struct timespec *pRate);

/**
 * @brief generic setup of posix timer
 * 
 * @param stop stop time
 * @param start start time
 * @param delta_t delta time
 * @return int8_t  success of operation
 */
int delta_t(struct timespec *stop, struct timespec *start, struct timespec *delta_t);

#endif /* CMN_TIMER_H */