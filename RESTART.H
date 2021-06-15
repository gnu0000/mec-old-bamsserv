/*
 *
 * restart.h
 * Wednesday, 1/18/1995.
 *
 * (C) 1995 Info Tech Inc.
 * Craig Fitzgerald
 *
 * Part of the BAMS Job Server
 * This module contains the fn to reload jobs whose ctl and data files
 * were written but have not executed.  This list of jobs is the list
 * of jobs that were queue'd when the server was shut down.  Jobs that
 * were executing at shutdown are not re-loaded
 */


/*
 * This fn looks at all the job directories in all the user directories
 * looking for Job.CTL files.  For each file found, its associated job
 * is added to the job queue.
 *
 */
void RestartServer (void);

