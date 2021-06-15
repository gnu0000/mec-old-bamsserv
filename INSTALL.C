/*
 *
 * install.c
 * Tuesday, 3/21/1995.
 *
 * (C) 1995 Info Tech Inc.
 * Craig Fitzgerald
 *
 *
 * Part of the BAMS Job Server
 *
 * Compile with MSC 6.0
 */

#define INCL_VIO
#define INCL_DOS
#include <os2.h>
#include <stdio.h>
#include <io.h>
#include <stdlib.h>
#include <string.h>
#include <direct.h>
#include <stdarg.h>
#include <GnuArg.h>
#include <GnuCfg.h>
#include <GnuScr.h>
#include <GnuStr.h>
#include <GnuDes.h>
#include <GnuRes.h>
#include <GnuFile.h>
#include <GnuKbd.h>
#include <GnuMisc.h>
#include <GnuZip.h>
#include <GnuMem.h>
#include "var.h"
#include "readebl.h"
#include "eblib.h"
#include "Bamsserv.h"   //contains VERSTR "0.61·"



#define ENTRIES   7
#define FIELDSIZE 54

#define P_SERVER     0
#define P_OUTFILES   1
#define P_JOBS       2
#define P_TEMPLATES  3
#define P_CONFIGS    4
#define P_IO         5
#define P_TMP        6

extern char szUsage[];
extern char szIntro[];
extern char szExtro[];
extern char szAbort[];

PVAR pvCFG = NULL;
char ppszPath [ENTRIES][128];
USHORT uSTART = 1;
PSZ  pszRES;

BOOL bMAKEALLDIRS = FALSE;
BOOL bDEBUG       = FALSE;

char   sz   [4096];   // general buff
char   sz2  [4096];   // general buff
char   szY  [36000U]; // i/o compresion Buff

/*
 * User Specified directories
 * --------------------------
 *
 * 0 Batch Server
 * 1 Output Files
 * 2 Batch Jobs
 * 3 Batch Job Templates
 * 4 Import/Export Dir
 * 
 */



/*
 * This fn concatenates n strings together
 * and returns that string
 */
PSZ _cdecl cat (PSZ pszDest, PSZ pszSrc, ...)
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
 *
 *
 */
BOOL Usage (void)
   {
   return TRUE;
   }


/*
 *
 *
 */
BOOL Quit (BOOL bFinished)
   {
   PMET pmet;
   PGW  pgw;

   pmet = ScrGetMetrics ();

   if (!bFinished)
      {
      pgw = GnuCreateWin (15, 60, NULL);
      GnuPaint (pgw,  1, 0,  3, 1, "ABORTING INSTALLATION");
      GnuPaint (pgw,  2, 0,  3, 1, "ﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂ");
      GnuPaintBig (pgw, 4, 2, 10, 56, 0, 0, szAbort);
      GnuPaint (pgw,  12,0,  3, 1, "Press any key to exit.");
      KeyGet (0);
      GnuDestroyWin (pgw);
      }
   ScrRestoreMode ();

   /*--- cls ---*/
   VioScrollDn (0, 0, 0xFFFF, 0xFFFF, 0xFFFF, pmet->bcOriginal, 0);
   GnuMoveCursor (NULL, 0, 0);
   ScrShowCursor (TRUE);

   exit (!bFinished);
   return TRUE;
   }


BOOL MsgWin (PSZ pszHeader, PSZ psz1, PSZ psz2, PSZ pszFooter)
   {
   PGW      pgw;
   PMET     pmet;
   USHORT   uXSize, c;

   pmet = ScrGetMetrics ();
   uXSize = min (pmet->uXSize-2U, max (30U, max (strlen (psz1), strlen (psz2))+6U));
   KeyClearBuff ();
   ScrSaveCursor (FALSE);
   pgw = GnuCreateWin (8, uXSize, NULL);

   GnuPaint (pgw, 1, 0, 3, 1, pszHeader);
   GnuPaint (pgw, 2, 0, 3, 0, psz1);
   GnuPaint (pgw, 3, 0, 3, 0, psz2);
   GnuPaint (pgw, 5, 0, 3, 1, pszFooter);
   c = KeyChoose ("\x1B\x0D", "");
   GnuDestroyWin (pgw);
   ScrRestoreCursor ();

   return (c == '\x0D');
   }


