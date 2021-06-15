/*
 *
 * jobq.c
 * Thursday, 12/22/1994.
 *
 * (C) 1995 Info Tech Inc.
 * Craig Fitzgerald
 *
 *
 * Part of the BAMS Job Server
 * This module contains functions which manipulate the
 * Pending Job Q, and the Running Jobs List.
 */

#define INCL_DOS
#include <os2.h>
#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include <Time.h>
#include "Var.h"
#include "BamsServ.h"
#include "UserIO.h"
#include "Util.h"
#include "RunJob.h"
#include "JobQ.h"
#include "Stat.h"


PJOB pjQHead = NULL;   /*-- get Q elements from here             --*/ 
PJOB pjQTail = NULL;   /*-- add Q elements here                  --*/

PJOB pjJobList = NULL; /*-- Job List elements list               --*/

ULONG semQMux = 0;
ULONG semQAdd = 0;     /*-- Q event: a record is added to the q  --*/
ULONG semJMux = 0;     /*-- Mutex access to the running job list --*/

USHORT uMAXQSIZE = 0;


/*********************************************************************/
/*                                                                   */
/*   Queue Maintenance functions                                     */
/*                                                                   */
/*********************************************************************/
/*                                                                   */
/*       pjQTail            pjQHead                                  */
/*       |                   |                                       */
/*       |                   |                                       */
/*       v                   v                                       */
/* ||<--pj<--pj<--pj<--pj<--pj                                       */
/*                                                                   */
/*********************************************************************/
 

BOOL JQFull () 
   {
   USHORT uSize;
   PSTAT  pstat;

   uSize = JQSize();
   pstat = StatAccess (TRUE);
   pstat->uMaxJobsInQueue = max (pstat->uMaxJobsInQueue, uSize);
   pstat = StatAccess (FALSE);

   return (uMAXQSIZE && uSize >= uMAXQSIZE);
   }


/*
 * compares the priority of 2 jobs
 * pj1 is the brand new job 
 * pj2 is the old job already in the list
 * the pj2 priority may be bumped up due to time in the queue
 */
static LONG PrtyCmp (PJOB pj1, PJOB pj2, LONG lBumpTime)
   {
   LONG lPrty2;

   if (!lBumpTime)
      lPrty2 = (LONG)pj2->uPriority;
   else
      lPrty2 = (LONG)pj2->uPriority + (time(NULL) - pj2->tmQueueTime)/lBumpTime;
   return ((LONG)pj1->uPriority - lPrty2);
   }



/*
 * Adds job to job queue
 * Takes priority into consideration
 * assumes semaphore protection already done
 */
static BOOL AddToQ (PJOB pj)
   {
   LONG lBumpTime;
   PJOB pjTmp, pjPrev;

   lBumpTime = VarGetl (pvCFG, "PriorityBumpTime");
              
   pjPrev = NULL;
   for (pjTmp = pjQHead; pjTmp; pjTmp = pjTmp->next)
      {
      if (PrtyCmp (pj, pjTmp, lBumpTime) > 0)
         break;
      pjPrev = pjTmp;
      }
   if (!(pj->next = pjTmp)) // If added to end, fix tail ptr
      pjQTail = pj;         //
   if (pjPrev)              //
      pjPrev->next = pj;    // added to Q at !(Front of Q)
   else                     //
      pjQHead = pj;         // added to front of Q
   return TRUE;
   }



/*
 * Add to the job Queue
 * 
 *
 */
USHORT JQPut (PJOB pj)
   {
   PSTAT  pstat;

   JMLog (6, "Putting Job On Queue. Job:%s", pj->pszJobID);

   if (JQFull ())
      {
      JMLog (3, "Job Refused: Queue is Full.  Job:%s", pj->pszJobID);
      pstat = StatAccess (TRUE);
      pstat->uJobsRefused++;
      pstat = StatAccess (FALSE);
      return JSE_REFUSED;
      }
   DosSemRequest (&semQMux, SEM_INDEFINITE_WAIT);

   AddToQ (pj);

   pstat = StatAccess (TRUE);
   pstat->uJobsQueued++;
   pstat = StatAccess (FALSE);

   DosSemClear (&semQMux);      // done with q
   DosSemClear (&semQAdd);      // signify that an element was added
   UpdateWindow (JM_QUEUE_WINDOW, NULL);
   return 0;
   }



/*
 * Gets the next job from the job Queue
 * this is a blocking call
 *
 */
