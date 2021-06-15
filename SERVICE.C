/*
 *
 * service.c
 * Thursday, 1/5/1995.
 *
 * (C) 1995 Info Tech Inc.
 * Craig Fitzgerald
 *
 * Part of the BAMS Job Server
 * This mod is where the client commands are serviced
 */

#define INCL_DOS
#define INCL_DOSERRORS
#include <os2.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <process.h>
#include <direct.h>
#include <GnuFile.h>
#include <GnuMisc.h>
#include "Var.h"
#include "BamsServ.h"
#include "CtlFile.h"
#include "Syssock.h"
#include "osock.h"
#include "JobQ.h"
#include "Util.h"
#include "Service.h"
#include "UserIO.h"
#include "Security.h"
#include "Sokutil.h"
#include "Print.h"
#include "Stat.h"

USHORT uACCEPTFLAGS = 0xFFFF;

/*
 * Converts a date in FDATE format to a D/M/Y string
 * the string is returned
 *
 */
static PSZ DateString2 (PSZ pszBuff, FDATE fdate)
   {
   sprintf (pszBuff, "%2.2d/%2.2d/%2.2d", fdate.day, fdate.month,
              (fdate.year + 1980) % 100);
   return pszBuff;
   }


/*
 * Converts a time in FTIME format to a HH:MM:SS string
 * 
 *
 */
static PSZ TimeString2 (PSZ pszBuff, FTIME ftime)
   {
   sprintf (pszBuff, "%2.2d:%2.2d:%2.2d", 
              ftime.hours, ftime.minutes, ftime.twosecs * 2);
   return pszBuff;
   }


/*
 * returns TRUE if accepting the command
 * returns status msg to client and returns 0 if not.
 *
 */
BOOL AcceptCmd (PCOMMAND pcmd, SOCKET sock)
   {
   if (!pcmd->uCommand || (pcmd->uCommand > JSC_LASTCMD))
      return TRUE; // not my problem
      
   if (IsFlag (uACCEPTFLAGS, pcmd->uCommand - 1))
      return TRUE;

   if (sock != -1)
      SendStatus (sock, JSE_REFUSED, "Command Refused");
   return FALSE;
   }


/*********************************************************************/
/*                                                                   */
/*                                                                   */
/*                                                                   */
/*********************************************************************/

/*
 * This is a non-returning fn. (for a thread)
 * This fn wakes up every 5 minutes looking for jobs that have
 * been running for too long and kills them.
 *
 */
VOID GrimReaperFn (PVOID p)
   {
   PJOB   pjQueue;
   LONG   lNow;
   USHORT uRet;
   char   szBuff [128];

   SetPriority ("PClass", "PDelta", -1, TRUE);

   while (TRUE)
      {
      DosSleep (1000 * 60 * 5);   // sleep for 5 minutes
      time ((time_t *)&lNow);

      if (pjQueue = JLAccess (TRUE))
         JMLog (5, "Grim reaper is Looking for jobs to kill at %s", TimeString (szBuff, lNow));

      /*--- look at each running job ---*/
      for (; pjQueue; pjQueue = pjQueue->next)
         {
         if (!pjQueue->tmMaxRunTime ||
             pjQueue->tmStartTime + pjQueue->tmMaxRunTime >= lNow)
            continue;

         /*--- Swing that sickle ! ---*/
         if (!(uRet = DosKillProcess (DKP_PROCESSTREE, pjQueue->iPID)))
            {
            SetFlag (&pjQueue->uFlags, JSF_TIMEDOUT, TRUE);
            JMLog (2, "Job: %s (%s) killed via timeout", pjQueue->pszJobTag, pjQueue->pszJobName);
            }
         else if (uRet != ERROR_INVALID_PROCID)
            JMLog (2, "Job: %s (%s) cannot be killed via timeout", pjQueue->pszJobTag, pjQueue->pszJobName);
         else
            JMLog (7, "Grim Reaper: job %s (%s) already dead", pjQueue->pszJobTag, pjQueue->pszJobName);
         }
      JLAccess (FALSE);
      }
   }



/* 
 * This is a non-returning fn. (for a thread)
 * This fn will purge old jobs automatically
 * p is a long which is the maximum age in hours of any job
 * Every hour this fn will do a seek and destroy
 */
VOID PurgeDaemonFn (PVOID p)
   {
   char   szBase  [256];
   char   szMatch [256];
   char   szSpec  [256];
   LONG   lOldTime;
   HDIR   hdir = 0;
   USHORT uJobs;

   SetPriority ("PClass", "PDelta", -1, TRUE);
   while (TRUE)
      {
      VarGet (pvCFG, "FilesDir", szBase);
      sprintf (szSpec, "%s\\*.*", szBase);

      lOldTime = FixTime ((LONG)p);

      JMLog (5, "Purge Daemon is Looking for jobs before %s", DateTimeString (szMatch, lOldTime));

      hdir = 0;
      uJobs = 0;
      while (FindAFile (szMatch, &hdir, szSpec, FILE_DIRECTORY, NULL))
         uJobs += CtlPurgeJobs (szMatch, lOldTime);
      if (uJobs)
         JMLog (5, "Purge Daemon ate %d jobs", uJobs);
      else
         JMLog (5, "Purge Daemon is sleeping");
      DosSleep (3600000);   // sleep for an hour
      }
   }


/*********************************************************************/
/*                                                                   */
/*                                                                   */
/*                                                                   */
/*********************************************************************/


/*
 * This fn logs a command to the screen
 * and to the log file (if enabled)
 *
 */
