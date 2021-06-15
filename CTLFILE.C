/*
 *
 * ctlfile.c
 * Monday, 1/9/1995.
 *
 * (C) 1995 Info Tech Inc.
 * Craig Fitzgerald
 *
 *
 * Part of the BAMS Job Server
 * This module contains functions to read/write the user and job ctl files
 * and to create new job ID's
 */

#define INCL_DOS
#include <os2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <io.h>
#include <GnuFile.h>
#include <GnuStr.h>
#include <GnuMem.h>
#include "Var.h"
#include "BAMSServ.h"
#include "Util.h"
#include "CtlFile.h"


ULONG psemCtlMux [10] = {0,0,0,0,0,0,0,0,0,0};


/*************************************************************************/
/*                                                                       */
/* JOB Control file manipulation                                         */
/*                                                                       */
/*************************************************************************/


/*
 * Creates a JOB control file
 *
 *
 */
BOOL JobCtlCreateFile (PSZ szJobCtlFile, PJOB pjob)
   {
   FILE *fp;
   char szBuff[128];

   JMLog (7, "creating Job Ctl File %s", szJobCtlFile);
   if (!(fp = my_fopen (szJobCtlFile, "w")))
      {
      JMError ("Unable to create Job Ctl File %s", szJobCtlFile);
      return FALSE;
      }
   fprintf (fp, "JobID=%s\n",       pjob->pszJobID  );
   fprintf (fp, "JobName=%s\n",     pjob->pszJobName);
   fprintf (fp, "JobTag=%s\n",      pjob->pszJobTag );
   fprintf (fp, "JobDesc=%s\n",     pjob->pszJobDesc);
   fprintf (fp, "ClientID=%s\n",    pjob->pszClientID );
   fprintf (fp, "MessagePort=%lu\n",pjob->lMessagePort);
   fprintf (fp, "QueueTime=%s\n",   TimeString (szBuff, pjob->tmQueueTime));
   fprintf (fp, "QueueDate=%s\n",   DateString (szBuff, pjob->tmQueueTime));
   fprintf (fp, "BinTime=%lu\n",    pjob->tmQueueTime);
   fprintf (fp, "Priority=%u\n",    pjob->uPriority  );
   fclose (fp);
   return TRUE;
   }


/*
 * Reads a JOB control file
 *
 */
BOOL JobCtlReadFile (PSZ szJobCtlFile, PJOB pjob)
   {                         
   PVAR   pv     = NULL;
   USHORT uError = 0;
   PSZ    psz;
   CHAR   szBuff[256];

   if (!VarReadFile (&pv, szJobCtlFile))
      return FALSE;

   pjob->tmStartTime  = 0;
   pjob->tmEndTime    = 0;
   pjob->uRetCode     = 0;
   pjob->iPID         = 0;
   pjob->pv           = NULL;
   pjob->pszUserID    = NULL;
   pjob->lMessagePort = VarGetl(pv, "MessagePort");
   pjob->tmQueueTime  = VarGetl(pv, "BinTime"    );
   pjob->uPriority    = (USHORT)VarGetl(pv, "Priority");

   if (!(psz = VarGet (pv, "JobID", szBuff)))
      uError++;
   pjob->pszJobID = strdup (psz);
   if (!(psz = VarGet (pv, "JobName", szBuff)))
      uError++;
   pjob->pszJobName = strdup (psz);
   if (!(psz = VarGet (pv, "JobTag", szBuff)))
      uError++;
   pjob->pszJobTag = strdup (psz);
   if (!(psz = VarGet (pv, "JobDesc", szBuff)))
      uError++;
   pjob->pszJobDesc = strdup (psz);
   if (!(psz = VarGet (pv, "ClientID", szBuff)))
      uError++;
   pjob->pszClientID = strdup (psz);

   FreeVar (pv);
   return !uError;
   }


/*************************************************************************/
/*                                                                       */
/* USER Control file manipulation                                        */
/*                                                                       */
/*************************************************************************/


/*
 * protect file usage for USER control file
 * for right now, I use 10 semaphores to control
 * access to the user control files. Later we should
 * enhance this semaphore usage but hey, it works better
 * than a single semaphore right?
 */
BOOL CtlFileAccess (PSZ pszUser, BOOL bGrant)
   {
   USHORT uHash;

   /*--- Code by SuperKludge (tm) ---*/
   uHash = *pszUser % 10;

   if (bGrant)
      DosSemRequest (psemCtlMux + uHash, SEM_INDEFINITE_WAIT);
   else
      DosSemClear (psemCtlMux + uHash);
   return bGrant;
   }



