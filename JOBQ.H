/*
 *
 * jobq.h
 * Thursday, 12/22/1994.
 *
 * (C) 1995 Info Tech Inc.
 * Craig Fitzgerald
 *
 * Part of the BAMS Job Server
 * This module contains functions which manipulate the
 * Pending Job Q, and the Running Jobs List.
 */


void SetJobQGlobals (void);

/*
 * Call me at startup
 * This creates the Q handlers
 * Woe is he who fails to call me
 */
BOOL InitQReader (void);


/*
 * Add to the job Queue
 * If priority is implemented, this fn will handle it
 *
 */
USHORT JQPut (PJOB pjQ);


/*
 * Gets the next job from the job Queue
 * this is a blocking call
 *
 */
PJOB JQGet (void);
     

/*
 * Deletes a job from the queue
 * job can be at any position
 */
BOOL JQDelete (PJOB pj);


/*
 * Allow access to ***READ*** the queue
 * This does NOT allow write permission
 * This fn returns TRUE on success
 *
 */
PJOB JQAccess (BOOL bGrant);


/*
 * Returns TRUE if Queue is full
 *
 */
BOOL JQFull (void);


/*
 * Returns # of jobs in the queue
 *
 */
USHORT JQSize (void);



/*
 * Grants access to the Running Jobs List
 * Use Like JQAccess
 */
PJOB JLAccess (BOOL bGrant);


/*
 * Returns # of jobs in the running jobs list
 *
 */
USHORT JLSize (void);