static void LogCommand (PCOMMAND pcmd)
   {
   char  szLine   [256];
   char  szTimStr [256];
   char  szTimeLine [256];
   TIME  tm;
   PSTAT pstat;

   time ((time_t *)&tm);

   if (!pcmd || !pcmd->pszUserID)
      {
      sprintf (szLine, "BAD COMMAND FORMAT");
      }
   else
      {
      switch (pcmd->uCommand)
         {
         case JSC_PING:
            sprintf (szLine, "%s: PING from %s", pcmd->pszUserID, pcmd->pszClientID);
            break;
         case JSC_SUBMIT:
            sprintf (szLine, "%s: SUBMIT %s - %s", pcmd->pszUserID, pcmd->pszJobID, pcmd->pszUser2);
            break;
         case JSC_LISTJOBS:
            sprintf (szLine, "%s: LIST JOBS", pcmd->pszUserID);
            break;
         case JSC_LISTFILES:
            sprintf (szLine, "%s: LIST FILES", pcmd->pszUserID);
            break;
         case JSC_PRINT:
            sprintf (szLine, "%s: PRINT %s - %s - %s", pcmd->pszUserID, pcmd->pszJobID, pcmd->pszUser1, pcmd->pszUser2);
            break;
         case JSC_COPY:
            sprintf (szLine, "%s: COPY %s - %s", pcmd->pszUserID, pcmd->pszJobID, pcmd->pszUser1);
            break;
         case JSC_DELETEJOB:
            sprintf (szLine, "%s: DELETE JOB %s", pcmd->pszUserID, pcmd->pszJobID);
            break;
         case JSC_DELETEFILE:
            sprintf (szLine, "%s: DELETE FILE %s - %s", pcmd->pszUserID, pcmd->pszJobID, pcmd->pszUser1);
            break;
         case JSC_ACCEPT:
            sprintf (szLine, "%s: ACCEPT %lx %lx", pcmd->pszUserID, pcmd->lUser1, pcmd->lUser2);
            break;
         case JSC_PURGE:
            sprintf (szLine, "%s: PURGE %lx %lx", pcmd->pszUserID, pcmd->lUser1, pcmd->lUser2);
            break;
         case JSC_SETDEFAULT:
            sprintf (szLine, "%s: SET DEFAULT %s %s", pcmd->pszUserID, pcmd->pszUser1, pcmd->pszUser2);
            break;                               
         case JSC_GETINFO:
            sprintf (szLine, "%s: GET INFO %lx", pcmd->pszUserID, pcmd->lUser1);
            break;                               
         case JSC_SHUTDOWN:
            sprintf (szLine, "%s: SHUTDOWN", pcmd->pszUserID);
            break;                               
         case JSC_INSTALLFILES:
            sprintf (szLine, "%s: INSTALLFILES", pcmd->pszUserID, pcmd->pszClientID);
            break;                               
         case JSC_TEST:
            sprintf (szLine, "%s: TEST %s", pcmd->pszUserID, pcmd->pszUser1);
            break;
         case JSC_TEST2:
            sprintf (szLine, "%s: TEST2 %s", pcmd->pszUserID, pcmd->pszUser1);
            break;
         default:
            sprintf (szLine, "%s: UNKNOWN command %d", pcmd->pszUserID, pcmd->uCommand);
         }
      }
   sprintf (szTimeLine, "%s %s", DateTimeString (szTimStr, tm), szLine);

   /*--- Send line to log window ---*/
   UpdateWindow (JM_COMMAND_WINDOW, szTimeLine);

   /*--- Log cmd ---*/
   JMLog (4, szLine);

   if (pcmd->uCommand <= JSC_LASTCMD)
      {
      pstat = StatAccess (TRUE);
      pstat->uCommands[pcmd->uCommand]++;
      StatAccess (FALSE);
      }
   }



/*********************************************************************/
/*                                                                   */
/*                                                                   */
/*                                                                   */
/*********************************************************************/




/*
 * Service Ping Command
 *
 * socket input data : none
 * socket output data: none
 * socket return code: 0=success
 */
static USHORT ServicePing (SOCKET sock, PCOMMAND pcmd)
   {
   LONG ulPerformance;

   ulPerformance = VarGetl (pvCFG, "ServerPerformance");

   return SendStatus (sock, ulPerformance, NULL);
   }


/*
 * Service Submit Command
 *
 * socket input data : params, subsetlist
 * socket output data: none
 * socket return code: 0=success
 */
