/*
 *
 * print.c
 * Thursday, 4/6/1995.
 *
 * (C) 1995 Info Tech Inc.
 * Craig Fitzgerald
 *
 *
 * Part of the BAMS Job Server
 */

#define INCL_DOS
#include <os2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <direct.h>
#include <io.h>
#include <Process.h>
#include <ctype.h>
#include <GnuStr.h>
#include <GnuFile.h>
#include <GnuMem.h>
#include <GnuMisc.h>
#include "Var.h"
#include "Bamsserv.h"
#include "UserIO.h"
#include "Util.h"
#include "Print.h"
#include "Stat.h"




/*
 * The print command used here uses the brute force method of 
 * redirecting stdout and stderr, if this becomes a problem, we
 * can go to the solution used to spawn the job (I.E. putting redirection
 * on the command line) used in RunJob.C:ProcessJob
 */
USHORT PrintFile (PSZ    pszFile, 
                  PSZ    pszPrinter, 
                  PSZ    pszServer, 
                  PSZ    pszForms, 
                  USHORT uCopies, 
                  BOOL   bDeleteFile)
   {
   PVAR   pvVars = NULL;
   FILE   *fpIn, *fpPrn;
   PSZ    *ppszCmd;
   USHORT uRet = 0;
   char   szBuff          [1024];
   char   szPrintCmd      [512];
   char   szPrintTemplate [256];
   int    stdouttmp, stderrtmp, iout, ierr;
   BOOL   bBlocking;

   bBlocking = VarTrue (pvCFG, "PrintWait");

   /*--- prepare cmd line ---*/
   VarSet (&pvVars, "File",         pszFile   );
   VarSet (&pvVars, "Printer",      pszPrinter);
   VarSet (&pvVars, "PrintServer",  pszServer );
   VarSet (&pvVars, "Forms",        pszForms  );
   VarSet (&pvVars, "Copies",       NumStr (szBuff, uCopies));

   VarGet (pvCFG,   "PrintCmd", szPrintTemplate);

   VarResolve (szPrintCmd, szPrintTemplate, pvVars);
   if (strchr (szPrintCmd, '@')) // still stuff to resolve?
      {
      strcpy (szPrintTemplate, szPrintCmd);
      VarResolve (szPrintCmd, szPrintTemplate, pvCFG);
      }

   ppszCmd = StrMakePPSZ (szPrintCmd, " ", FALSE, TRUE, NULL);

   /*--- Redirect stdout and stderr ---*/
   flushall ();
   if ((stdouttmp = dup (fileno (stdout))) != -1)
      freopen ("NUL", "w", stdout);

   if ((stderrtmp = dup (fileno (stderr))) != -1)
      freopen ("NUL", "w", stderr);

   /*--- do exec ---*/
   if (bDEBUG)
      {
      JMLog (3, "DEBUG: print spawn ignored in debug mode");
      }
   else
      {
      /*
       * If there is no print command, use 'prn' and copy the
       * file to the printer directly.
       */
      if (!*szPrintCmd)
         {
         /* If no print command is defined, send the output to the 
          * default locally defined printer. (I.E. 'prn')
          */
         JMLog (6, "Print: About to local print file", szPrintCmd);
         if (!(fpPrn = my_fopen ("PRN", "w")))
            {
            JMError ("Unable to open printer handle");
            uRet = JSE_CANNOT_PRINT;
            }
         else
            {
            if (!(fpIn  = my_fopen (pszFile, "r")))
               {
               JMError ("Unable to open file to print");
               uRet = JSE_CANNOT_FIND_FILE;
               }
            else
               {
               while (FilReadLine (fpIn, szBuff, NULL, sizeof (szBuff) != 0xFFFF))
                  fprintf (fpPrn, "%s\n", szBuff);
               fclose (fpIn);
               }
            fclose (fpPrn);
            if (bDeleteFile)
               unlink (pszFile);
            }
         }
      /*
       * There was a print command given, so use it
       */
      else
         {
         if (bDeleteFile || !bBlocking)
            {
            /*--- spooled files are cleaned up, and are always blocking ---*/
            JMLog (6, "Print: Blocking Spawn: %s", szPrintCmd);
            if (uRet = spawnvp (P_WAIT, ppszCmd[0], ppszCmd))
               {
               JMError ("Blocking Print return Code: %u", uRet);
               uRet = JSE_CANNOT_PRINT;
               }
            if (bDeleteFile)
               {
               if (!VarTrue (pvCFG, "KeepSpoolFile"))
                  unlink (pszFile);
               }
            }
         else
            {
            JMLog (6, "Print: Non-Blocking Spawn: %s", szPrintCmd);
            if ((uRet = spawnvp (P_NOWAIT, ppszCmd[0], ppszCmd)) == -1)
               {
               JMError ("Non-Blocking Print unsuccessful");
               uRet = JSE_CANNOT_PRINT;
               }
            }
        }
      }
   /*--- un-redirect the redirected redirectors directly ---*/
   iout = fileno (stdout);
   ierr = fileno (stderr);
   dup2 (stdouttmp, iout);           // undo redirection
   dup2 (stderrtmp, ierr);           // undo redirection
   close (iout);
   close (ierr);
   flushall ();

   /*--- cleanup ---*/
   MemFreePPSZ (ppszCmd, 0);
   FreeVar5 (pvVars);
   return uRet;
   }






