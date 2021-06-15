/*
 *
 * service.h
 * Thursday, 1/5/1995.
 *
 * (C) 1995 Info Tech Inc.
 * Craig Fitzgerald
 *
 * Part of the BAMS Job Server
 * This mod is where the client commands are serviced
 */



/*
 * This fn should be called with the newly created socket connection
 * via a StartThread call (This fn kills the thread when done).
 * This fn negotiates security, reads the command, and calls the 
 * appropriate command handler.  Then fn then disconnects and cleans up
 */
VOID AcceptFn (PVOID p);



VOID PurgeDaemonFn (PVOID p);

VOID GrimReaperFn (PVOID p);

/*
 * kills all running jobs
 *
 */
void KillRunningJobs (BOOL bAllowRestart);

/*
 *
 *
 */
BOOL InitServices (void);

/*********************************************************************/
/*                                                                   */
/*                                                                   */
/*                                                                   */
/*********************************************************************/