static USHORT ServiceSubmit (SOCKET sock, PCOMMAND pcmd)
   {
   char  szJobID      [32];
   char  szPath       [256];
   char  szJobCtlFile [256];
   PJOB  pjob;
   USHORT uRet;

   JMLog (5, "IN JMSubmit");

   if (!AcceptCmd (pcmd, sock))
      return JSE_REFUSED;

   if (JQFull ())
      return SendStatus (sock, JSE_REFUSED, "Job Submit command Refused: Queue is Full Job:%s", pcmd->pszJobID);

   CreateNewJobID (pcmd, szJobID);
   pcmd->pszJobID = strdup (szJobID);

   /*--- Create output directory ---*/
   FilesDir (szPath, pcmd, FALSE);
   if (!PathExists (szPath) && !FilMakePath (szPath))
      return SendStatus (sock, JSE_CANNOT_CREATE_DIR, "Cannot make output dir: %s", szPath);

   /*--- Create System file directory ---*/
   FilesDir (szPath, pcmd, TRUE);
   if (!PathExists (szPath) && !FilMakePath (szPath))
      return SendStatus (sock, JSE_CANNOT_CREATE_DIR, "Cannot make system dir: %s", szPath);

   /*--- Let the client know all is cool, and get next data piece ---*/
   SendStatus (sock, 0, NULL);

   /*--- read params file ---*/
   if (MySokFileRecv (sock, PARAMS_FILE, szPath))
      return SendStatus (-1, JSE_SOCKET_SEND_ERROR, "Cannot get Param file from socket: %s\\%s", szPath, PARAMS_FILE);

   /*--- Let the client know all is cool, and get next data piece ---*/
   SendStatus (sock, 0, NULL);

   /*--- read subset list ---*/
   if (MySokFileRecv (sock, SUBSET_FILE, szPath))
      return SendStatus (-1, JSE_SOCKET_SEND_ERROR, "Cannot get subset file from socket: %s\\%s", szPath, SUBSET_FILE);

   /*--- setup job structure ---*/
   pjob = MakeJob ();
   pjob->pszJobID    = strdup (pcmd->pszJobID)   ;
   pjob->pszJobName  = strdup (pcmd->pszUser1)   ;
   pjob->pszJobTag   = strdup (pcmd->pszUser2)   ;
   pjob->pszJobDesc  = strdup (pcmd->pszUser3)   ;
   pjob->pszUserID   = strdup (pcmd->pszUserID)  ;
   pjob->pszClientID = strdup (pcmd->pszClientID);
   pjob->lMessagePort= pcmd->lMessagePort;
   pjob->tmStartTime = 0;
   pjob->tmEndTime   = 0;
   pjob->uPriority   = pcmd->uPriority;   
   time ((time_t *)&pjob->tmQueueTime);

   /*--- create job.ctl file ---*/
   sprintf (szJobCtlFile, "%s\\%s", szPath, JOB_CTL_FILE);
   if (!JobCtlCreateFile (szJobCtlFile, pjob))
      JMLog (3, "Job cannot be re-queued");

   /*--- queue the job ---*/
   if (uRet = JQPut (pjob))
      {
      JMLog (3, "Job Submit command Refused: Queue is Full  Job:%s", pjob->pszJobID);
      FreeJob (pjob);
      }
   return SendStatus (sock, uRet, "JQPut return: %d", uRet);
   }



/*
 *
 * service.c:264
 */
static void WriteJobRow (FILE *fp, PJOB pj, PSZ pszStatus)
   {
   char  szTmp[64];

   fprintf (fp, "%s\t", pj->pszJobID);
   fprintf (fp, "%s\t", pj->pszJobName);
   fprintf (fp, "%s\t", pj->pszJobTag);
   fprintf (fp, "%s\t", pj->pszJobDesc);
   fprintf (fp, "%s\t", TimeString (szTmp, pj->tmQueueTime));
   fprintf (fp, "%s\t", TimeString (szTmp, pj->tmStartTime));
   fprintf (fp, "%s\t", TimeString (szTmp, pj->tmEndTime));
   fprintf (fp, "%u\t", pj->uPriority);
   fprintf (fp, "%s\n", pszStatus);
   }



/*
 * Service List Jobs Command
 *
 * socket input data : none
 * socket output data: joblist
 * socket return code: 0=success
 *
 *
 * joblist file format:
 *    Job Tag
 *    Job Desc
 *    Job Priority
 *    Job Queue Time
 *    Job Start Time
 *    Job End Time
 *    Job Return Code
 *
 */
static USHORT ServiceListJobs (SOCKET sock, PCOMMAND pcmd)
   {
   PJOB   pjQueue;
   JOB    job;
   FILE   *fpList, *fpCtl;
   char   szBuff[256];
   char   szListFile[256];
   USHORT uStatus, uRet;

   JMLog (5, "IN ServiceListJobs");

   if (!AcceptCmd (pcmd, sock))
      return JSE_REFUSED;

   sprintf (szListFile, "%s\\%s", UserDir (szBuff, pcmd->pszUserID), JOB_LIST_FILE);
   if (!(fpList = my_fopen (szListFile, "w")))
      return SendStatus (sock, JSE_CANNOT_CREATE_FILE, "Unable to create temp list file: %s", szListFile);

   /*--- pending jobs ---*/
   pjQueue = JQAccess (TRUE);
   for (; pjQueue; pjQueue = pjQueue->next)
      if (!stricmp (pjQueue->pszUserID, pcmd->pszUserID))
         WriteJobRow (fpList, pjQueue, "Job Pending");
   JQAccess (FALSE);

   /*--- running jobs ---*/
   pjQueue = JLAccess (TRUE);
   for (; pjQueue; pjQueue = pjQueue->next)
      if (!stricmp (pjQueue->pszUserID, pcmd->pszUserID))
         WriteJobRow (fpList, pjQueue, "Job Running");
   JLAccess (FALSE);

   /*--- completed, stored jobs ---*/
   if (fpCtl = CtlFileHandle (pcmd->pszUserID, NULL, NULL))
      {
      while (CtlReadLine (fpCtl, &job))
         WriteJobRow (fpList, &job, ReturnCodeString (job.uRetCode));
      CtlFileHandle (pcmd->pszUserID, NULL, fpCtl);
      }
   fclose (fpList);

   /*--- Let the client know all is cool, and file is coming ---*/
   SendStatus (sock, 0, NULL);

   JMLog (5, "IN ServiceListJobs: About to send file...");
   if (MySokFileSend (sock, szListFile))
      return SendStatus (-1, JSE_SOCKET_SEND_ERROR, "Unable to send file over socket: %s", szListFile);
   JMLog (5, "IN ServiceListJobs: sent file/about to get status...");

   if ((uRet = RecvStatus (sock, &uStatus)) == FALSE)
      return JMError ("Got bad status return sending JobListFile to %s", pcmd->pszUserID);
   if (uStatus)
      return JMError ("Got bad status return from client %s in JobListFile", pcmd->pszUserID);
   JMLog (5, "IN ServiceListJobs: got status...");
   
   if (!bDEBUG)
      unlink (szListFile);
   return SendStatus (sock, 0, NULL);
   }



