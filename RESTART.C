/*
 *
 * restart.c
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

#define INCL_DOS
#include <os2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Var.h"
#include "Bamsserv.h"
#include "JobQ.h"
#include "UserIO.h"
#include "CtlFile.h"
#include "Util.h"
#include "Syssock.h"
#include "osock.h"
#include "Service.h"
#include "Stat.h"


/*
 * Called once per job directory
 *
 * This fn looks in a particular job dir looking for the job's
 * control file
 */
static BOOL LookForJob (PSZ pszUser, PSZ pszJob)
   {
   char szCtl [256];
   BOOL bFound;
   PJOB pjob;

   JMLog (8, "Restart: Looking for job in job dir: %s\\%s", pszUser, pszJob);
   FilesDir0 (szCtl, pszUser, pszJob, TRUE);
   strcat (szCtl, "\\");
   strcat (szCtl, JOB_CTL_FILE);

   CtlFileAccess (pszUser, TRUE);
   pjob = MakeJob ();
   if (bFound = JobCtlReadFile (szCtl, pjob))
      {
      pjob->pszUserID = strdup (pszUser);
      JQPut (pjob);
      }
   else
      FreeJob (pjob);
      
   CtlFileAccess (pszUser, FALSE);
   return !!bFound;
   }



/*
 * Called once per user directory
 *
 * This fn looks at all the job directories in all the user directories
 * looking for Job.CTL files.  For each file found, its associated job
 * is added to the job queue.
 *
 */
static USHORT AddJobs (PSZ pszBase, PSZ pszUser)
   {
   HDIR     hdir = 0;
   char     szSpec [256];
   char     szMatch [256];
   USHORT   uJobs = 0;

   JMLog (3, "Restart: Looking for jobs in user dir: %s", pszUser);
   sprintf (szSpec, "%s\\%s\\*.*", pszBase, pszUser);
   while (FindAFile (szMatch, &hdir, szSpec, FILE_DIRECTORY, NULL))
      uJobs += LookForJob (pszUser, szMatch);

   return uJobs;
   }


/*
 * This fn looks at all the job directories in all the user directories
 * looking for Job.CTL files.  For each file found, its associated job
 * is added to the job queue.
 *
 */
void RestartServer (void)
   {
   HDIR     hdir = 0;
   char     szBase [256];
   char     szSpec [256];
   char     szMatch[256];
   USHORT   uJobs = 0;
   PSTAT    pstat;

   /*--- give all q reader threads a chance to start first ---*/
   DosSleep (1000);

   VarGet (pvCFG, "FilesDir", szBase);
   sprintf (szSpec, "%s\\*.*", szBase);

   JMLog (6, "Restart: SearchSpec: %s", szSpec);
   while (FindAFile (szMatch, &hdir, szSpec, FILE_DIRECTORY, NULL))
      uJobs += AddJobs (szBase, szMatch);

   JMLog (3, "Restart: %u Jobs Returned to Queue", uJobs);

   pstat = StatAccess (TRUE);
   pstat->uJobsRestarted = uJobs;
   pstat = StatAccess (FALSE);
   }

