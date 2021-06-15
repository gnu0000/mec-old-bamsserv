/*
 *
 * runjob.c
 * Thursday, 1/12/1995.
 *
 * (C) 1995 Info Tech Inc.
 * Craig Fitzgerald
 *
 * Part of the BAMS Job Server
 * This module contains the fn to pre-process, process, and post-process
 * a job.
 *
 */


#define INCL_DOS
#define INCL_DOSERRORS
#include <os2.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <process.h>
#include <string.h>
#include <io.h>
#include <errno.h>
#include <ctype.h>
#include <direct.h>
#include <GnuStr.h>
#include <GnuFile.h>
#include <GnuMem.h>
#include <GnuMisc.h>
#include "Var.h"
#include "BamsServ.h"
#include "CtlFile.h"
#include "Util.h"
#include "RunJob.h"
#include "Syssock.h"
#include "osock.h"
#include "SokUtil.h"
#include "Banner.h"
#include "Print.h"
#include "Stat.h"


/*
 *
 *  JobParm.Dat fields that i (may) use:
 *  -------------------------------------
 *  TypRun         'D' means run in debug mode
 *  PrintOut       'Y' to print output files
 *  OutKeep        'N' to delete job files
 *  Dest           Printer name to print (may have ",server")
 *  Forms          Print forms
 *  Copies         Copies to print
 *  ExecProg       Job exe name to run
 *  ExecCfg        Job cfg name to use
 *  ExecCmd        Cmd line to exec a job
 *  PrintServer    Printer server name to print to
 *  NotifyClient   Sends status message to client when job completes
 *  Message0 (thru Message9) Message to log (for debug)
 *
 *
 *  Config options that i (may) use:
 *  -------------------------------------
 *  DefaultPrinter    
 *  DefaultPrintForms 
 *  DefaultJob         -- prog to exec (if no ExecProg)
 *  ExecCmd            -- cmd line to exec (if not from JobParm.dat)
 *  DefaultPrintServer
 *  NotifyClient      
 *  JobDir             -- if job to run doesn't have path info
 *
 * --- Full param list in parms.doc ---
 */

/*
 * This fn does the work to prepare for a job run
 * This fn assumes that the parm file is always created
 *
 * > Adds vars to parm file
 * > loads params into var structure
 */
USHORT PreProcessJob (PJOB pjob)
   {
   int  i;
   char szParmFile [256];
   char szPath     [256];
   char szBuff     [256];
   char szBuff2    [256];
   FILE *fpParm;  

   FilesDir2 (szPath, pjob, TRUE);
   sprintf (szParmFile, "%s\\%s", szPath, PARAMS_FILE);
   JMLog (4, "PreProcessing JobID:%s Name:%s at:%s for:%s", pjob->pszJobID, pjob->pszJobName, szPath, pjob->pszUserID);

   pjob->pv = NULL;
   if (!(VarReadFile (&pjob->pv, szParmFile)))
      {
      JMError ("Unable to open param file : %s", szParmFile);
      return 4;
      }
   if (!(fpParm = my_fopen (szParmFile, "a")))
      {
      JMError ("Unable to open param file! : %s", szParmFile);
      JMError ("Something's fishy in PreProcessJob! : %s", szParmFile);
      return 4;
      }

   /*--- Any user messages to display? ---*/
   for (i=0; i<9; i++)
      {
      sprintf (szBuff, "Message%d", i);
      if (VarGet (pjob->pv, szBuff, szBuff2))
         JMLog (3, "%s: %s", pjob->pszUserID, szBuff2);
      }

   /*--- add new params to param file ---*/
   fprintf (fpParm, "\n");
   fprintf (fpParm, "Home=%s\n",     FilesDir2 (szPath, pjob, 2));
   fprintf (fpParm, "SysFiles=%s\n", FilesDir2 (szPath, pjob, 1));
   fprintf (fpParm, "OutFiles=%s\n", FilesDir2 (szPath, pjob, 0));
   fprintf (fpParm, "UserID=%s\n",   pjob->pszUserID); 
   fprintf (fpParm, "JobID=%s\n",    pjob->pszJobID);
// fprintf (fpParm, "JobName=%s\n",  pjob->pszJobName); // already done
// fprintf (fpParm, "JobTag=%s\n",   pjob->pszJobTag);  // already done
   fprintf (fpParm, "JobDesc=%s\n",  pjob->pszJobDesc);
   fprintf (fpParm, "Date=%s\n",     DateString (szBuff, pjob->tmStartTime));
   fprintf (fpParm, "Time=%s\n",     TimeString (szBuff, pjob->tmStartTime));
   fprintf (fpParm, "Process=%s\n",  pjob->pszJobName);
   fclose (fpParm);

   return 0;
   }