/*
 * Service List Files Command
 *
 * socket input data : none
 * socket output data: filelist
 * socket return code: 0=success
 *
 *
 * filelist file format
 *    file name
 *    file data
 *    file time
 *    file size
 *
 */
static USHORT ServiceListFiles (SOCKET sock, PCOMMAND pcmd)
   {
   FILEFINDBUF3 fbuf3;
   HDIR        hdir;
   USHORT      i, uStatus;
   FILE        *fpList;
   char        szFile[256];
   char        szListFile[256];
   char        szFileSpec[256];
   char        szBuff[256];

   JMLog (5, "IN ServiceListFiles");

   if (!AcceptCmd (pcmd, sock))
      return JSE_REFUSED;

   sprintf (szListFile, "%s\\%s", UserDir (szBuff, pcmd->pszUserID), FILE_LIST_FILE);
   if (!(fpList = my_fopen (szListFile, "w")))
      return SendStatus (sock, JSE_CANNOT_CREATE_FILE, "Unable to create temp list file: %s", szListFile);

   for (i=0; i<2; i++)
      {
      hdir = 0;
      sprintf (szFileSpec, "%s\\*.*", FilesDir (szBuff, pcmd, !!i));
      while (FindAFile (szFile, &hdir, szFileSpec, FILE_NORMAL, &fbuf3))
         {
         fprintf (fpList, "%s\t", fbuf3.achName);
         fprintf (fpList, "%s\t", DateString2 (szBuff, fbuf3.fdateLastWrite));
         fprintf (fpList, "%s\t", TimeString2 (szBuff, fbuf3.ftimeLastWrite));
         fprintf (fpList, "%lu\t", fbuf3.cbFile /*size*/);
         fprintf (fpList, "%d\n", i /*SysFilesFlag*/);
         }
      }
   fclose (fpList);

   /*--- Let the client know all is cool, and that file is coming ---*/
   SendStatus (sock, 0, NULL);

   if (MySokFileSend (sock, szListFile))
      return SendStatus (-1, JSE_SOCKET_SEND_ERROR, "Unable to send file over socket: %s", szListFile);

   if ((RecvStatus (sock, &uStatus)) == FALSE)
      return JMError ("Got bad status return sending FileListFile to %s", pcmd->pszUserID);
   if (uStatus)
      return JMError ("Got bad status return from client %s in fn FileListFile", pcmd->pszUserID);

   if (!bDEBUG)
      unlink (szListFile);
   return SendStatus (sock, 0, NULL);
   }



// FILE fpFile, fpPrn;
//   fpFile= my_fopen (szFile, "rb");
//   fpPrn = my_fopen (IniVar ("Default_Printer"), "wb");
//   while (uLen = fread (szBuff, 1, COPYBUFFSIZE, fpFile))
//      fwrite (szBuff, 1, uLen, fpPrn);
//   fclose (fpFile);
//   fclose (fpPrn);
//   DosCopy (szFile, szPrinter, DCPY_EXISTING, 0);
/*
 * Service Print Command
 *
 * socket input data : none
 * socket output data: none
 * socket return code: 0=success
 *
 *
 */
static USHORT ServicePrint (SOCKET sock, PCOMMAND pcmd)
   {
   char   szFile[256];
   char   szPrinter[256];
   char   szServer[256];
   char   szBuff[256];
   USHORT uRet;
   PSZ    psz;

   JMLog (5, "IN ServicePrint");

   if (!AcceptCmd (pcmd, sock))
      return JSE_REFUSED;

   /*--- Build the full filespec ---*/
   if (pcmd->lUser1 > 1)             // file has path info already
      strcpy (szFile, pcmd->pszUser1);
   else                              // file in output or sys dir
      sprintf (szFile , "%s\\%s", FilesDir (szBuff, pcmd, pcmd->lUser1), pcmd->pszUser1);

   /*--- Is the printer name (and possibly the server name) given? ---*/
   if (pcmd->pszUser2 && *pcmd->pszUser2)
      {
      strcpy (szPrinter, pcmd->pszUser2);
      if (psz = strchr (szPrinter, ',')) // server could be appended on
         {
         *psz++ = '\0';
         strcpy (szServer, psz);
         }
      }

   /*--- Look for defaults if printer and/or server are not defined ---*/
   if (!szPrinter)
      VarGet (pvCFG, "DefaultPrinter", szPrinter);
   if (!szServer)
      VarGet (pvCFG, "DefaultPrintServer", szServer);

   /*
    * This code is assuming that the printer name coming across is valid
    * This will need to be checked against a list of valid printers in the
    * cfg file.
    */
   uRet = PrintFile (szFile,         // Filename
                     szPrinter,      // Printer Name
                     szServer,       // Print Server Name
                     pcmd->pszUser3, // Forms (?)
                     pcmd->lUser2,   // Copies
                     FALSE);         // don't delete it!

   return SendStatus (sock, uRet, "Print return code: %d", uRet);
   }