PJOB JQGet (void)
   {
   PJOB pj;

   DosSemRequest (&semQMux, SEM_INDEFINITE_WAIT);
   if (pj = pjQHead) // assignment!
      {
      pjQHead = pjQHead->next;
      pj->next = NULL;
      }
   if (!pjQHead)  
      pjQTail = NULL;

   DosSemClear (&semQMux);
   UpdateWindow (JM_QUEUE_WINDOW, NULL);
   return pj;
   }
     


/*
 * Deletes a job from the queue
 * job can be at any position
 * this fn does not free the job
 *
 * THIS FN ASSUMES YOU ALREADY HAVE Q ACCESS VIA JQAccess!
 *
 */
BOOL JQDelete (PJOB pjDel)
   {
   PJOB pj, pjPrev = NULL;

   JMLog (7, "In JQDelete. Job:%s", pjDel->pszJobID);

   for (pj = pjQHead; pj; pj = pj->next)
      {
      if (pj == pjDel)
         {
         if (pjPrev)
            pjPrev->next = pj->next;
         if (pj == pjQHead)
            pjQHead = pjQHead->next;
         if (pj == pjQTail)
            pjQTail = pjPrev;

         JMLog (6, "Deleted Job From Queue. Job:%s", pjDel->pszJobID);
         UpdateWindow (JM_QUEUE_WINDOW, NULL);
         return TRUE;
         }
      pjPrev = pj;
      }
   UpdateWindow (JM_QUEUE_WINDOW, NULL);
   JMLog (6, "Cannot Delete: Job Not Found in Q. Job:%s", pjDel->pszJobID);
   return FALSE;
   }



/*
 * This is a wrapper for the Job Q access semaphore
 * 
 *
 */
PJOB JQAccess (BOOL bGrant)
   {
   if (bGrant)
      {
      DosSemRequest (&semQMux, SEM_INDEFINITE_WAIT);
      return pjQHead;
      }
   else
      {
      DosSemClear (&semQMux);
      return NULL;
      }
   }



/*
 * This fn simply returns TRUE if the Job Q is empty
 *
 *
 */
BOOL JQEmpty (void)
   {
   BOOL bRet;

   DosSemRequest (&semQMux, SEM_INDEFINITE_WAIT);
   bRet = !pjQHead;
   DosSemClear (&semQMux);
   return bRet;
   }


/*
 * This fn returns the number of jobs in the job q
 *
 *
 */
USHORT JQSize (void)
   {
   PJOB pj;
   USHORT uCount = 0;

   DosSemRequest (&semQMux, SEM_INDEFINITE_WAIT);
   for (pj = pjQHead; pj; pj = pj->next)
      uCount++;
   DosSemClear (&semQMux);

   JMLog (10, "Job Q Size = %d", (ULONG) uCount);
   return uCount;
   }


/*********************************************************************/
/*                                                                   */
/* Running Job List maintenance functions                            */
/*                                                                   */
/*********************************************************************/

/*
 * Adds a job to the running job list
 *
 */
USHORT JLAdd (PJOB pj)
   {
   PSTAT  pstat;

   DosSemRequest (&semJMux, SEM_INDEFINITE_WAIT);

   pj->next  = pjJobList;
   pjJobList = pj;

   DosSemClear (&semJMux);       // done with q

   pstat = StatAccess (TRUE);
   pstat->uJobsStarted++;
   pstat = StatAccess (FALSE);

   UpdateWindow (JM_RUNNING_WINDOW, NULL);
   JMLog (7, "Job Added to running List Job:%s", pj->pszJobID);
   return 0;
   }



/*
 * Removes a job from the running job list
 *
 */
BOOL JLRemove (PJOB pjDel)
   {
   PJOB pj, pjPrev = NULL;
   BOOL bFound = FALSE;

   JMLog (7, "In JLRemove. Job:%s", pjDel->pszJobID);

   DosSemRequest (&semJMux, SEM_INDEFINITE_WAIT);

   for (pj = pjJobList; pj && !bFound; pj = pj->next)
      {
      if (pj == pjDel)
         {
         if (pjPrev)
            pjPrev->next = pj->next;
         else                    // (pj == pjJobList) true by logic
            pjJobList = pj->next;
         bFound = TRUE;
         pj->next = NULL;
         }
      pjPrev = pj;
      }
   DosSemClear (&semJMux);
   UpdateWindow (JM_RUNNING_WINDOW, NULL);
   return bFound;
   }



/*
 * This is a wrapper for the Running Job List access semaphore
 * 
 *
 */
