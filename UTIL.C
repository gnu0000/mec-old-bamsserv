/*
 *
 * util.c
 * Thursday, 1/5/1995.
 *
 * (C) 1995 Info Tech Inc.
 * Craig Fitzgerald
 *
 * Part of the BAMS Job Server
 * This mod contains various utility functions
 */

#define INCL_DOS
#include <os2.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <io.h>
#include <time.h>
#include <ctype.h>
#include <direct.h>
#include <process.h>
#include <errno.h>
#include <GnuStr.h>
#include <GnuFile.h>
#include <GnuMem.h>
#include <GnuMisc.h>
#include "Var.h"
#include "Bamsserv.h"
#include "UserIO.h"
#include "Util.h"


/**************************************************************************/
/*                                                                        */
/*                                                                        */
/*                                                                        */
/**************************************************************************/


/*
 * This is basically a fopen that
 * does not allow hard errors
 *
 */
FILE *my_fopen (PSZ pszFile, PSZ pszMode)
   {
   FILE *fp;

   DosError (HARDERROR_DISABLE);
   fp = fopen (pszFile, pszMode);
   DosError (HARDERROR_ENABLE);
   return fp;
   }



/*
 * returns a 'random' number from 0 to iLimit-1
 *
 */
SHORT Rnd (SHORT iLimit)
   {
   return (SHORT) ((float)iLimit*rand()/(RAND_MAX+1.0));
   }



/*
 * This fn creates threads
 * This is encapsulated so that you can change
 * DosCreateThread to _beginthread and vise versa
 * #define THREADFN VOID APIENTRY    for DosBeginThread
 *
 */
ULONG StartThread (THREADFN pfn, ULONG ulArg)
   {
//   ULONG ID;
//
//   return DosCreateThread (&ID, pfn, ulArg, 0, STACK_SIZE);

   return _beginthread (pfn, NULL, STACK_SIZE, (PVOID)ulArg);
   }



/*
 * This fn concatenates n strings together
 * and returns that string
 */
PSZ FNTYPE cat (PSZ pszDest, PSZ pszSrc, ...)
   {
   va_list vlst;

   if (!pszDest)
      return NULL;

   *pszDest = '\0';

   va_start (vlst, pszSrc);
   while (pszSrc)
      {
      strcat (pszDest, pszSrc);
      pszSrc = va_arg (vlst, PSZ);
      }
   va_end (vlst);
   return pszDest;
   }



/*
 */
PSZ NumStr (PSZ psz, USHORT i)
   {
   sprintf (psz, "%u", i);
   return psz;
   }



/*
 *
 *
 */
BOOL PathExists (PSZ pszPath)
   {
   BOOL bExists;
   char szCurrPath [256];

   getcwd (szCurrPath, sizeof szCurrPath);
   if (bExists = !chdir (pszPath))
      chdir (szCurrPath);

   return bExists;
   }



/*
 * This is a findfirst/findnext type thingie
 * This fn is re-entrant
 * the calling fn must create a hdir = 0; for the first call
 * The last param may be null.
 * This fn returns NULL when there are no matches left.  
 * Always call this fn until it returns NULL, as it triggers a cleanup
 * Alternatively, you may do a "DosFindClose (hdir)" yourself to cleanup
 *
 */