/*
 * Service Copy Command
 *
 * socket input data : none
 * socket output data: none
 * socket return code: 0=success
 *
 */
static USHORT ServiceCopy (SOCKET sock, PCOMMAND pcmd)
   {
   char   szFile[256];
   char   szBuff[256];
   USHORT uStatus, uRet;

   JMLog (5, "IN ServiceCopy");

   if (!AcceptCmd (pcmd, sock))
      return JSE_REFUSED;

   if (pcmd->lUser1 > 1)
      strcpy (szFile, pcmd->pszUser1);
   else
      sprintf (szFile , "%s\\%s", FilesDir (szBuff, pcmd, (USHORT)pcmd->lUser1), 
               pcmd->pszUser1);

   /*--- Let the client know all is cool, and that file is coming ---*/
   SendStatus (sock, 0, NULL);

   if (MySokFileSend (sock, szFile))
      return SendStatus (-1, JSE_SOCKET_SEND_ERROR, "Unable to send file over socket: %s", szFile);

   if ((uRet = RecvStatus (sock, &uStatus)) == FALSE)
      return JMError ("Got bad status return sending copy file to %s", pcmd->pszUserID);
   if (uStatus)
      return JMError ("Got bad status return from %s in copyfile", pcmd->pszUserID);

   return SendStatus (sock, 0, NULL);
   }


/*
 * returns TRUE if successful
 *
 */
BOOL KillRunningJob (PJOB pjob)
   {
   USHORT uRet;

   if (!(uRet = DosKillProcess (DKP_PROCESSTREE, pjob->iPID)))
      SetFlag (&pjob->uFlags, JSF_KILLED, TRUE);

   return !uRet;
   }


/*
 * kills all running jobs
 *
 */
void KillRunningJobs (BOOL bAllowRestart)
   {
   PJOB pjQueue;

   for (pjQueue = JLAccess (TRUE); pjQueue; pjQueue = pjQueue->next)
      {
      if (KillRunningJob (pjQueue))
         {
         JMLog (3, "Job %s Killed", pjQueue->pszJobID);
         SetFlag (&pjQueue->uFlags, JSF_RESTARTABLE, bAllowRestart);
         }
      else
         JMLog (3, "Could not kill job %s", pjQueue->pszJobID);
      }
   JLAccess (FALSE);
   }



/*
 * Service Delete Command
 *
 * socket input data : none
 * socket output data: none
 * socket return code: 0=success
 *
 */
static USHORT ServiceDeleteJob (SOCKET sock, PCOMMAND pcmd)
   {
   PJOB pjQueue;
   BOOL bFound;
   char szBuff    [256];
   USHORT uRet;

   JMLog (5, "IN ServiceDeleteJob");

   if (!AcceptCmd (pcmd, sock))
      return JSE_REFUSED;

   /*--- look at pending jobs ---*/
   pjQueue = JQAccess (TRUE);
   for (; pjQueue; pjQueue = pjQueue->next)
      if (!stricmp (pjQueue->pszUserID, pcmd->pszUserID) &&
          !stricmp (pjQueue->pszJobID,  pcmd->pszJobID))
         {
         JQDelete (pjQueue);
         JQAccess (FALSE);
         return SendStatus (sock, 0, NULL);
         }
   JQAccess (FALSE);

   /*--- look at running jobs ---*/
   pjQueue = JLAccess (TRUE);
   for (; pjQueue; pjQueue = pjQueue->next)
      if (!stricmp (pjQueue->pszUserID, pcmd->pszUserID) &&
          !stricmp (pjQueue->pszJobID,  pcmd->pszJobID))
         {
         if (uRet = KillRunningJob (pjQueue))
            JMLog (3, "Job %s Killed", pjQueue->pszJobID);
         else
            JMLog (3, "Could not kill job %s", pjQueue->pszJobID);
         JLAccess (FALSE);
         return SendStatus (sock, (uRet ? 0 : JSE_CANNOT_DELETE), NULL);
         }
   JLAccess (FALSE);

   /*--- assume its a completed job ---*/
   if (bFound = CtlRemoveLine (pcmd->pszUserID, pcmd->pszJobID))
      RemoveDir (FilesDir0 (szBuff, pcmd->pszUserID, pcmd->pszJobID, 2), TRUE);

   uRet = (bFound ? 0 : JSE_CANNOT_FIND_FILE);
   return SendStatus (sock, uRet, "Unable to Find/Delete job");
   }



/*
 * Service Delete file Command
 *
 * socket input data : none
 * socket output data: none
 * socket return code: 0=success
 *
 */
static USHORT ServiceDeleteFile (SOCKET sock, PCOMMAND pcmd)
   {
   char szFile[256];
   char szBuff[256];
   USHORT uRet;

   JMLog (5, "IN ServiceDeleteFile");

   if (!AcceptCmd (pcmd, sock))
      return JSE_REFUSED;

   if (pcmd->lUser1 > 1)
      strcpy (szFile, pcmd->pszUser1);
   else
      sprintf (szFile , "%s\\%s", FilesDir (szBuff, pcmd, (USHORT)pcmd->lUser1), 
               pcmd->pszUser1);

   uRet = (unlink (szFile) ? JSE_CANNOT_FIND_FILE : 0);
   return SendStatus (sock, uRet, "Unable to Find/Delete file %s", szFile);
   }



/*
 * Allows / Disallows specific server commands
 * pcmd->lUser1 is used as an array of flags where 1 is on and 0 os off
 * flag order is same as command order
 */