BOOL ErrWin (PSZ psz1, PSZ psz2)
   {
   MsgWin ("Error:", psz1, psz2, "Press <Return> to continue");
   return Quit (FALSE);
   }


/*
 * cleans up the screen
 *
 */
BOOL Intro (void)
   {
   PGW pgw;
   int c;

   pgw = GnuCreateWin (18, 76, NULL);

   GnuPaint (pgw,  1, 0,  3, 1, "BAMS Batch Server Installation Program");
   GnuPaint (pgw,  2, 0,  3, 1, "ﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂ");
   GnuPaint (pgw,  4, 0,  3, 0, "Copyright (c) 1995 by Info Tech. Inc.");
   GnuPaint (pgw,  5, 0,  3, 0, "All Rights Reserved.");

   sprintf (sz2, szIntro, VERSTR);
   GnuPaintBig (pgw, 7, 2, 9, 70, 0, 0, sz2);
   GnuPaint (pgw, 15, 0,  3, 1, "Press <Enter> to continue or <ESC> to abort installation.");

   c = KeyChoose ("\x1B\x0D", "");
   GnuDestroyWin (pgw);
   return (c != '\x1B');
   }


/*
 * cleans up the screen
 *
 */
void Extro (void)
   {
   PGW pgw;

   pgw = GnuCreateWin (20, 76, NULL);

   GnuPaint (pgw,  1, 0,  3, 1, "BAMS Batch Server Installation Program");
   GnuPaint (pgw,  2, 0,  3, 1, "ﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂ");
   GnuPaint (pgw,  4, 0,  3, 0, "Copyright (c) 1995 by Info Tech. Inc.");
   GnuPaint (pgw,  5, 0,  3, 0, "All Rights Reserved.");

   sprintf (sz2, szExtro, VERSTR, ppszPath[P_SERVER], ppszPath[P_JOBS]);
   GnuPaintBig (pgw, 7, 2, 9, 70, 0, 0, sz2);
   GnuPaint (pgw, 16, 0,  3, 1, "Press any key to quit.");
   KeyGet (0);
   GnuDestroyWin (pgw);
   return;
   }


/*
 * create filespec given path information 
 * copy path, append \ if needed
 * append pszFile if no prog there already
 *
 */
PSZ FillPathSpec (PSZ pszFull, PSZ pszPath, PSZ pszFile)
   {
   strcpy (pszFull, pszPath);

   if (*pszPath && (!stristr (pszPath, ".EXE")) && (pszPath[strlen(pszPath)-1] != '\\'))
      strcat (pszFull, "\\"); 

   if (*pszPath && (!stristr (pszPath, ".EXE")))
      strcat (pszFull, pszFile);

   return pszFull;
   }



BOOL DriveExists (PSZ pszPath)
   {
   USHORT uCurrentDisk;
   ULONG  ulDrives;

   if (!pszPath || !*pszPath) // no drive, so were ok
      return TRUE;
   if (!strrchr (pszPath, ':')) // no drive spec.
      return TRUE;
   if (pszPath[1] != ':')     // illegal spec, so were not ok
      return FALSE;
   if (*pszPath=='a' || *pszPath=='A' || *pszPath=='b' || *pszPath=='B')
      return TRUE;
   DosError (HARDERROR_DISABLE);
   DosQCurDisk (&uCurrentDisk, &ulDrives);
   DosError (HARDERROR_ENABLE);

   return (BOOL) ((ulDrives >> (toupper (*pszPath) - 'A')) & 1);
   }


BOOL ErrorWin (USHORT i, PSZ psz)
   {
   PGW  pgw;

   pgw = GnuCreateWin (12, 70, NULL);

   GnuPaint (pgw,  1, 0, 3, 1, "CREATE PATH ERROR");
   GnuPaint (pgw,  2, 0, 3, 1, "ﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂ");
   GnuPaint (pgw,  5, 5, 3, 1, psz);
   GnuPaint (pgw,  8, 0, 3, 1, "press <Enter> to continue");
   if (i == 0)
      {
      GnuPaint (pgw,  4, 2, 3, 0, "The path:");
      GnuPaint (pgw,  6, 2, 3, 0, "Could not be created");
      }
   else if (i == 1)
      {
      GnuPaint (pgw,  4, 2, 3, 0, "The path could not be created:");
      GnuPaint (pgw,  6, 2, 3, 0, "This drive cannot be found");
      }
   KeyChoose ("\x1B\x0D", "");
   GnuDestroyWin (pgw);
   return FALSE;
   }



