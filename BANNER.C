/*
 *
 * banner.c
 * Monday, 3/27/1995.
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
#include <ctype.h>
#include "Var.h"
#include "Bamsserv.h"
#include "Util.h"


/* 
 * This fn creates a banner file
 * The file is created in the Jobs sys directory
 * The file name is specified by BANNER_FILE
 * If in debug mode, extra junk is printed
 * TRUE is returned if all is good
 */
BOOL CreateBannerFile (PJOB pjob)
   {
   char     szPath   [256];
   char     szFile   [256];
   char     szBanner [256];
   char     szBuff   [256];
   FSINFO   fs;
   FILE     *fp;
   USHORT   uDrive; 

   if (!pjob)
      return FALSE;

   FilesDir2 (szPath, pjob, TRUE);
   if (!VarGet (pvCFG, "BannerFile", szBanner))
      strcpy (szBanner, BANNER_FILE);
   sprintf (szFile, "%s\\%s", szPath, szBanner);

   if (!(fp = my_fopen (szFile, "w")))
      return JMError ("Unable to create banner file: %s", szFile);

   fprintf (fp, "\n\n");
   fprintf (fp, "*****************************************************************************\n");
   fprintf (fp, "*****************************************************************************\n");
   fprintf (fp, "*****************************************************************************\n");
   fprintf (fp, "\n\n");
   fprintf (fp, "                  AMERICAN ASSOCIATION OF STATE HIGHWAY\n");
   fprintf (fp, "                      AND TRANSPORTATION OFFICIALS\n\n");
   fprintf (fp, "                 A proprietary Computer Software Product\n");
   fprintf (fp, "                            AASHTOWARE(tm)\n\n\n\n");
   fprintf (fp, "           *******        ***       ***     ***     *******  tm\n");
   fprintf (fp, "           ********      *****      ****   ****    *********\n");
   fprintf (fp, "            **   ***    *** ***     ***** *****   ***     **\n");
   fprintf (fp, "            **    **   ***   ***    ** ***** **   **\n");
   fprintf (fp, "            **   ***   **     **    **  ***  **   ***\n");
   fprintf (fp, "            *******    **     **    **   *   **    *******  \n");
   fprintf (fp, "            *******    *********    **       **     ******* \n");
   fprintf (fp, "            **   ***   *********    **       **          ***\n");
   fprintf (fp, "            **    **   **     **    **       **           **\n");
   fprintf (fp, "            **    **   **     **    **       **           **\n");
   fprintf (fp, "            **   ***   **     **    **       **   **     ***\n");
   fprintf (fp, "           ********   ****   ****  ****     ****  *********\n");
   fprintf (fp, "           *******    ****   ****  ****     ****   *******\n");
   fprintf (fp, "\n\n\n");
   fprintf (fp, "       AASHTO's Information System for Managing Transportation Programs\n\n");
   fprintf (fp, "                        (C) COPYRIGHT 1991-1995\n\n\n\n");
   fprintf (fp, "      USER NAME: %-15.15s  TIME Queued   : %s\n\n",  pjob->pszUserID,  DateTimeString (szBuff, pjob->tmQueueTime));
   fprintf (fp, "      JOB NAME : %-15.15s  TIME Run      : %s\n\n",  pjob->pszJobTag,  DateTimeString (szBuff, pjob->tmStartTime));
   fprintf (fp, "      REPORT   : %-15.15s  Time Completed: %s\n\n",  pjob->pszJobName, DateTimeString (szBuff, pjob->tmEndTime  ));

//   if (IsFlag (pjob->uFlags, JSF_TIMEDOUT))
//      strcpy (szBuff, "TIMEDOUT");
//   else if (IsFlag (pjob->uFlags, JSF_KILLED))
//      strcpy (szBuff, "KILLED");
//   else if (IsFlag (pjob->uFlags, JSF_ABNORMALEXIT))
//      strcpy (szBuff, "ABORTED");
//   else
//      sprintf (szBuff, "%d", pjob->uRetCode);

   sprintf (szBuff, "%s", ReturnCodeString (pjob->uRetCode));
   fprintf (fp, "      JOB ID   : %-15.15s  Execution Code: %s\n\n\n\n", pjob->pszJobID, szBuff);

   if (bDEBUG)
      {
      uDrive = toupper (szFile[0]) - 'A' + 1;
      DosQFSInfo (uDrive, FSIL_VOLSER, &fs, sizeof (FSINFO));
      fprintf (fp, "      PATH     : [%s] %s\n\n", fs.vol, szPath);
      }
   fprintf (fp, "*****************************************************************************\n");
   fprintf (fp, "*****************************************************************************\n");
   fprintf (fp, "*****************************************************************************\n");
   fprintf (fp, "\n");
   fclose (fp);
   return TRUE;
   }