/*
 * spools a file to the spool file
 *
 */
static BOOL SpoolFile (FILE *fpSpool, PSZ pszDir, PSZ pszFile)
   {
   char szBuff [512];
   char szSpec [256];
   FILE *fpIn;
   PSZ  psz;
   BOOL bFirstLine;

   sprintf (szSpec, "%s\\%s", pszDir, pszFile);
   if (!(fpIn = my_fopen (szSpec, "r")))
      return JMWarning ("Unable to open file %s", szSpec);

   JMLog (6, "Spooling file %s...", szSpec);

   if (VarTrue (pvCFG, "FixFormFeeds"))
      {
      bFirstLine = TRUE;
      while ((USHORT)FilReadLine (fpIn, szBuff, "", sizeof (szBuff)) != 0xFFFFU)
         {
         psz = szBuff;
         if (bFirstLine)   /*--- batch starts files with a \f ! ---*/
            {
            bFirstLine = FALSE;
            if (*psz == '\f')
               psz++;
            }

         for (; *psz; psz++)
            {
            fputc (*psz, fpSpool);
            if (*psz == '\f')      /*--- we need a \r after \f ---*/
               fputc ('\r', fpSpool);
            }
         fputc ('\n', fpSpool);
         }
      }
   else
      {
      while ((USHORT)FilReadLine (fpIn, szBuff, "", sizeof (szBuff)) != 0xFFFFU)
         fprintf (fpSpool, "%s\n", szBuff);
      }
   fprintf (fpSpool, "\f\r");
   fclose (fpIn);
   return TRUE;
   }


///*
// * spools a file to the spool file
// *
// */
//static BOOL SpoolFile (FILE *fpSpool, PSZ pszDir, PSZ pszFile)
//   {
//   char szBuff [512];
//   char szSpec [256];
//   FILE *fpIn;
//
//   sprintf (szSpec, "%s\\%s", pszDir, pszFile);
//   if (!(fpIn = my_fopen (szSpec, "r")))
//      return JMWarning ("Unable to open file %s", szSpec);
//
//   JMLog (6, "Spooling file %s...", szSpec);
//   while ((USHORT)FilReadLine (fpIn, szBuff, "", sizeof (szBuff)) != 0xFFFFU)
//      fprintf (fpSpool, "%s\n", szBuff);
//   fprintf (fpSpool, "\f");
//   fclose (fpIn);
//   return TRUE;
//   }




/*
 * Prints a job to the printer
 *
 */