/*
 * Checks to see if the path exists
 * If not, it asks the user if she wants it created.
 * returns TRUE if the path exists or is created
 */
BOOL CheckCreatePath (PSZ pszPath)
   {
   PGW  pgw;
   int  c;
   char szCurrPath [128];

   if (pszPath && *pszPath && (pszPath [strlen (pszPath) - 1] == '\\'))
      pszPath [strlen (pszPath) - 1] = '\0';

   getcwd (szCurrPath, sizeof szCurrPath);

   if (!chdir (pszPath))
      {
      chdir (szCurrPath);
      return TRUE;            // path exists - all is good
      }

   if (!DriveExists (pszPath))
      return ErrorWin (1, pszPath);    // drive does not exist - all is bad

   if (!bMAKEALLDIRS)
      {
      pgw = GnuCreateWin (12, 65, NULL);
      GnuPaint (pgw,  1, 0, 3, 1, "CREATE PATH");
      GnuPaint (pgw,  2, 0, 3, 1, "ﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂ");
      GnuPaint (pgw,  4, 2, 3, 0, "The path:");
      GnuPaint (pgw,  5, 2, 3, 1, pszPath);
      GnuPaint (pgw,  6, 2, 3, 0, "Does not exist");
      GnuPaint (pgw,  8, 2, 3, 1, "Would you like it created ? ([Y]es [N]o [A]ll)");
      
      c = KeyChoose ("YNA", "");
      GnuDestroyWin (pgw);

      if (c == 'N')
         return FALSE;           // not allowed to create path - all is bad

      bMAKEALLDIRS = (c == 'A');
      }

   if (FilMakePath (pszPath))
      {
      chdir (szCurrPath);
      return TRUE;            // path created - all is good
      }
   return ErrorWin (0, pszPath);  // cannot create new path - all is bad
   }



//   {
//   PSZ psz;
//
//   strcpy (ppszPath[P_SERVER   ], "c:\\bams");
//   strcpy (ppszPath[P_OUTFILES ], "c:\\bams\\files");
//   strcpy (ppszPath[P_JOBS     ], "c:\\bams\\jobs");
//   strcpy (ppszPath[P_TEMPLATES], "c:\\bams\\jobs\\template");
//   strcpy (ppszPath[P_IO       ], "c:\\bams\\io");
//   strcpy (ppszPath[P_TMP      ], "c:\\tmp");
//
//   if (psz = getenv ("TEMP"))
//      strcpy (ppszPath[P_TMP], psz);
//   if (psz = getenv ("TMP"))
//      strcpy (ppszPath[P_TMP], psz);
//
//   return TRUE;
//   }


/*
 * sets default paths for the path variable
 * uses pszBase as the base path, and other path
 * are relative to it.
 */
void DefaultPaths (PSZ pszBase)
   {
   PSZ psz;

   psz = ((pszBase && *pszBase) ? pszBase : "c:\\bams");
   if (psz[strlen (psz)-1] == '\\')
      psz[strlen (psz)-1] = '\x0';

           strcpy (ppszPath[P_SERVER   ], psz);
   strcat (strcpy (ppszPath[P_OUTFILES ], psz), "\\files");
   strcat (strcpy (ppszPath[P_JOBS     ], psz), "\\jobs");
   strcat (strcpy (ppszPath[P_TEMPLATES], psz), "\\jobs");
   strcat (strcpy (ppszPath[P_CONFIGS  ], psz), "\\jobs");
   strcat (strcpy (ppszPath[P_IO       ], psz), "\\io");

   strcpy (ppszPath[P_TMP], "c:\\tmp");
   if (psz = getenv ("TEMP"))
      strcpy (ppszPath[P_TMP], psz);
   if (psz = getenv ("TMP"))
      strcpy (ppszPath[P_TMP], psz);
   }



/*
 *
 *
 */