static USHORT ServiceAccept (SOCKET sock, PCOMMAND pcmd)
   {
   JMLog (5, "IN ServiceAccept");

   if (!AcceptCmd (pcmd, sock))
      return JSE_REFUSED;

   SetFlag (&uACCEPTFLAGS, (USHORT)(ULONG)pcmd->lUser1, (USHORT)(ULONG)pcmd->lUser2);

   JMLog (6, "Setting Accept Cmd #%d to %d", (USHORT)(ULONG)pcmd->lUser1, (USHORT)(ULONG)pcmd->lUser2);
   return SendStatus (sock, 0, "OK");
   }



/*
 * Job purge command
 *
 * lUser1 contains the purge mode. Values are:
 *  0 - remove all users completed jobs before specific datetime
 *  1 - remove all completed jobs before specific datetime
 *
 *
 * lUser2 contains the datetime
 *       Note: this value is the DateTime returned from time()
 *             which is seconds since 1970
 *             If the value is 1 thru 1000 however, it is considered a
 *             relative number in hours.
 */
static USHORT ServicePurge (SOCKET sock, PCOMMAND pcmd)
   {
   USHORT uJobs = 0;
   char   szBuff  [64];
   char   szBase  [256];
   char   szSpec  [256];
   char   szMatch [256];
   HDIR   hdir = 0;

   JMLog (5, "IN ServicePurge");

   if (!AcceptCmd (pcmd, sock))
      return JSE_REFUSED;

   pcmd->lUser2 = FixTime (pcmd->lUser2);

   if (!pcmd->lUser1)                           // this user
      {
      uJobs = CtlPurgeJobs (pcmd->pszUserID, pcmd->lUser2);
      JMLog (3, "Purge: user %s jobs before %s were purged. count=%d", pcmd->pszUserID, DateTimeString (szBuff, pcmd->lUser2), uJobs);
      }
   else                                         // all users
      {
      VarGet (pvCFG, "FilesDir", szBase);
      sprintf (szSpec, "%s\\*.*", szBase);

      while (FindAFile (szMatch, &hdir, szSpec, FILE_DIRECTORY, NULL))
         uJobs += CtlPurgeJobs (szMatch, pcmd->lUser2);

      JMLog (3, "Purge: All jobs before %s were purged. count=%d", DateTimeString (szBuff, pcmd->lUser2), uJobs);
      }
   return SendStatus (sock, 0, "OK");
   }




/*
 *
 *
 *
 */
static USHORT ServiceSetDefault (SOCKET sock, PCOMMAND pcmd)
   {
   JMLog (5, "IN ServiceSetDefault");

   if (!AcceptCmd (pcmd, sock))
      return JSE_REFUSED;

   VarSet (&pvCFG, pcmd->pszUser1, pcmd->pszUser2);

   /*--- some may have changed ---*/
   SetBamsServGlobals ();
   SetUserIOGlobals ();
   SetJobQGlobals ();

   return SendStatus (sock, 0, "OK");
   }



static void ShowFreeSpace (FILE *fpStat)
   {
   FSALLOCATE fs;
   ULONG      i;
   ULONG      ulDrives;

   DosError (HARDERROR_DISABLE);
   DosQCurDisk (&i, &ulDrives);
   DosError (HARDERROR_ENABLE);
   for (i=2; i<26; i++)
      {
      if ((ulDrives >> i) & 1)
         {
         DosError (HARDERROR_DISABLE);
         DosQFSInfo (i+1, FSIL_ALLOC, &fs, sizeof (fs));
         DosError (HARDERROR_ENABLE);
         fprintf (fpStat, "       Space on Drive %c: %lu\n", 'A' + i, fs.cbSector * fs.cSectorUnit * fs.cUnitAvail);
         }
      }
   }


/*
 * lUser1 contains flags:
 *   0 - Statistics
 *   1 - Variables
 *   2 - ScreenDump
 *
 */
static USHORT ServiceGetInfo (SOCKET sock, PCOMMAND pcmd)
   {
   char  szBuff    [256];
   char  szStatFile[256];
   FILE  *fpStat;
   USHORT uFlags, uStatus;

   JMLog (5, "IN Service");

   if (!AcceptCmd (pcmd, sock))
      return JSE_REFUSED;

   sprintf (szStatFile, "%s\\%s", UserDir (szBuff, pcmd->pszUserID), STAT_FILE);
   if (!(fpStat = my_fopen (szStatFile, "w")))
      return SendStatus (sock, JSE_CANNOT_CREATE_FILE, "Unable to create temp stat file: %s", szStatFile);

   uFlags = (USHORT)pcmd->lUser1;
   if (IsFlag (uFlags, 0))  // Statistics
      {
      fputc ('\n', fpStat);
      Identify (fpStat, "STATISTICS");
      ShowFreeSpace (fpStat);
      StatDump (fpStat);
      }
   if (IsFlag (uFlags, 1))  // Variables
      {
      fputc ('\n', fpStat);
      Identify (fpStat, "VARIABLES");
      VarDump (fpStat, pvCFG);
      }
   if (IsFlag (uFlags, 2))  // Screen Dump
      {
      fputc ('\n', fpStat);
      ScreenDump (fpStat);
      }
   fclose (fpStat);
      
   /*--- Let the client know all is cool, and that file is coming ---*/
   SendStatus (sock, 0, NULL);

   if (MySokFileSend (sock, szStatFile))
      return SendStatus (-1, JSE_SOCKET_SEND_ERROR, "Unable to send status file over socket: %s", szStatFile);

   if ((RecvStatus (sock, &uStatus)) == FALSE)
      return JMError ("Got bad status return sending StatFile to %s", pcmd->pszUserID);
   if (uStatus)
      return JMError ("Got bad status return from client %s in fn StatFile", pcmd->pszUserID);

   if (!bDEBUG)
      unlink (szStatFile);
   return SendStatus (sock, 0, NULL);
   }