BOOL PrintJob (PJOB pjob, BOOL bDebug)
   {
   FILEFINDBUF3 fbuf;
   char   szSysDir  [256];
   char   szOutDir  [256];
   char   szPrinter [256];
   char   szServer  [256];
   char   szForms   [256];
   char   szMatch   [256];
   char   szSpec    [256];
   char   szBanner  [256];
   char   szSpoolFile [256];
   USHORT uCopies, i;
   USHORT uAtts = FILE_NORMAL;
   FILE   *fpSpool;
   HDIR   hdir;
   PSZ    psz;
   PSTAT  pstat;

   JMLog (4, "Printing Job:%s Name:%s", pjob->pszJobID, pjob->pszJobName);

   FilesDir2 (szSysDir, pjob, 1);  // sys files dir
   FilesDir2 (szOutDir, pjob, 0);  // out files dir

   /*--- get values from params, cfg file, or get defaults ---*/
   if (!VarGet (pjob->pv, "Dest",   szPrinter))
      VarGet (pvCFG, "DefaultPrinter", szPrinter);

   if (psz = strchr (szPrinter, ','))
      {
      *psz++ = '\0';
      strcpy (szServer, psz);
      }
   else if (!VarGet (pjob->pv, "PrintServer",   szServer))
      VarGet (pvCFG, "DefaultPrintServer", szServer);

   if (!VarGet (pjob->pv, "Forms",   szForms))
      VarGet (pvCFG, "DefaultPrintForms", szForms);

   uCopies = min (100, max (1, (USHORT) VarGetl (pjob->pv, "Copies")));

   /*
    * First, we must spool all files to a tmp file
    * This is to ensure the files print in a contiguous block
    * The banner will come first
    */

   /*--- create spool file ---*/
   if (!(fpSpool = CreateTempFile ("w", NULL, "SPOO", szSpoolFile)))
      return JMError ("Unable to create spool file %s", szSpoolFile);

   /*--- print any printer setup codes ---*/
   WritePrinterCodes (szPrinter, szForms, fpSpool, TRUE);

   /*--- spool banner file to spool first ---*/
   if (!VarGet (pvCFG, "BannerFile", szBanner))
      strcpy (szBanner, BANNER_FILE);
   SpoolFile (fpSpool, szSysDir, szBanner);

   /*--- spool files in output dir next ---*/
   sprintf (szSpec, "%s\\*.*", szOutDir);
   hdir = 0;
   while (FindAFile (szMatch, &hdir, szSpec, uAtts, &fbuf))
      {
      if (!stricmp (szMatch, szBanner)) // this guy is already done
         continue;

      if (fbuf.cbFile < 3) // don't print empty files
         continue;

      for (i=0; i<uCopies; i++)
         {
         if (VarTrue (pvCFG, "PrintFileHeaders"))
            Identify (fpSpool, szMatch);
         SpoolFile (fpSpool, szOutDir, szMatch);
         }
      }

   /*--- if debugging is on, spool sys files ---*/
   if (bDEBUG || bDebug)
      {
      sprintf (szSpec, "%s\\*.*", szSysDir);
      hdir = 0;
      while (FindAFile (szMatch, &hdir, szSpec, uAtts, NULL))
         {
         if (!stricmp (szMatch, szBanner)) // this guy is already done
            continue;

         if (VarTrue (pvCFG, "PrintFileHeaders"))
            Identify (fpSpool, szMatch);

         SpoolFile (fpSpool, szSysDir, szMatch);
         }
      }
   /*--- print any printer term codes ---*/
   WritePrinterCodes (szPrinter, szForms, fpSpool, FALSE);
   fclose (fpSpool);

   JMLog (6, "Printing the spooler file %s", szSpoolFile);
   PrintFile (szSpoolFile, szPrinter, szServer, szForms, uCopies, TRUE);

   pstat = StatAccess (TRUE);
   pstat->uJobsPrinted++;
   pstat = StatAccess (FALSE);

   return TRUE;
   }



/*
 * This code was un-ceremoniously
 * copied from the printc source
 */