/*
 * call with fpCtl=NULL to open file and return handle
 * call with valid fpCtl to close file and free mux
 *
 */
FILE *CtlFileHandle (PSZ pszUserID, PSZ pszMode, FILE *fpCtl)
   {
   char szBuff    [256];
   char szUser    [256];
   char szCtlFile [256];
   FILE *fp;

   if (!fpCtl)
      {
      CtlFileAccess (pszUserID, TRUE);
      MakeValidDirName (szUser, pszUserID);
      sprintf (szCtlFile, "%s\\%s.CTL", UserDir (szBuff, pszUserID), szUser);

      if (!(fp = my_fopen (szCtlFile, (pszMode ? pszMode : "r"))))
         CtlFileAccess (pszUserID, FALSE);
      return fp;
      }
   fclose (fpCtl);
   CtlFileAccess (pszUserID, FALSE);
   return NULL;
   }



/*
 * This fn removes an entry from the ctl file
 *
 */
BOOL CtlRemoveLine (PSZ pszUserID, PSZ pszJobID)
   {
   char szBuff    [256];
   char szCtlFile [256];
   char szTmpFile [256];
   char szUser    [256];
   char szLine    [256];
   BOOL bFound = FALSE;
   FILE *fpIn, *fpOut;


   MakeValidDirName (szUser, pszUserID);
   sprintf (szCtlFile, "%s\\%s.CTL", UserDir (szBuff, pszUserID), szUser);
   NewExtension (szTmpFile, szCtlFile, ".TMP");

   CtlFileAccess (pszUserID, TRUE);
   if (!(fpIn = my_fopen (szCtlFile, "r")))
      {
      CtlFileAccess (pszUserID, FALSE);
      return FALSE;
      }
   if (!(fpOut = my_fopen (szTmpFile, "w")))
      {
      fclose (fpIn);
      CtlFileAccess (pszUserID, FALSE);
      return FALSE;
      }

   while (FilReadLine (fpIn, szLine, ";", sizeof szLine) != 0xFFFF)
      {
      StrStrip (szLine, " \t");
      if (strnicmp (szLine, pszJobID, strlen (pszJobID)))
         fprintf (fpOut, "%s\n", szLine);
      else
         bFound = TRUE;
      }
   fclose (fpIn);
   fclose (fpOut);

   if (bFound)
      {
      unlink (szCtlFile);
      rename (szTmpFile, szCtlFile);
      }
   CtlFileAccess (pszUserID, FALSE);
   return bFound;
   }



USHORT CtlPurgeJobs (PSZ pszUserID, ULONG dt)
   {
   char   szBuff    [256];
   char   szCtlFile [256];
   char   szTmpFile [256];
   char   szUser    [256];
   char   szLine    [256];
   USHORT uCols, uCount = 0;
   FILE   *fpIn, *fpOut;
   PSZ    *ppsz;

   MakeValidDirName (szUser, pszUserID);
   sprintf (szCtlFile, "%s\\%s.CTL", UserDir (szBuff, pszUserID), szUser);
   NewExtension (szTmpFile, szCtlFile, ".TMP");

   CtlFileAccess (pszUserID, TRUE);
   if (!(fpIn = my_fopen (szCtlFile, "r")))
      {
      CtlFileAccess (pszUserID, FALSE);
      return 0;
      }

   if (!(fpOut = my_fopen (szTmpFile, "w")))
      {
      fclose (fpIn);
      CtlFileAccess (pszUserID, FALSE);
      return 0;
      }

   while (FilReadLine (fpIn, szLine, ";", sizeof szLine) != 0xFFFF)
      {
      ppsz = StrMakePPSZ (szLine, ",\n", TRUE, TRUE, &uCols);

      if (!ppsz || (uCols<7))                    // incorrect format - pass
         fprintf (fpOut, "%s\n", szLine);
      else if (!ppsz[6] || ((ULONG)atoi (ppsz[6]) > dt)) // too new - pass
         fprintf (fpOut, "%s\n", szLine);
      else
         {
         uCount++;
         JMLog (4, "Purging job: %s, user %s", ppsz[0], pszUserID);
         RemoveDir (FilesDir0 (szBuff, pszUserID, ppsz[0], 2), TRUE);
         }
      MemFreePPSZ (ppsz, uCols);
      }
   fclose (fpIn);
   fclose (fpOut);

   if (uCount)
      {
      unlink (szCtlFile);
      rename (szTmpFile, szCtlFile);
      }
   CtlFileAccess (pszUserID, FALSE);
   return uCount;
   }



/* 
 * Reads a ctl line into a pjob struct
 * I don't know what whis comment means:
 *    must set job values to NULL at end
 *
 */
