/*
 *
 * ctlfile.h
 * Wednesday, 1/18/1995.
 *
 * (C) 1995 Info Tech Inc.
 * Craig Fitzgerald
 *
 * Part of the BAMS Job Server
 * This module contains functions to read/write the user and job ctl files
 * and to create new job ID's
 */


/*************************************************************************/
/*                                                                       */
/* JOB Control file manipulation                                         */
/*                                                                       */
/*************************************************************************/

/*
 * Creates a JOB control file
 *
 */
BOOL JobCtlCreateFile (PSZ szJobCtlFile, PJOB pjob);


/*
 * Reads a JOB control file
 *
 */
BOOL JobCtlReadFile (PSZ szJobCtlFile, PJOB pjob);


/*
 * renames a JOB control file
 *
 */
BOOL JobCtlRenameFile (PSZ szJobCtlFile);


/*************************************************************************/
/*                                                                       */
/* USER Control file manipulation                                        */
/*                                                                       */
/*************************************************************************/

/*
 * protect file usage for USER control file
 * Only needed with CtlReadLine
 * for right now, I use 10 semaphores to control
 * access to the user control files. Later we should
 * enhance this semaphore usage but hey, it works better
 * than a single semaphore right?
 */
BOOL CtlFileAccess (PSZ pszUser, BOOL bGrant);


/*
 * takes care of file access mux
 * call with fpCtl=NULL to open file and return handle
 * call with valid fpCtl to close file and free mux
 *
 */
FILE *CtlFileHandle (PSZ pszUserID, PSZ pszMode, FILE *fpCtl);


/* 
 * must set job values to NULL at end
 *
 */
BOOL CtlReadLine (FILE *fpCtl, PJOB pjob);


/*
 * This fn removes an entry from the ctl file
 *
 */
BOOL CtlRemoveLine (PSZ pszUserID, PSZ pszJobID);


/*
 * transparently controlls file access, and ctl location
 *
 */
BOOL CtlWriteLine (PJOB pjob);


/*
 * This fn generates a new job ID
 * This fn updates the last-id ctl file
 *
 */
PSZ CreateNewJobID (PCOMMAND pcmd, PSZ pszJobID);


/*
 * purges Users jobs created before datetime dt
 *
 */
USHORT CtlPurgeJobs (PSZ pszUserID, ULONG dt);