/*
 *
 *
 *
 */
static USHORT ServiceShutdown (SOCKET sock, PCOMMAND pcmd)
   {
   JMLog (5, "IN ServiceShutdown");

   if (!AcceptCmd (pcmd, sock))
      return JSE_REFUSED;

   if (pcmd->lUser1)
      KillRunningJobs (pcmd->lUser1 == 1);

   SendStatus (sock, 0, "Service Shutdown");

   TriggerShutdown ();
   return 0;
   }



/*
 * Service InstallFiles Command
 *
 */
static USHORT ServiceInstallFiles (SOCKET sock, PCOMMAND pcmd)
   {
   char   szListFile [256];
   char   szPath     [256];
   USHORT uRet, uStatus;

   JMLog (5, "IN ServiceInstallfiles");

   if (!AcceptCmd (pcmd, sock))
      return JSE_REFUSED;

   if (!VarGet (pvCFG, "ClientInstallFiles", szPath))
      strcpy (szPath, "."); // this shouldn't happen

   sprintf (szListFile, "%s\\%s", szPath, INSTALLLIST_FILE);

   JMLog (5, "IN ServiceInstallFiles: About to send file...");
   if (MySokFileSend (sock, szListFile))
      return SendStatus (-1, JSE_SOCKET_SEND_ERROR, "Unable to send file over socket: %s", szListFile);
   JMLog (5, "IN ServiceInstallFiles: sent file/about to get status...");

   if ((uRet = RecvStatus (sock, &uStatus)) == FALSE)
      return JMError ("Got bad status return sending InstallFiles to %s", pcmd->pszUserID);
   if (uStatus)
      return JMError ("Got bad status return from client %s in JobInstallFiles", pcmd->pszUserID);
   JMLog (5, "IN ServiceInstallFiles: got status...");

   return SendStatus (sock, 0, "Service install files");
   }




/*
 * Service Test Command
 *
 * socket input data : none
 * socket output data: none
 * socket return code: 0=success
 *
 * This dumps mem info to stdout
 *
 */
static USHORT ServiceTest (SOCKET sock, PCOMMAND pcmd)
   {
   JMLog (5, "IN ServiceTest");

   if (!AcceptCmd (pcmd, sock))
      return JSE_REFUSED;

#if defined (__IBMC__)
   _dump_allocated (16);
#endif

   return SendStatus (sock, 0, "Service test");
   }



/*
 * Service Test2 Command
 *
 * socket input data : none
 * socket output data: none
 * socket return code: 0=success
 *
 *
 * This send a status message dgram to the client
 *
 */
static USHORT ServiceTest2 (SOCKET sock, PCOMMAND pcmd)
   {
   PJOB  pjob;

   JMLog (5, "IN ServiceTest");

   if (!AcceptCmd (pcmd, sock))
      return JSE_REFUSED;

   pjob = MakeJob ();
   pjob->pszJobTag   = strdup ("*DEBUG_TEST*")   ;
   pjob->uRetCode    = 0;
   pjob->lMessagePort= pcmd->lMessagePort;
   pjob->pszClientID = strdup (pcmd->pszClientID);
   pjob->pszUserID   = strdup (pcmd->pszUserID)  ;

   SendStatus (sock, 0, "Service test");

   NotifyClient (pjob);
   FreeJob (pjob);

   return 0;
   }



/*************************************************************************/
/*                                                                       */
/*                                                                       */
/*                                                                       */
/*************************************************************************/

/*************************************************************************/
/*                                                                       */
/*                                                                       */
/*                                                                       */
/*************************************************************************/

static void ShowCmd (PCOMMAND pcmd, USHORT uLogLevel)
   {
   char szBuff [32];

   JMLog (uLogLevel, "");
   JMLog (uLogLevel, "------------ COMMAND STRUCTURE  [%s] ------------", DateTimeString (szBuff, time(NULL)));
   JMLog (uLogLevel, "uCommand    = %-6u    pszUserID  = %s",   pcmd->uCommand ,    pcmd->pszUserID  );
   JMLog (uLogLevel, "uPriority   = %-6u    pszClientID= %s",   pcmd->uPriority,    pcmd->pszClientID);
   JMLog (uLogLevel, "lMessagePort= %-9lu pszServerID= %s",     pcmd->lMessagePort, pcmd->pszServerID);
   JMLog (uLogLevel, "                        pszJobID   = %s",                     pcmd->pszJobID   );
   JMLog (uLogLevel, "lUser1      = %-9lu pszUser1   = %s",     pcmd->lUser1,       pcmd->pszUser1   );
   JMLog (uLogLevel, "lUser2      = %-9lu pszUser2   = %s",     pcmd->lUser2,       pcmd->pszUser2   );
   JMLog (uLogLevel, "lUser3      = %-9lu pszUser3   = %s",     pcmd->lUser3,       pcmd->pszUser3   );
   JMLog (uLogLevel, "---------------------------------------------------------------");
   }


/*
 * This fn accepts commands from the client and dispatches them
 *
 *
 */