///*
// * This fn execs the job run
// *
// */
//USHORT ProcessJob (PJOB pjob)
//   {
//   char   szBuff    [256];
//   char   szParm    [256];
//   char   szCfg     [256];
//   char   szStdOut  [256];
//   char   szStdErr  [256];
//   char   szSysPath [256];
//   char   szExecProg[256];
//   char   szExecSpec[256];
//   char   szExecCfg [256];
//   int    stdouttmp, stderrtmp;
//   USHORT uCStat,    uStat;
//
//
//   JMLog (3, "Processing JobID:%s Name:%s", pjob->pszJobID, pjob->pszJobName);
//
//   FilesDir2 (szSysPath, pjob, 1);
//
//   /*--- Get exec prog ---*/
//   if (!VarGet (pjob->pv, "ExecProg", szExecProg))
//      VarGet (pvCFG, "DefaultJob", szExecProg);
//
//   /*--- combine exec prog with its path ---*/
//   if (VarGet (pvCFG, "JobDir", szBuff))
//      sprintf (szExecSpec, "%s\\%s", szBuff, szExecProg);
//   else
//      strcpy (szExecSpec, szExecProg);
//
//   /*--- make name of exec cfg file ---*/
//   NewExtension (szExecCfg, szExecSpec, ".CFG");
//
//   /*--- Build param strings ---*/
//   sprintf (szParm, "CLNTENV=%s\\%s", szSysPath, PARAMS_FILE);
//   sprintf (szCfg,  "SYSENV=%s", szExecCfg);
//
//   BaseName (szBuff, szExecSpec, FALSE);
//   sprintf (szStdOut, "%s\\%s%s", szSysPath, szBuff, ".OUT");
//   sprintf (szStdErr, "%s\\%s%s", szSysPath, szBuff, ".ERR");
//
//   flushall ();
//
//
//   /*--- Redirect stdout and stderr ---*/
//   if ((stdouttmp = dup (fileno (stdout))) != -1)
//      if (!(freopen (szStdOut, "wt", stdout)))
//         {
//         dup2 (stdouttmp, fileno (stdout));     // cannot redirect
//         JMError ("Unable to redirect stdout to %s for job %s", szStdOut, pjob->pszJobName);
//         }                                                    
//
//   if ((stderrtmp = dup (fileno (stderr))) != -1)
//      if (!(freopen (szStdErr, "wt", stderr)))
//         {
//         dup2 (stderrtmp, fileno (stderr));     // cannot redirect
//         JMError ("Unable to redirect stderr to %s for job %s", szStdErr, pjob->pszJobName);
//         }
//
//   /*--- exec the job already goddamnit! ---*/
//   JMLog (5, "About to exec prog: %s %s %s", szExecSpec, szParm, szCfg);
//   pjob->uPID = spawnlp (P_NOWAIT, szExecSpec, szExecSpec, 
//                         szParm, szCfg, NULL);
//
//   if (pjob->uPID == (USHORT) -1)
//      {
//      JMError ("Unable to exec prog: %s %s %s", szExecSpec, szParm, szCfg);
//      dup2 (stdouttmp, fileno (stdout));        // undo redirection
//      dup2 (stderrtmp, fileno (stderr));        // undo redirection
//      flushall ();
//      return 10;
//      }
//
//   /*--- wait for job to finish, get return code ---*/
//   uCStat = cwait (&uStat, pjob->uPID, WAIT_CHILD);
//   time (&pjob->tmEndTime);
//
//   dup2 (stdouttmp, fileno (stdout));           // undo redirection
//   dup2 (stderrtmp, fileno (stderr));           // undo redirection
//   flushall ();
//
//
//   if (uCStat != (USHORT) -1)  // Somewhat normal return
//      {
//      pjob->uRetCode =  (uStat >> 8);
//      }
//   else  // abnormal termination or invalid cwait call
//      {
////      if (errno == ECHILD || errno == EINVAL) // invalid cwait call
////         {
////         JMError ("Unable to exec prog: %s %s %s", szExecSpec, szParm, szCfg);
////         return 10;
////         }
//      pjob->uRetCode =  (uStat << 8); // prog crash
//      }
//   return 0;
//   }