PSZ FindAFile (PSZ          pszBuff, // buffer to hold new name
               HDIR         *phdir,  // handle, start with 0
               PSZ          pszBase, // base dir to search (incl wildcard)
               USHORT       uAtts,   // attributes to match
               FILEFINDBUF3 *pfbuf)  // file struct, can be NULL
   {
   FILEFINDBUF3 fbuf, *pfbufTmp;
   ULONG       ulCount = 1;

   pfbufTmp = (pfbuf ? pfbuf : &fbuf); 

   while (TRUE)
      {
      if (!*phdir)   // Find First
         {
         *phdir = HDIR_CREATE;
         if (DosFindFirst(pszBase, phdir, uAtts, pfbufTmp, sizeof(fbuf), &ulCount, 1))
            {
            DosFindClose (*phdir);
            return NULL;
            }
         }
      else if (DosFindNext (*phdir, pfbufTmp, sizeof(fbuf), &ulCount))
         {
         DosFindClose (*phdir);
         return NULL;
         }
      /*
       * Warning! The @#$@#$^$@^$%%#$%^ ICBM C/C++ library
       * is screwed up yet again.
       * the DosFindFirst now implicitly includes all normal files in the 
       * search spec under all conditions. So setting uAtts to FILE_DIRECTORY
       * will return all matching files in the current dir as well.
       */
      if (uAtts == FILE_DIRECTORY && !(pfbufTmp->attrFile & FILE_DIRECTORY))
         continue;

      if (*pfbufTmp->achName != '.')  // exclude . and .. from matches
         return strcpy (pszBuff, pfbufTmp->achName);
      }
   }


/**************************************************************************/
/*                                                                        */
/*                                                                        */
/*                                                                        */
/**************************************************************************/


/*
 *
 *
 *
 */
PSZ NewExtension (PSZ pszNewPath, PSZ pszOldPath, PSZ pszExt)
   {
   PSZ p, p2;

   strcpy (pszNewPath, pszOldPath);
   StrStrip (pszNewPath, " \t");
   if (p = strchr (pszNewPath, ' '))
      *p = '\0';

   if (p = strrchr (pszNewPath, '\\'))
      p++;
   else
      p = pszNewPath;
   if (p2 = strrchr (p, '.'))
      *p2 = '\0';
   strcat (pszNewPath, pszExt);
   return pszNewPath;
   }



/*
 * This fn takes a string, and works it over until it is
 * a valid file/dir name
 *
 */
PSZ MakeValidDirName (PSZ pszDest, PSZ pszSrc)
   {
   PSZ   psz;

   if (!pszSrc || !*pszSrc)
      return strcpy (pszDest, "UNKNOWN");
   strcpy (pszDest, pszSrc);

   /*---limit len ---*/
   if (strlen (pszDest) > 8)
      pszDest[8] = '\0';

   /*--- remove bad chars ---*/
   for (psz = pszDest; *psz; psz++)
      if (!isalnum (*psz))
         *psz = '-';

   return pszDest;
   }



/*
 *  This function converts a relative time to an absolute time
 *
 *  if tm is <= 1000 it is considered a relative time which is
 *  expressed in terms of hours from the present.
 *
 *  example tm=1 is converted to 1 hour ago
 *
 */
TIME FixTime (TIME tm)
   {
   if (tm <= 1000)
      return time (NULL) - tm * 3600;
   return tm;
   }



/*
 *
 *
 *
 */
PSZ TimeString (PSZ pszBuff, TIME time)
   {
   struct tm *tim;
   BOOL   bPM;

   if (!time)
      return strcpy (pszBuff, "00:00:00");

   tim = localtime ((time_t *)&time);

   bPM = (tim->tm_hour >= 12);
   if (tim->tm_hour > 12)
      tim->tm_hour -= 12;

   sprintf (pszBuff, "%2.2d:%2.2d:2.2d %cm", 
            tim->tm_hour, tim->tm_min, tim->tm_sec (bPM ? 'p' : 'a'));

   return pszBuff;
   }



/*
 *
 *
 *
 */
PSZ DateString (PSZ pszBuff, TIME time)
   {
   struct tm *tim;

   if (!time)
      return strcpy (pszBuff, "00/00/00");

   if (tim = localtime ((time_t *)&time))
      sprintf (pszBuff, "%2.2d/%2.2d/%2.2d", tim->tm_mon+1, tim->tm_mday, tim->tm_year);
   else
      strcpy (pszBuff, "INVALID");
   return pszBuff;
   }


