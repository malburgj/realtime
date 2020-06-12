/***********************************************************************************
 * @author Joshua Malburg
 * joshua.malburg@colorado.edu
 * Real-time Embedded Systems
 * ECEN5623 - Sam Siewert
 * @date 12Jun2020
 * Ubuntu 18.04 LTS and Jetbot
 ************************************************************************************
 *
 * @file schedule.h
 * @brief schedule helper functions
 *
 ************************************************************************************
 */

#ifndef SCHEDULE_H
#define SCHEDULE_H

/**
 * @brief print policy of current process
 */
void print_scheduler(void);

/**
 * @brief set schedule policy
 * 
 * @param attr pointer to thread attribute structure
 * @param policy policy to set
 * @return void
 */
void setSchedPolicy(pthread_attr_t *attr, int policy);

#endif