/*
 * This fn execs the job run
 *
 * building the spawn command requires several steps:
 * 1> Get the job program name. first look for it in the param file
 *    under the name 'ExecProg'.  Failing that, use the default name 
 *    from the cfg file named 'DefaultJob'
 * 2> Get the command line from 'ExecCmd' from the cfg file.  This string
 *    may/will have variable references that need to be replaced
 * 3> turn the command string into a ppsz for the spawnvp fn
 *
 *
 *
 */
USHORT ProcessJob (PJOB pjob)
   {
   char szBuff         [512];
   char szJobName      [256];
   char szJobPath      [256];
   char szJobSpec      [256];
   char szCfgName      [256];
   char szCfgSpec      [256];
   char szExecTemplate [256];
   char szExecCmd      [512];
   char szSysPath      [256];
   char szOutPath      [256];
   char szBaseName     [256]; // hold onto your stack!
   int  iCStat, i, iStat;
   PSZ  *ppszCmd, psz;
   PVAR pvVars = NULL;

   JMLog (4, "Processing JobID:%s Name:%s", pjob->pszJobID, pjob->pszJobName);

   /*--- Get job prog name ---*/
   if (!VarGet (pjob->pv, "ExecProg", szJobName))
      VarGet (pvCFG, "DefaultJob", szJobName);

   /*--- Get the job path ---*/
   VarGet (pvCFG, "JobDir", szJobPath);
   StrClip (szJobPath, "\\");

   /*--- Build full path spec for job prog ---*/
   if (!strchr (szJobName, '\\') &&   // combine path with job prog name
       !strchr (szJobName, ':')  &&   // if job doesn't already have path
       *szJobPath)                    // and path spec exists
      sprintf (szJobSpec, "%s\\%s", szJobPath, szJobName);
   else
      strcpy (szJobSpec, szJobName);

   /*--- Get job cfg file ---*/
   if (!VarGet (pjob->pv, "ExecCfg", szCfgName))
      NewExtension (szCfgName, szJobName, ".CFG"); // use job name if no spec

   /*--- Build full path spec for cfg file ---*/
   if (!strchr (szCfgName, '\\') &&   // combine path with job cfg name
       !strchr (szCfgName, ':')  &&   // if job doesn't already have path
       *szJobPath)                    // and path spec exists
      sprintf (szCfgSpec, "%s\\%s", szJobPath, szCfgName);
   else
      strcpy (szCfgSpec, szCfgName);

   FilesDir2 (szSysPath, pjob, 1);
   FilesDir2 (szOutPath, pjob, 0);
   BaseName (szBaseName, szJobName, FALSE);

   VarSet (&pvVars, "Job",      szJobSpec); 
   VarSet (&pvVars, "JobDir",   szJobPath);
   VarSet (&pvVars, "CfgFile",  szCfgSpec);
   VarSet (&pvVars, "ParmFile", cat (szBuff, szSysPath, "\\", PARAMS_FILE, NULL));
   VarSet (&pvVars, "StdOut",   cat (szBuff, szSysPath, "\\", szBaseName, ".out", NULL));
   VarSet (&pvVars, "StdErr",   cat (szBuff, szSysPath, "\\", szBaseName, ".err", NULL));
   VarSet (&pvVars, "SysDir",   szSysPath);
   VarSet (&pvVars, "OutDir",   szOutPath);

   /*--- Read command line ---*/
   if (!VarGet (pjob->pv, "ExecCmd", szExecTemplate))
      VarGet (pvCFG, "ExecCmd", szExecTemplate);
   if (!*szExecTemplate)
      strcpy (szExecTemplate, "@Job");

   VarResolve (szExecCmd, szExecTemplate, pvVars);

   if (strchr (szExecCmd, '@')) // still stuff to resolve?
      {
      strcpy (szExecTemplate, szExecCmd);
      VarResolve (szExecCmd, szExecTemplate, pvCFG);
      }

   FreeVar3 (pvVars);

   ppszCmd = StrMakePPSZ (szExecCmd, " ", FALSE, TRUE, NULL);

   flushall ();

   /*--- exec the job already goddamnit! ---*/
   JMLog (6, "About to spawn prog: %s", szExecCmd);
   JMLog (7, "--------- Job Exec Command ---------");
   for (i=0; ppszCmd[i]; i++)
      JMLog (7, "[%d] [%s]", i, ppszCmd[i]);
   JMLog (7, "------------------------------------");


   /*
    * Make JobDir the current drive and path added 6/22/95
    * This is done to allow a relative DPATH (i.e. ;.\DLL;)
    * This will allow a machine to run multiple servers
    * with different copies of the DLL's
    */
   chdir (szJobPath);
   if (*szJobPath && (szJobPath[1]==':'))
      DosSelectDisk (toupper (*szJobPath) - 'A' + 1);

   if (bDEBUG)
      {
      JMLog (3, "DEBUG: spawn commented out in debug mode");
      pjob->uRetCode =  0;
      MemFreePPSZ (ppszCmd, 0);
      return 0;
      }
   else
      {
      pjob->iPID = spawnvp (P_NOWAIT, ppszCmd[0], ppszCmd);
      JMLog (4, "Spawned prog: %s pid=%d", szExecCmd, pjob->iPID);
      }

   if (pjob->iPID == (USHORT) -1)
      {
      JMError ("Unable to exec prog: %s", szExecCmd);
      flushall ();
      MemFreePPSZ (ppszCmd, 0);
      pjob->uRetCode = JRC_CANNOTEXEC;
      return 10;
      }

   /*--- wait for job to finish, get return code ---*/
   iCStat = cwait (&iStat, pjob->iPID, WAIT_CHILD);
   time ((time_t *)&pjob->tmEndTime);
   JMLog (7, "Returned from cwait with: iCStat=%x & iStat=%x", iCStat, iStat);

   flushall ();
   MemFreePPSZ (ppszCmd, 0);

   if (pjob->uRetCode = XlateReturnCode (pjob, iCStat, iStat, errno))
      {
      psz = ReturnCodeString (pjob->uRetCode);
      JMError ("Prog Return Code: %s [errno=%d] %s", psz, errno, szExecCmd);
      }
   else
      {
      JMLog (5, "Prog Returned Successfully: %s", szExecCmd);
      }

   return pjob->uRetCode;

//   /*--- NORMAL RETURN ---*/
//   if (iCStat != -1)
//      {
//      pjob->uRetCode =  (iStat >> 8);
//      JMLog (6, "Exec Return Code: %d", iStat);
//      return 0;
//      }
//
//   /*--- ABNORMAL RETURN ---*/
//   else
//      {
//      pjob->uRetCode =  (iStat << 8);
//
//      if (errno == ECHILD || errno == EINVAL) // invalid cwait call
//         JMError ("Unable to exec prog: %s [errno=%d]", szExecCmd, errno);
//      else if (iStat == 1)
//         JMError ("Prog died with Hard Error: %s [errno=%d]", szExecCmd, errno);
//      else if (iStat == 2)
//         JMError ("Prog died with a Trap: %s [errno=%d]", szExecCmd, errno);
//      else if (iStat == 3)
//         JMError ("Prog died with a bad SIGTERM: %s [errno=%d]", szExecCmd, errno);
//      else
//         JMError ("Abnormal Exec Return Code: [Ret=%d] [errno=%d]", iStat, errno);
//
//      SetFlag (&pjob->uFlags, JSF_ABNORMALEXIT, TRUE);
//      return 10;
//      }
//   return 0;
   }