PSZ DateTimeString (PSZ pszBuff, TIME time)
   {
   char sz1[32], sz2[32];

   sprintf (pszBuff, "%s %s", DateString (sz1, time), TimeString (sz2, time));
   return pszBuff;
   }



PSZ DeltaTimeString (PSZ pszBuff, TIME time, USHORT uDiv)
   {
   struct tm;
   USHORT uHours, uMinutes, uSeconds;

   if (!uDiv || !time)
      return strcpy (pszBuff, "00h 00m 00s");

   time /= (TIME)uDiv;

   uHours   = time / 3600L;
   uMinutes = (time - (LONG)uHours * 3600L) / 60L;
   uSeconds = time - (LONG)uHours * 3600L - (LONG)uMinutes * 60L;

   sprintf (pszBuff, "%2.2dh %2.2dm %2.2ds", uHours, uMinutes, uSeconds);
   return pszBuff;
   }

   


/*
 * Deletes all files in the dir
 * and then removes the dir
 */
BOOL RemoveDir (PSZ pszDir, BOOL bKillChildren)
   {
   HDIR hdir = 0;
   char szSpec     [256];
   char szSubDir   [256];
   char szFile     [256];
   char szChildDir [256];

   sprintf (szSpec, "%s\\*.*", pszDir);

   if (bKillChildren)
      while (FindAFile (szSubDir, &hdir, szSpec, FILE_DIRECTORY, NULL))
         {
         sprintf (szChildDir, "%s\\%s", pszDir, szSubDir);
         RemoveDir (szChildDir, bKillChildren);
         }
   hdir = 0;
   while (FindAFile (szFile, &hdir, szSpec, FILE_NORMAL, NULL))
      {
      sprintf (szChildDir, "%s\\%s", pszDir, szFile);
      unlink (szChildDir);
      }
   return !rmdir (pszDir);
   }



/*
 * This file deletes all closed, empty files in pszDir
 *
 *
 */
void RemoveEmptyFiles (PSZ pszDir)
   {
   HDIR         hdir = 0;
   FILEFINDBUF3 fbuf;
   char         szSpec     [256];
   char         szFile     [256];
   char         szFileSpec [256];

   sprintf (szSpec, "%s\\*.*", pszDir);

   while (FindAFile (szFile, &hdir, szSpec, FILE_NORMAL, &fbuf))
      {
      if (fbuf.cbFile) // does file have a size?
         continue;     // ok

      sprintf (szFileSpec, "%s\\%s", pszDir, szFile);
      unlink (szFileSpec);
      }
   }



/*
 * Strips away leading path info about a file
 * optionally strips away extension
 *
 */
PSZ BaseName (PSZ pszBase, PSZ pszFullPath, BOOL bKeepExt)
   {
   PSZ p;

   if (p = strrchr (pszFullPath, '\\'))
      p++;
   else if (p = strrchr (pszFullPath, ':'))
      p++;
   else
      p = pszFullPath;

   strcpy (pszBase, p);
   if (p = strchr (pszBase, ' '))
      *p = '\0';

   if (!bKeepExt && (p = strchr (pszBase, '.')))
      *p = '\0';
   return pszBase;
   }



/**************************************************************************/
/*                                                                        */
/*                                                                        */
/*                                                                        */
/**************************************************************************/

/*
 * This fn displays an error to the log window
 * and to the log file
 * as long as log values are > 0
 */
USHORT FNTYPE JMError (PSZ psz, ...)
   {
   char szBuff [512];
   char szBuff2 [256];
   va_list vlst;

   va_start (vlst, psz);
   vsprintf (szBuff, psz, vlst);
   va_end (vlst);

   sprintf (szBuff2, "Error: %s", szBuff);

   if (uLOG_WINDOW_LEVEL)
      UpdateWindow (JM_ERROR_WINDOW, szBuff2);

   if (bLOG && uLOG_FILE_LEVEL)
      fprintf (fpLOGFILE, "%s\n", szBuff2);

   return 0;
   }



