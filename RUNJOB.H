/*
 *
 * dojob.h
 * Thursday, 1/12/1995.
 *
 * (C) 1995 Info Tech Inc.
 * Craig Fitzgerald
 *
 * Part of the BAMS Job Server
 * This module contains the fn to pre-process, process, and post-process
 * a job.
 */


/*
 * This fn prepares a Job To Run
 *
 */
USHORT PreProcessJob (PJOB pjob);


/*
 * This Fn actually runs the Job
 *
 */
USHORT ProcessJob (PJOB pjob);


/*
 * This fn does cleanup, and prints the Job
 *
 */
USHORT PostProcessJob (PJOB pjob);