/*
 * > conditional print
 * > rename job ctl file
 * > keep or discard output and sys files
 * > ???
 *
 */
USHORT PostProcessJob (PJOB pjob)
   {
   char   szBuff    [256];
   char   szJobDir  [256];
   char   szSysDir  [256];
   char   szOutDir  [256];
   char   szNewName [256];
   char   szJobCtlFile [256];
   BOOL   bDebug, bNotify;

   JMLog (4, "PostProcessing JobID:%s Name:%s", pjob->pszJobID, pjob->pszJobName);

   FilesDir2 (szJobDir, pjob, 2);
   FilesDir2 (szSysDir, pjob, 1);
   FilesDir2 (szOutDir, pjob, 0);

   bDebug = (VarGet (pjob->pv, "TypRun", szBuff) && toupper(*szBuff) == 'D');

   /*--- rename job.ctl file. we dont want to reload job at restart ---*/
   if (!IsFlag (pjob->uFlags, JSF_RESTARTABLE))
      {
      sprintf (szJobCtlFile, "%s\\%s", szSysDir, JOB_CTL_FILE);
      NewExtension (szNewName, szJobCtlFile, "CTL.DAT");
      if (!access (szNewName, 0))
         unlink (szNewName);
      rename (szJobCtlFile, szNewName);
      }

   /*--- remove empty files ---*/
   RemoveEmptyFiles (szSysDir);
//   RemoveEmptyFiles (szOutDir);

   CreateBannerFile (pjob);

   if (!IsFlag (pjob->uFlags, JSF_TIMEDOUT)     &&  // job timed out
       !IsFlag (pjob->uFlags, JSF_KILLED)       &&  // job killed
       !IsFlag (pjob->uFlags, JSF_ABNORMALEXIT))    // job aborted
      {
      if (VarTrue (pjob->pv, "PrintOut"))
         PrintJob (pjob, bDebug);

     /*
      * As long as the output is immediately spooled, or the print exec is
      * synchronous, we are OK.  But if this is not the case, we need a way
      * to ensure that the files are printed before they are deleted.
      *
      * Currently, the files are immediately spooled.
      */
      if (VarGet (pjob->pv, "OutKeep", szBuff) && toupper(*szBuff) == 'N')
         {
         JMLog (5, "Deleting output JobID:%s Name:%s", pjob->pszJobID, pjob->pszJobName);
         RemoveDir (szJobDir, TRUE);
         }
      else
         {
         JMLog (5, "Keeping output JobID:%s Name:%s", pjob->pszJobID, pjob->pszJobName);
         CtlWriteLine (pjob);
         }
      }
   else // job was killed or died, so keep the files and write ctl line
      {
      JMLog (5, "Keeping aborted output JobID:%s Name:%s", pjob->pszJobID, pjob->pszJobName);
      CtlWriteLine (pjob);
      }

   /*--- we may need to notify the client ---*/
   if (VarGet (pjob->pv, "NotifyClient", szBuff))
      bNotify = VarTrue (pjob->pv, "NotifyClient");
   else
      bNotify = VarTrue (pvCFG, "NotifyClient");
   if (bNotify)
      NotifyClient (pjob);

   JMLog (7, "returning from post-process job Job:%s", pjob->pszJobID);
   return 0;
   }