/*
 * This fn displays a warning to the log window
 * and to the log file
 * as long as log values are > 1
 */
USHORT FNTYPE JMWarning (PSZ psz, ...)
   {
   char szBuff [256];
   char szBuff2 [256];
   va_list vlst;

   va_start (vlst, psz);
   vsprintf (szBuff, psz, vlst);
   va_end (vlst);

   sprintf (szBuff2, "Warning: %s", szBuff);

   if (uLOG_WINDOW_LEVEL > 1)
      UpdateWindow (JM_ERROR_WINDOW, szBuff2);

   if (bLOG && uLOG_FILE_LEVEL > 1)
      fprintf (fpLOGFILE, "%s\n", szBuff2);

   return 0;
   }




/*
 * Send a message to the window/logfile
 * you specify the minimum log level in uLogVal
 *
 */
USHORT FNTYPE JMLog (USHORT uLogVal, PSZ psz, ...)
   {
   char szBuff [512];
   va_list vlst;

   va_start (vlst, psz);
   vsprintf (szBuff, psz, vlst);
   va_end (vlst);

   if (uLOG_WINDOW_LEVEL >= uLogVal)
      UpdateWindow (JM_ERROR_WINDOW, szBuff);

   if (bLOG && uLOG_FILE_LEVEL >= uLogVal)
      fprintf (fpLOGFILE, "%s\n", szBuff);

   return 0;
   }



/*
 * Given a userID, this returns the path to his Files dir
 *
 * This fn can Handle Jos‚ and O'Malley
 *
 */
PSZ UserDir (PSZ pszPath, PSZ pszUserID)
   {
   char szBase [256];
   char szUser [256];

   MakeValidDirName (szUser, pszUserID);

   VarGet (pvCFG, "FilesDir", szBase);
   StrClip (szBase, "\\ \t");

   if (!*szBase || chdir(szBase))
      return NULL;

   sprintf (pszPath, "%s\\%s", szBase, szUser);

   if (chdir(pszPath))
      if (mkdir (pszPath))
         {
         /*--- *error cannot make user path* ---*/
         }
   return pszPath;
   }



/*
 * returns path to proper files dir, given lots of ugly params
 *
 */
PSZ FilesDir0 (PSZ pszBuff, PSZ pszUserID, PSZ pszJobID, USHORT bSysFiles)
   {
   char szBuff [256];
   char szBuff2[256];

   UserDir (pszBuff, pszUserID);
   strcat (pszBuff, "\\");
   strcat (pszBuff, pszJobID);

   if (bSysFiles > 1)
      *szBuff = '\0';
   else if (bSysFiles) /*--- System Files Dir ---*/
      VarGet (pvCFG, "SystemFilesDir", szBuff);
   else           /*--- Output Files Dir ---*/
      VarGet (pvCFG, "OutputFilesDir", szBuff);

   StrClip (StrStrip(szBuff, " \t"), "\\ \t");

   if (*szBuff)
      {
      MakeValidDirName (szBuff2, szBuff);
      strcat (pszBuff, "\\");
      strcat (pszBuff, szBuff2);
      }
   return pszBuff;
   }



/*
 * returns path to proper files dir, given pcmd
 *
 */
PSZ FilesDir (PSZ pszBuff, PCOMMAND pcmd, BOOL bSysFiles)
   {
   return FilesDir0 (pszBuff, pcmd->pszUserID, pcmd->pszJobID, bSysFiles);
   }



/*
 * returns path to proper files dir, given pjob
 *
 */
PSZ FilesDir2 (PSZ pszBuff, PJOB pjob, BOOL bSysFiles)
   {
   return FilesDir0 (pszBuff, pjob->pszUserID, pjob->pszJobID, bSysFiles);
   }

/*
 *
 *
 *
 */