PJOB JLAccess (BOOL bGrant)
   {
   if (bGrant)
      {
      DosSemRequest (&semJMux, SEM_INDEFINITE_WAIT);
      return pjJobList;
      }
   else
      {
      DosSemClear (&semJMux);
      return NULL;
      }
   }


/*
 * returns the size of the running jobs list
 *
 */
USHORT JLSize (void)
   {
   PJOB pj;
   USHORT uCount = 0;

   DosSemRequest (&semJMux, SEM_INDEFINITE_WAIT);
   for (pj = pjJobList; pj; pj = pj->next)
      uCount++;
   DosSemClear (&semJMux);
   JMLog (10, "Running Job List Size = %d", (ULONG) uCount);
   return uCount;
   }


/*********************************************************************/
/*                                                                   */
/*                                                                   */
/*                                                                   */
/*********************************************************************/



/*
 * This fn sets the pjob->tmMaxRunTime for a job that is about to run
 *
 */
void SetMaxRunningLife (PJOB pjob)
   {
   char szBuff[128];
   char szBuff2[128];
   PSZ  psz;

   if (!pjob || !pjob->pszJobName)
      return;

   /*--- determine timeout interval ---*/
   sprintf (szBuff, "%s_TimeOut", pjob->pszJobName);
   psz = (VarGet (pvCFG, szBuff, szBuff2) ? szBuff : "Default_TimeOut");
   pjob->tmMaxRunTime = VarGetl (pvCFG, psz);
   JMLog (7, "Grim Reaper: Using %s=%lu max runtime for %s", psz, pjob->tmMaxRunTime, pjob->pszJobName);
   }



/*
 * This fn runs a job
 * 1> Adds the job to the running jobs list
 * 2> pre-processes the job (prep)
 * 3> processes the job (execs)
 * 4> post-processes the job (print, etc...)
 * 5> Removes the job from the running jobs list
 * 6> Free's the job
 */
void HandleJobRequest (PJOB pjob)
   {
   USHORT uRet;
   PSTAT  pstat;

   time ((time_t *)&pjob->tmStartTime); // set jobs start time

   SetMaxRunningLife (pjob);  // determine how long job can run

   JLAdd (pjob);              // add to running job list

   /*--- pre process ---*/
   uRet = PreProcessJob (pjob);

   /*--- process job ---*/
   if (!uRet)
      uRet = ProcessJob (pjob);

   JLRemove (pjob);           // remove from running job list

//   /*--- post process ---*/
//   if (!uRet)
      uRet = PostProcessJob (pjob);


   pstat = StatAccess (TRUE);
   pstat->tmTotalQueueTime += pjob->tmStartTime - pjob->tmQueueTime;
   pstat->tmTotalRunTime   += pjob->tmEndTime   - pjob->tmStartTime;
   pstat->uJobsCompleted++;
   pstat = StatAccess (FALSE);

   if (uRet)
      JMLog (6, "Aborted job %s", pjob->pszJobID);
   else
      JMLog (6, "Completed job %s", pjob->pszJobID);

   FreeJob (pjob);

#if defined (__IBMC__)
   _heapmin ();
#endif 
   }


/*
 * This fn is the home of all q reader threads
 *
 * This function monitors the Job Q
 * This fn is called via _beginthread (it does not return)
 * This fn reads entries off the q and handles them
 *
 */
VOID QReaderFn (PVOID p)
   {
   PJOB   pjob;
   USHORT i=0;

   while (TRUE)
      {
      DosSemSetWait (&semQAdd, SEM_INDEFINITE_WAIT);     /*--- wait for q to get data ---*/

      while (!JQEmpty())              /*--- while q has data ---*/
         {
         if (pjob = JQGet ())
            HandleJobRequest (pjob);

         i++; //this is only here so i can set a CVP breakpoint

         JMLog (10, "Thread n going ...");
         }
      }
   }



/*
 *
 *
 *
 */
void SetJobQGlobals (void)
   {
   uMAXQSIZE = (USHORT) VarGetl (pvCFG, "MaxQueueSize");
   }


/*
 *  This is the init for the q manager
 *
 *
 */
BOOL InitQReader (void)
   {
   USHORT i, uMaxJobs;

   uMaxJobs = (USHORT) VarGetl (pvCFG, "MaxProcesses");

   SetJobQGlobals ();

   for (i=0; i<uMaxJobs; i++)
      StartThread (QReaderFn, i);

   JMLog (6, "Init Q Reader: %d Reader thread%s created", i, (i>1 ? "s": ""));
   return TRUE;
   }