VOID AcceptFn (PVOID p)
   {
   PCOMMAND pcmd;
   ULONG    sock;
   char     szName[32];
   PSTAT    pstat;

   JMLog (10, "Thread m going ...");

   SetPriority ("PClass", "PDelta", 0, TRUE);

   sock = (ULONG) p;

   if (!SecurityCheck (sock))
      {
      pstat = StatAccess (TRUE);
      pstat->uSecurityFailures++;
      StatAccess (FALSE);
      return;
      }

   /*--- we do not want this on the stack seg! ---*/
   pcmd = calloc (1, sizeof (COMMAND));

   if (SokCmdRecv (sock, pcmd))
      {
      free (pcmd);
      SendStatus (sock, JSE_SOCKET_RECV_ERROR, "error receiving a command");
      return;
      }

   /*--- This may be a more reliable way to get the client ID ---*/
   if (!pcmd->pszClientID || !*pcmd->pszClientID)
      {
      if (*SokGetPeerIP (sock, szName))
         {
         if (pcmd->pszClientID)
            free (pcmd->pszClientID);
         pcmd->pszClientID = strdup (szName);
         }
      }
   ShowCmd (pcmd, 6);

   switch (pcmd->uCommand)
      {
      case JSC_PING:                    // ping server
         ServicePing (sock, pcmd);
         break;
      case JSC_SUBMIT:                  // submit a job
         ServiceSubmit (sock, pcmd);   
         break;
      case JSC_LISTJOBS:                // list jobs for a given user
         ServiceListJobs (sock, pcmd); 
         break;
      case JSC_LISTFILES:               // list files for a given user/job
         ServiceListFiles (sock, pcmd);
         break;
      case JSC_PRINT:                   // print a file for a given user/job/file
         ServicePrint (sock, pcmd);
         break;
      case JSC_COPY:                    // copy a file to the user
         ServiceCopy (sock, pcmd);
         break;
      case JSC_DELETEJOB:               // delete a job for a given user
         ServiceDeleteJob (sock, pcmd);
         break;
      case JSC_DELETEFILE:              // delete a file for a given user/job/file
         ServiceDeleteFile (sock, pcmd);
         break;
      case JSC_ACCEPT:                  // accept/deny specific commands
         ServiceAccept (sock, pcmd);
         break;
      case JSC_PURGE:                   // purge user or all files before a given date
         ServicePurge (sock, pcmd);
         break;
      case JSC_SETDEFAULT:              // set/change default values
         ServiceSetDefault (sock, pcmd);
         break;                        
      case JSC_GETINFO:                 // get running statistics
         ServiceGetInfo (sock, pcmd);
         break;
      case JSC_SHUTDOWN:                // shutdown
         ServiceShutdown (sock, pcmd);
         break;
      case JSC_INSTALLFILES:            // bootstrap
         ServiceInstallFiles (sock, pcmd);
         break;
      case JSC_TEST:                    // test mode
         ServiceTest (sock, pcmd);
         break;
      case JSC_TEST2:                   // test mode
         ServiceTest2 (sock, pcmd);
         break;
      default:
         SendStatus (sock, JSE_INVALID_CMD, "Unknown command revieved: %d", pcmd->uCommand);
      }
   LogCommand (pcmd);
   if (SokDisconnect (sock) == -1) 
      JMLog (2, "Command socket having trouble with disconnect...");
   FreeCommand (pcmd);
   _endthread ();
   }



void AddFiles (FILE *fp, PSZ pszBase, PSZ pszMatch)
   {
   FILEFINDBUF3 fbuf3;
   HDIR         hdir = 0;
   char         szBuff [128];
   char         szFile [256];
   char         szSpec [256];

   sprintf (szSpec, "%s\\%s\\*.*", pszBase, pszMatch);
   while (FindAFile (szFile, &hdir, szSpec, FILE_NORMAL, &fbuf3))
      {
      fprintf (fp, "%-8s, ", pszMatch);
      fprintf (fp, "%-12s, ",szFile);
      fprintf (fp, "%5lu, ", fbuf3.cbFile /*size*/);
      fprintf (fp, "%s, ",   DateString2 (szBuff, fbuf3.fdateLastWrite));
      fprintf (fp, "%s\n",   TimeString2 (szBuff, fbuf3.ftimeLastWrite));
      }
   }



BOOL CreateInstallFilesFile (void)
   {
   FILE *fp;
   HDIR hdir = 0;
   char szPath     [256];
   char szListFile [256];
   char szMatch    [256];
   char szSpec     [256];

   if (!VarGet (pvCFG, "ClientInstallFiles", szPath))
      strcpy (szPath, "."); // this shouldn't happen

   sprintf (szListFile, "%s\\%s", szPath, INSTALLLIST_FILE);

   if (!(fp = my_fopen (szListFile, "w")))
      return JMError ("Unable to create InstallFiles file: %s", szListFile);

   sprintf (szSpec, "%s\\*.*", szPath);
   while (FindAFile (szMatch, &hdir, szSpec, FILE_DIRECTORY, NULL))
      AddFiles (fp, szPath, szMatch);
   fclose (fp);
   return TRUE;
   }


BOOL InitServices (void)
   {
   PSTAT pstat;

   /*--- Initialization for GetInfo command ---*/
   InitStat ();
   pstat = StatAccess (TRUE);
   time ((time_t *)&pstat->tmServerStart);
   pstat = StatAccess (FALSE);

   /*--- Initialization for InstallFiles command ---*/
   CreateInstallFilesFile ();
   return TRUE;
   }