PJOB MakeJob (void)
   {
   PJOB pjob;

   pjob = calloc (1, sizeof (JOB));
//   pjob->ulTest = 12345678;
   return pjob;
   }



/*
 *
 *
 *
 */
PJOB FreeJob (PJOB pjob)
   {
//   if (pjob->ulTest != 12345678)
//      {
//      JMError ("***Job has been hosed! ID=%s Name=%s User=%s", 
//            pjob->pszJobID, pjob->pszJobName, pjob->pszUserID);
//      }

   if (pjob->pszJobID   ) free (pjob->pszJobID   );
   if (pjob->pszJobName ) free (pjob->pszJobName );
   if (pjob->pszJobTag  ) free (pjob->pszJobTag  );
   if (pjob->pszJobDesc ) free (pjob->pszJobDesc );
   if (pjob->pszUserID  ) free (pjob->pszUserID  );
   if (pjob->pszClientID) free (pjob->pszClientID);
   FreeVar4 (pjob->pv);
   free (pjob);
   return NULL;
   }



/*
 *
 *
 *
 */
PCOMMAND FreeCommand (PCOMMAND pcmd)
   {
   if (pcmd->pszUserID  ) free (pcmd->pszUserID  );
   if (pcmd->pszClientID) free (pcmd->pszClientID);
   if (pcmd->pszServerID) free (pcmd->pszServerID);
   if (pcmd->pszJobID   ) free (pcmd->pszJobID   );
   if (pcmd->pszUser1   ) free (pcmd->pszUser1   );
   if (pcmd->pszUser2   ) free (pcmd->pszUser2   );
   if (pcmd->pszUser3   ) free (pcmd->pszUser3   );
   free (pcmd);
   return NULL;
   }


PSZ TempDir (PSZ pszDir, PSZ pszDefault)
   {
   PSZ psz;

   if (VarGet (pvCFG, "TempDir", pszDir))
      ;
   else if (psz = getenv ("TMP")) 
      strcpy (pszDir, psz);
   else if (psz = getenv ("TEMP")) 
      strcpy (pszDir, psz);
   else if (pszDefault)
      strcpy (pszDir, psz);
   else 
      strcpy (pszDir, "");
   if (*pszDir && (pszDir[strlen (pszDir)] != '\\'))
      strcat (pszDir, "\\");
   return pszDir;
   }



/*
 * creates a temporary file, opens it, and returns its handle
 * pszBase, if non NULL, should be no longer than 4 chars
 *
 */
FILE *CreateTempFile (PSZ pszMode, PSZ pszDir, PSZ pszBase, PSZ pszTempFile)
   {
   char   szDir  [256];
   char   szFile [256];
   USHORT uVal, i;
   FILE   *fp;
   PSZ    pszBaseName;

   /*--- Get the temp file's dir ---*/
   if (!pszDir)
      TempDir (szDir, NULL);
   else
      {
      strcpy (szDir, pszDir);
      if ((szDir[strlen (szDir)] != '\\'))
         strcat (szDir, "\\");
      }

   /*--- find a new name for the file ---*/
   pszBaseName = (pszBase ? pszBase : "BAMS");
   srand ((USHORT)time(NULL));

   /*--- find new name, i only go 100 times, after that i collide ---*/
   for (i=0; i<100; i++)
      {
      uVal = rand() % 10000;
      sprintf (szFile, "%s%s%4.4d.TMP", szDir, pszBaseName, uVal);
      if (access (szFile, 0))
         i=32767;             // IE: break!
      }
   if (!(fp = my_fopen (szFile, pszMode)))
      return NULL;

   if (pszTempFile)
      {
      *pszTempFile = '\0';
      strcpy (pszTempFile, szFile);
      }
   return fp;      
   }



/**************************************************************************/
/*                                                                        */
/*                                                                        */
/*                                                                        */
/**************************************************************************/
/*
 * Change priority of process or thread
 * lDelta is added to the priority delta
 *
 */