BOOL GetPaths (void)
   {
   USHORT c, i, uField, uRet, uLen;
   PGW    pgw;
   BOOL   bOK, bFirst;

   pgw = GnuCreateWin (19, 75, NULL);
   GnuPaintBorder (pgw);

   GnuPaint (pgw,  1, 0,  3, 1, "INSTALLATION PATH INFORMATION");
   GnuPaint (pgw,  2, 0,  3, 1, "ﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂ");
   GnuPaint (pgw, 16, 0,  3, 1, "<Tab>,Arrows=Move, <Enter>=Done, <Esc>=Abort");

   GnuPaintBig (pgw, 3, 2, 3, 70, 0, 0,
      "Please enter the path information below.  See installation notes "
      "for information regarding these settings.");

   GnuPaintBig (pgw, 6, 0, ENTRIES, 17, 1, 0,
      "Batch Server:\nOutput Files:\n"
      "Batch Jobs:\nBatch Templates:\n"
      "Batch Configs:\nI/O Dir:\nTemp Dir:\n");

   KeyEditCellMode ("\t\x0D", "\x0F\x50\x48", FALSE);

   for (i=0; i<ENTRIES; i++)
      GnuPaint (pgw, i+6, 18, 0, 1, ppszPath[i]);

   bFirst = TRUE;
   for (c = 0; c != 'Y' && c != 'y';)  
      {
      ScrShowCursor (TRUE);
      for (uRet=uField=0; uRet != '\x0D'; )
         {
         GnuPaintNChar (pgw, uField+6, 18, 0, (15<<8)|15, ' ', 1);

         uRet = KeyEditCell (ppszPath[uField], TopOf(pgw)+uField+7, LeftOf(pgw)+19, FIELDSIZE, 1);
         GnuPaint (pgw, uField+6, 18, 0, 1, ppszPath[uField]);
         uLen = strlen (ppszPath[uField]);
         GnuPaintNChar (pgw, uField+6, 18 + uLen, 0, 1, ' ', FIELDSIZE-uLen);

         /*--- default other entries 1st time through ---*/
         if (!uField && bFirst) 
            {
            bFirst = FALSE;
            DefaultPaths (ppszPath[P_SERVER]);
            for (i=0; i<ENTRIES; i++)
               {
               uLen = GnuPaint (pgw, i+6, 18, 0, 1, ppszPath[i]);
               GnuPaintNChar (pgw, uField+6+i, 18 + uLen, 0, 1, ' ', FIELDSIZE-uLen);
               }
            }

         switch (uRet)
            {
            case 0x150: /*--- Dn Arrow ---*/
            case '\t':  /*--- tab      ---*/
               uField = (uField + 1) % ENTRIES;
               break;

            case 0x148: /*--- Up Arrow ---*/
            case 0x10F:
               uField = (uField ? uField-1 : ENTRIES-1);
               break;

            case 0:
               GnuDestroyWin (pgw);
               return FALSE;
            }
         }
      ScrShowCursor (FALSE);

      for (bOK=TRUE,i=0; i<ENTRIES; i++)
         if (StrBlankLine (ppszPath[i]))
            bOK=FALSE;

      if (bOK)
         {
         GnuPaint (pgw, 14, 2, 3, 1, "Is everything correct ? <Y or N>");
         c = KeyChoose ("YyNn", "");
         GnuPaintNChar (pgw,  14, 2,  3, 1, ' ', 35);
         bOK = (c=='y' || c=='Y');
         }

      for (i=0; bOK && i<ENTRIES; i++)
         if (!CheckCreatePath (ppszPath[i]))
            bOK=FALSE;

      if (!bOK)
         {
         Beep (0);
         c = 'N';
         }
      }
   KeyEditCellMode (NULL, NULL, 2);
   GnuDestroyWin (pgw);
   return TRUE;
   }


/*
 *
 *
 */