BOOL CtlReadLine (FILE *fpCtl, PJOB pjob)
   {
   char szLine[512];
   PSZ  *ppsz;
   USHORT uCols;

   if (FilReadLine (fpCtl, szLine, ";", sizeof szLine) == 0xFFFF)
      return FALSE;

   if (!(ppsz = StrMakePPSZ (szLine, ",\n", TRUE, TRUE, &uCols)))
      return FALSE;

   if (uCols < 8)
      {
      MemFreePPSZ (ppsz, uCols);
      return FALSE;
      }

   pjob->pszJobID   = strdup (ppsz[0]);
   pjob->pszJobName = strdup (ppsz[1]);
   pjob->pszJobTag  = strdup (ppsz[2]);
   pjob->pszJobDesc = strdup (ppsz[3]);
   pjob->tmQueueTime= atoi   (ppsz[4]);
   pjob->tmStartTime= atoi   (ppsz[5]);
   pjob->tmEndTime  = atoi   (ppsz[6]);
   pjob->uRetCode   = atoi   (ppsz[7]);
   pjob->uPriority  = atoi   (ppsz[8]);

   MemFreePPSZ (ppsz, uCols);
   return TRUE;
   }




/*
 * Adds the job to the users job ctl file
 *
 */
BOOL CtlWriteLine (PJOB pjob)
   {
   FILE *fp;
   char szBuff    [256];
   char szUser    [256];
   char szCtlFile [256];

   JMLog (7, "Writing a Ctl Line Job:%s", pjob->pszJobID);
   MakeValidDirName (szUser, pjob->pszUserID);
   sprintf (szCtlFile, "%s\\%s.CTL", UserDir (szBuff, pjob->pszUserID), szUser);
   CtlFileAccess (pjob->pszUserID, TRUE);

   if (!(fp = my_fopen (szCtlFile, "a")))
      {
      JMError ("Unable to open ctl file %s Job:%s", szCtlFile, pjob->pszJobID);
      CtlFileAccess (pjob->pszUserID, FALSE);
      return FALSE;
      }
   fprintf (fp, "%s,",  StrMakeCSVField (szBuff, pjob->pszJobID  ));
   fprintf (fp, "%s,",  StrMakeCSVField (szBuff, pjob->pszJobName));
   fprintf (fp, "%s,",  StrMakeCSVField (szBuff, pjob->pszJobTag ));
   fprintf (fp, "%s,",  StrMakeCSVField (szBuff, pjob->pszJobDesc));
   fprintf (fp, "%u,",  pjob->tmQueueTime);
   fprintf (fp, "%u,",  pjob->tmStartTime);
   fprintf (fp, "%u,",  pjob->tmEndTime  );
   fprintf (fp, "%u,",  pjob->uRetCode  );
   fprintf (fp, "%u\n", pjob->uPriority );
   fclose (fp);
   CtlFileAccess (pjob->pszUserID, FALSE);
   return TRUE;
   }





/*
 * This fn generates a new job ID
 * This fn updates the last-id ctl file
 *
 */
PSZ CreateNewJobID (PCOMMAND pcmd, PSZ pszJobID)
   {
   char  szFile[256];
   char  szLastID[256];
   FILE  *fpCtl;
   ULONG ulID;

   JMLog (7, "Creating New Job ID for User:%s", pcmd->pszUserID);
   UserDir (szFile, pcmd->pszUserID);
   strcat (szFile, "\\");
   strcat (szFile, LAST_ID_FILE);

   if (!(fpCtl = my_fopen (szFile, "r+")))
      {
      if (fpCtl = my_fopen (szFile, "w+"))
         {
         JMLog (7, "Creating New LastID file for User:%s", pcmd->pszUserID);
         fprintf (fpCtl, "00000000");
         rewind (fpCtl);
         }
      }
   if (!fpCtl)
      {
      /*--- for some reason we cannot open/create the ctl file ---*/
      srand ((USHORT)time(NULL));
      ulID = (ULONG) rand ();
      sprintf (szLastID, "%8.8lu", ulID);
      JMError ("Cannot open/create LastID file for user:%s", pcmd->pszUserID);
      }
   else
      {
      FilReadLine (fpCtl, szLastID, ";", sizeof szLastID);

      if (!*szLastID)
         strcpy (szLastID, "0");

      ulID = atol (szLastID) + 1;
      sprintf (szLastID, "%8.8lu", ulID);
      rewind (fpCtl);
      fprintf (fpCtl, "%s\n", szLastID);
      fclose (fpCtl);
      }
   strcpy (pszJobID, szLastID);
   return pszJobID;
   }