void SetPriority (PSZ pszPClass, PSZ pszPDelta, LONG lDelta, BOOL bThread)
   {
   LONG lClass, lLevel, lScope;

   lClass = VarGetl (pvCFG, pszPClass);
   lLevel = VarGetl (pvCFG, pszPDelta);
   lScope = (bThread ? PRTYS_THREAD : PRTYS_PROCESS);

   if (lClass || lLevel)
      DosSetPriority (lScope, lClass, lLevel + lDelta, 0);
   }





#define IDWIDTH 78

/*
 * Writes a file header to the fp
 *
 */
void Identify (FILE *fp, PSZ pszStr)
   {
   int i;

   for (i=0; i<IDWIDTH; i++)    fprintf (fp, "*");
   fprintf (fp, "\n*");
   for (i=0; i<IDWIDTH-2; i++)  fprintf (fp, " ");
   fprintf (fp, "*\n*");
   for (i=0; i< (IDWIDTH-2-strlen (pszStr))/2; i++) fprintf (fp, " ");
   fprintf (fp, "%s", pszStr);
   for (i=0; i< (IDWIDTH-1-strlen (pszStr))/2; i++) fprintf (fp, " ");
   fprintf (fp, "*\n*");
   for (i=0; i<IDWIDTH-2; i++)  fprintf (fp, " ");
   fprintf (fp, "*\n");
   for (i=0; i<IDWIDTH; i++)    fprintf (fp, "*");
   fprintf (fp, "\n");
   }



extern int XlateReturnCode (PJOB pjob, int iCStat, int iStat, int errnocpy)
   {
   int iRet;

   if (IsFlag (pjob->uFlags, JSF_TIMEDOUT))      // job timed out
      iRet = JRC_TIMEDOUT;

   else if (IsFlag (pjob->uFlags, JSF_RESTARTABLE)) // job stopped by shutdown command
      iRet = JRC_RESTARTABLE;

   else if (IsFlag (pjob->uFlags, JSF_KILLED))   // job killed by user
      iRet = JRC_KILLED;

   else if (iCStat != -1)                        // normal return 
      {
      switch (iStat >> 8) 
         {
         case 0: iRet = JRC_OK;        break;
         case 1: iRet = JRC_APPERROR;  break;
         case 2: iRet = JRC_STOP;      break;
         case 3: iRet = JRC_PLIERROR;  break;
         default:iRet = JRC_UNKNOWN;   break;
         }
      }

   else if (errnocpy == ECHILD || errnocpy == EINVAL)  // invalid cwait call
      iRet = JRC_CANNOTEXEC;

   else  // abnormal return
      switch (iStat && 0xFF) 
         {
         case 1: iRet = JRC_HARDERROR; break;
         case 2: iRet = JRC_TRAP;      break;
         case 3: iRet = JRC_SIGTERM;   break;
         default:iRet = JRC_UNKNOWN;   break;
         }
   return iRet;
   }


extern PSZ ReturnCodeString (int iStat)
   {
   switch (iStat)
      {
      case JRC_OK:          return "Completed Successfully";
      case JRC_APPERROR:    return "Application Error";
      case JRC_STOP:        return "Stop Encountered";
      case JRC_PLIERROR:    return "PL/I Error";
      case JRC_HARDERROR:   return "Hard Error";
      case JRC_TRAP:        return "Trap Error";
      case JRC_SIGTERM:     return "SigTerm Error";
      case JRC_TIMEDOUT:    return "Timed Out";
      case JRC_KILLED:      return "Aborted by User";
      case JRC_RESTARTABLE: return "Shutdown Aborted";
      case JRC_UNKNOWN:     return "Unknown Error";
      case JRC_CANNOTEXEC:  return "Cannot Run Job";
      default:              return "Unknown Error #2";
      }
   }