USHORT XlatePrintCodes (PSZ pszDest, PSZ pszSrc)
   {
   int    uCount = 0;
   CHAR   ch, chr;

   if (!pszSrc || !*pszSrc || !pszDest)
      return 0;

   /*--- skip leading space ---*/
   while (*pszSrc == ' ')
      pszSrc++;

   while (1)
      {
      ch = *pszSrc++;

      if (!ch || ch == '\n')
         break;
      if (ch == ' ')
         continue;
      if (ch == '\\')       
         {
         if (!*pszSrc)
            Error ("bad \\ format: unexpected eol.");

         if (*pszSrc < '0' || *pszSrc > '9')      /*--- \char ---*/
            chr = *pszSrc++;

         else if (toupper (pszSrc[2]) == 'H')     /*--- \00h ---*/
            {
            ch = (CHAR) toupper (*pszSrc++);
            if (ch < '0' || ch > 'H' || (ch > '9' && ch < 'A'))
               Error ("Invalid char in \\00h format");
            chr = (CHAR) (16 * (ch - (ch > '9' ? 'A' + 10 : '0')));
            ch = (CHAR) toupper (*pszSrc++);
            if (ch < '0' || ch > 'H' || (ch > '9' && ch < 'A'))
               Error ("Invalid char in \\00h format");
            chr += (CHAR) (ch - (ch > '9' ? 'A' - 10 : '0'));
            }
         else                                     /*--- \000 ---*/
            {
            if (pszSrc[0] < '0' || pszSrc[0] > '9'  ||
                pszSrc[1] < '0' || pszSrc[1] > '9'  ||
                pszSrc[2] < '0' || pszSrc[2] > '9')
               Error ("Invalid char in \000 format");

            chr  = (CHAR)(100 * ((*pszSrc++ - '0') % 10));
            chr += (CHAR)(10  * ((*pszSrc++ - '0') % 10));
            chr += (CHAR)(      ((*pszSrc++ - '0') % 10));
            }
         }
      else if (ch == '~')
         chr = (CHAR) 27;

      else if (ch == '^')
         {
         ch = (CHAR) toupper (*pszSrc++);
         if (ch < 'A' || ch > 'Z')
            Error ("Illegal char in ^<char> format");
         chr = (CHAR)(ch - 'A' + 1);
         }

      else
         chr = ch;

      pszDest[uCount++] = chr;
      }
   pszDest[uCount++] = '\0';
   return (CHAR)(uCount);
   }



/*
 *
 *
 *
 */
BOOL WritePrinterCodes (PSZ pszPrinter, PSZ pszForms, FILE *fp, BOOL bInit)
   {
   char szBuff    [256];
   char szDest    [256];
   char szPrinter [128];
   char szForms   [128];
   char szVar     [128];
   PSZ  psz;
   USHORT uLen, i, j;

   if (!pszPrinter || !pszForms || !*pszPrinter || !*pszForms)
      return FALSE;

   /*--- copy printer name, strip server if appended ---*/
   strcpy (szForms, pszForms);
   strcpy (szPrinter, pszPrinter);
   if (psz = strchr (szPrinter, ','))
      *psz = '\0';

   StrStrip (StrClip (szPrinter, " \t"), " \t");
   StrStrip (StrClip (szForms,   " \t"), " \t");

   /*--- look for specific printer, then default ---*/
   for (i=0; i<2; i++)
      {
      if (i == 1)
         strcpy (szPrinter, "Default");

      if (bInit)
         sprintf (szVar, "%s_%s_Codes", szPrinter, szForms);
      else
         sprintf (szVar, "%s_Reset", szPrinter);

      if (!VarGet (pvCFG, szVar, szBuff))
         continue;

      for (j=0; j<10; j++) // resolve up to 10 levels
         if (strchr (szBuff, '@'))
            {
            VarResolve (szDest, szBuff, pvCFG);
            strcpy (szBuff, szDest);
            }
      StrStrip (StrClip (szBuff, " \t"), " \t");
      uLen = XlatePrintCodes (szDest,  szBuff);
      fwrite (szDest, 1, uLen, fp);
      return TRUE;
      }
   return FALSE;
   }