BOOL InstallResources (void)
   {
   PSZ    psz;
   PSZ    psz2;
   char   szName [256];
   USHORT uSize;
   FILE   *fpOut;

   /*--- xfer the bamsserv.cfg file ---*/
   if (!(psz = ResLoadData (pszRES, "BAMSSERV.CFS", NULL, 0, &uSize)))
      ErrWin ("Cannot find resource", "BAMSSERV.CFG");
   psz2 = malloc (uSize + 1024);

   sprintf (psz2, psz, VERSTR, ppszPath[P_JOBS], 
                               ppszPath[P_OUTFILES], 
                               ppszPath[P_SERVER],
                               ppszPath[P_TMP],
                               ppszPath[P_SERVER],
                               "", "", "", "" );

   sprintf (szName, "%s\\BAMSSERV.CFG", ppszPath[P_SERVER]);

   if (!(fpOut = fopen (szName, "wt")))
      ErrWin ("Cannot open resource file", szName);

   fputs (psz2, fpOut);
   fclose (fpOut);
   free (psz);
   free (psz2);


   /*--- xfer the jobexec.cfg file ---*/
   if (!(psz = ResLoadData (pszRES, "Jobexec.cfs", NULL, 0, &uSize)))
      ErrWin ("Cannot find resource", "JOBEXEC.CFG");
   psz2 = malloc (uSize + 1024);

   sprintf (psz2, psz, VERSTR,
                       ppszPath[P_JOBS], 
                       ppszPath[P_TEMPLATES], 
                       ppszPath[P_CONFIGS], 
                       ppszPath[P_IO], "", "", "", "", "");
   sprintf (szName, "%s\\JOBEXEC.CFG", ppszPath[P_JOBS]);

   if (!(fpOut = fopen (szName, "wt")))
      ErrWin ("Cannot open resource file", szName);

   fputs (psz2, fpOut);
   fclose (fpOut);
   free (psz);
   free (psz2);

   return TRUE;
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
BOOL AskInsertDisk (PSZ pszPath, USHORT uIndex)
   {
   char szText[128];

   if (!pszPath || !*pszPath)
      sprintf (szText, "    Please insert disk #%d     ", uIndex, pszPath);
   else
      sprintf (szText, " Please insert disk #%d in %s ", uIndex, pszPath);

   return MsgWin ("Change Disk", szText, "", "<Enter>=Continue, <Esc>=Abort");
   }


/*
 * install a disk of ebl files
 *
 */
BOOL InstallDisk (FILE *fpIn, PSZ pszSrcPath, PBOOL pbLast)
   {
   char   szLine     [256];
   char   szDestPath [256];
   PSZ    *ppsz, psz;
   USHORT uCols;

   *pbLast = FALSE;
   while (FilReadLine (fpIn, szLine, ";", sizeof (szLine)) != 0xFFFF)
      {
      if (StrBlankLine (szLine))
         continue;

      if (psz = strchr (szLine, ';'))
         *psz = '\0';

      ppsz = StrMakePPSZ (szLine, ",\n", TRUE, TRUE, &uCols);
      if (!stricmp (ppsz[0], "end"))
         {
         *pbLast = TRUE;
         MemFreePPSZ (ppsz, uCols);
         return TRUE;
         }

      if (uCols < 2)
         ErrWin ("Bad line in fileNN.dat", szLine);

      VarResolve (szDestPath, ppsz[1], pvCFG);
      if (strchr (szDestPath, '@'))
         ErrWin ("Unable to resolve", szDestPath);

      /*--- We may need to create the dest path ---*/
      if (!FilMakePath (szDestPath))
         ErrWin ("Cannot make path", szDestPath);

      /*--- lib name could be 'null' which means just do the path ---*/
      if (stricmp (ppsz[0], "null"))
         {
         sprintf (szLib, "%s%s", pszSrcPath, ppsz[0]);
         if (!ExtractLib (szLib, szDestPath))
            return FALSE;
         }
      MemFreePPSZ (ppsz, uCols);
      }
   return TRUE;
   }


/*
 *
 *
 */
BOOL InstallPrograms (PSZ pszArg0)
   {
   char   szSrcPath [256];
   char   szFile [256];
   FILE   *fpIn;
   PSZ    psz;
   USHORT uIndex;
   BOOL   bLast = FALSE;

   /*--- get install source path---*/
   strcpy (szSrcPath, pszArg0);
   if (psz = strrchr (szSrcPath, '\\'))       //
      psz[1] = '\0';                          // clip after path spec
   else if (psz = strrchr (szSrcPath, ':'))   //
      psz[1] = '\0';                          // clip after drive spec
   else                                       //
      *szSrcPath = '\0';                      // no path

   for (uIndex=uSTART; !bLast; uIndex++)
      {
      sprintf (szFile, "%sFILES%2.2d.DAT", szSrcPath, uIndex);

      fpIn = NULL;
      while (!fpIn)
         {
         DosError (HARDERROR_DISABLE);
         fpIn = fopen (szFile, "rt");
         DosError (HARDERROR_ENABLE);
         if (!fpIn && !AskInsertDisk (szSrcPath, uIndex))
            break;
         }

      if (!fpIn)
         return FALSE;

      if (!InstallDisk (fpIn, szSrcPath, &bLast))
         return FALSE;

      fclose (fpIn);
      fpIn = NULL;
      }
   return TRUE;
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
BOOL PathExists (PSZ pszLine, PSZ pszElement)
   {
   char   szTmp1 [512];
   char   szTmp2 [256];
   USHORT i;

   for (i=0; pszLine[i]; i++)
      szTmp1[i] = (char)toupper(pszLine[i]);
   szTmp1[i] = '\0';

   for (i=0; pszElement[i]; i++)
      szTmp2[i] = (char)toupper(pszElement[i]);
   szTmp2[i] = '\0';

   if (bDEBUG)
      GnuMsgBox ( (!!strstr (szTmp1, szTmp2) ? "TRUE" : "FALSE"),
               szTmp2, "\x0D\x1B", szTmp1);

   return !!strstr (szTmp1, szTmp2);
   }

 
/*
 * Modifies config.sys on the boot drive
 *
 * adds @JobDir\DLL; to LIBPATH (resolved)
 * adds @JobDir\EXE; to PATH    (resolved)
 */
BOOL ModifyConfig (void)
   {
   GINFOSEG *pgseg;
   SEL      selG, selL;
   USHORT   cBootDrive;
   char     szLine   [512];
   char     szJobDir [256];
   char     szNewLib [256];
   char     szNewExe [256];
   char     szOldCfg [256];
   char     szNewCfg [256];
   char     szBakCfg [256];
   BOOL     bLibPathFound, bPathFound, bDPathFound;
   FILE     *fpIn, *fpOut;
   PSZ      psz;

   /*--- Get Boot Drive ---*/
   DosGetInfoSeg (&selG, &selL);
   pgseg = MAKEP (selG, 0);
   cBootDrive = pgseg->bootdrive + 'A' - 1;

   sprintf (szOldCfg, "%c:\\Config.sys", cBootDrive);
   sprintf (szNewCfg, "%c:\\Config.@@@", cBootDrive);
   sprintf (szBakCfg, "%c:\\Config.IT0", cBootDrive);

   if (!(fpIn = fopen (szOldCfg, "rt")))
      ErrWin ("Cannot open config file", szOldCfg);
   if (!(fpOut = fopen (szNewCfg, "wt")))
      ErrWin ("Cannot open temp file", szNewCfg);

   if (!VarGet (pvCFG, "JobDir", szJobDir))
      strcpy (szJobDir, ".");
   sprintf (szNewLib, "%s\\dll;", szJobDir);
   sprintf (szNewExe, "%s\\exe;", szJobDir);

   bLibPathFound = bPathFound = bDPathFound = FALSE;

   while (FilReadLine (fpIn, szLine, "", sizeof (szLine)) != 0xFFFF)
      {
      psz = StrSkipBy (szLine, " \t");
      if (psz && !strnicmp (psz, "set ", 4))
         psz += 4;

      if (psz && !strnicmp (psz, "LIBPATH", 7))
         {
         psz+=7;
         bLibPathFound = TRUE;
         if (StrEatChar (&psz, "=", " \t") && !PathExists (psz, szNewLib))
            {
            fprintf (fpOut, "LIBPATH=%s%s\n", szNewLib, StrSkipBy (psz, " \t"));
            continue;
            }
         }
      else if (psz && !strnicmp (psz, "PATH", 4))
         {
         psz+=4;
         bPathFound = TRUE;
         if (StrEatChar (&psz, "=", " \t") && !PathExists (psz, szNewExe))
            {
            fprintf (fpOut, "SET PATH=%s%s\n", szNewExe, StrSkipBy (psz, " \t"));
            continue;
            }
         }
      else if (psz && !strnicmp (psz, "DPATH", 5))
         {
         psz+=5;
         bDPathFound = TRUE;
         if (StrEatChar (&psz, "=", " \t") && !PathExists (psz, szNewExe))
            {
            fprintf (fpOut, "SET DPATH=%s%s\n", szNewExe, StrSkipBy (psz, " \t"));
            continue;
            }
         }
      fprintf (fpOut, "%s\n", szLine);
      }
   fclose (fpIn);
   fclose (fpOut);

   if (!bLibPathFound)
      ErrWin ("Error modifying config.sys", "LIBPATH not found");
   if (!bPathFound)
      ErrWin ("Error modifying config.sys", "PATH not found");
   if (!bDPathFound)
      ErrWin ("Error modifying config.sys", "DPATH not found");

   unlink (szBakCfg);
   if (rename (szOldCfg, szBakCfg))
      ErrWin ("Unable to replace config.sys", "");
   else if (rename (szNewCfg, szOldCfg))
      ErrWin ("Unable to replace config.sys", "");

   MsgWin ("",  "CONFIG.SYS has been updated",
           "A backup has been copied to CONFIG.IT0", 
           "Press <Return> to continue");
   return TRUE;
   }

/**************************************************************************/
/*                                                                        */
/*                                                                        */
/*                                                                        */
/**************************************************************************/

void SetVars (void)
   {
   VarSet (&pvCFG, "ServerDir",   ppszPath[P_SERVER   ]); 
   VarSet (&pvCFG, "FilesDir",    ppszPath[P_OUTFILES ]); 
   VarSet (&pvCFG, "FileDir",     ppszPath[P_OUTFILES ]); 
   VarSet (&pvCFG, "JobDir",      ppszPath[P_JOBS     ]); 
   VarSet (&pvCFG, "JobsDir",     ppszPath[P_JOBS     ]); 
   VarSet (&pvCFG, "TemplateDir", ppszPath[P_TEMPLATES]); 
   VarSet (&pvCFG, "TemplatesDir",ppszPath[P_TEMPLATES]); 
   VarSet (&pvCFG, "ConfigDir",   ppszPath[P_CONFIGS  ]); 
   VarSet (&pvCFG, "ConfigsDir",  ppszPath[P_CONFIGS  ]); 
   VarSet (&pvCFG, "IODir",       ppszPath[P_IO       ]); 
   VarSet (&pvCFG, "I/ODir",      ppszPath[P_IO       ]); 
   VarSet (&pvCFG, "TempDir",     ppszPath[P_TMP      ]); 
   VarSet (&pvCFG, "TmpDir",      ppszPath[P_TMP      ]); 
   }



/*
 *
 *
 */
int _cdecl main (int argc, char *argv[])
   {
   PSZ    psz;
   USHORT uLines;


   ArgBuildBlk ("? *^Help *^Lines% *^IO *^Debug *^Res% *^Start%");

   if (ArgFillBlk (argv))
      {
      fprintf (stderr, "%s\n", ArgGetErr ());
      Usage ();
      }

   uSTART = (ArgIs ("Start") ? atoi (ArgGet("Start", 0)) : 1);

   if (ArgIs ("help") || ArgIs ("?"))
      Usage ();

   pszRES = ArgGet ("Res", 0);  // normally null

   bDEBUG = ArgIs ("Debug");

   InitEbLib (szY);
// Cmp2Init (szY, 3, 1);
   ScrInitMetrics ();
   ScrShowCursor (FALSE);
   GnuSetBorderChars (NULL, "€€€€ﬂ‹›ﬁ ");

   if (ArgIs ("io"))
      ScrAttachIO (FALSE);

   if (ArgIs ("Lines"))
      {
      psz = ArgGet ("lines", 0);
      if ((uLines = atoi (psz)) && uLines < 61)
         ScrSetMode (uLines, 80);
      }

   /*--- cls ---*/
//   GnuClearWin (NULL, '\xB0', 0x0A00, FALSE);
   GnuClearWin (NULL, '\xB0', 0x1900, FALSE);
   sprintf (sz, " BAMS JOB SERVER INSTALLATION %s ", VERSTR);
   GnuPaint (NULL, 0, 0, 3, 0x1900, sz);

   if (!Intro ())
      Quit (FALSE);

   DefaultPaths (NULL);

   if (!GetPaths ())
      Quit (FALSE);

   SetVars ();

   if (!InstallResources ())
      Quit (FALSE);

   if (!InstallPrograms (argv[0]))
      Quit (FALSE);

   if (!ModifyConfig ())
      Quit (FALSE);

   Extro ();
   Quit (TRUE);
   return 0;
   }

