/*
 *
 * userio.c
 * Monday, 1/9/1995.
 *
 * (C) 1995 Info Tech Inc.
 * Craig Fitzgerald
 *
 * Part of the BAMS Job Server
 * This module handles screen painting and server user input
 */

#define INCL_DOSSEMAPHORES
#define INCL_DOS
#define INCL_VIO
#include <os2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <process.h>
#include <GnuScr.h>
#include <GnuMisc.h>
#include <GnuKbd.h>
#include <time.h>
#include <conio.h>
#include "Var.h"
#include "Bamsserv.h"
#include "JobQ.h"
#include "UserIO.h"
#include "Util.h"
#include "Service.h"

USHORT uLOG_WINDOW_LEVEL;
USHORT uLOG_FILE_LEVEL  ;

PGW pgwQUEUE  ;
PGW pgwRUNNING;
PGW pgwCOMMAND;
PGW pgwLOG  ;


ULONG semQueueWindowEvent   = 0; // event sem - data in this win has changed
ULONG semRunningWindowEvent = 0; // event sem - data in this win has changed
ULONG semCommandWindowEvent = 0; // event sem - data in this win has changed
ULONG semLogWindowEvent     = 0; // event sem - data in this win has changed

//ULONG semQueueWindowMux   = 0; // no need - maintains own data - w/1 thread
//ULONG semRunningWindowMux = 0; // no need - maintains own data - w/1 thread

ULONG semCommandWindowMux = 0; // event sem - data in this win has changed
ULONG semLogWindowMux     = 0; // event sem - data in this win has changed


/***********************************************************************/
/*                                                                     */
/*                                                                     */
/*                                                                     */
/***********************************************************************/

/*
 * Clears the screen 
 * 
 *
 */
static void PaintBkg (void)
   {
   char   szTmp[80];
   PMET   pmet;

   pmet = ScrGetMetrics ();
   GnuClearWin (NULL, ' ', 0x0A00, FALSE);
   sprintf (szTmp, "BAMS Job Server  %s", VERSTR);
   GnuPaint (NULL, 0, 0, 3, 0x0A00, szTmp);
   GnuPaint (NULL, pmet->uYSize-1, 0, 3, 0x0A00, "<Alt-X>=Shutdown  <Alt-R>=Refresh  <Alt-C>=Clear Log  <Alt-K>=Kill Running Jobs");
   }


/***********************************************************************/
/*                                                                     */
/* Interface fns to handle local user                                  */
/*                                                                     */
/***********************************************************************/


/*
 * This fn refreshes the screen.
 * currently i completely ignore the screen semaphores
 * this will need to be changed if/when we allow popups
 */
static void RefreshScreen (void)
   {
   PaintBkg ();

//   UpdateWindow (JM_QUEUE_WINDOW,   NULL); 
//   UpdateWindow (JM_RUNNING_WINDOW, NULL); 
//   UpdateWindow (JM_COMMAND_WINDOW, NULL); 
//   UpdateWindow (JM_ERROR_WINDOW,   NULL); 

   GnuPaintWin (pgwQUEUE  , 0xFFFF);
   GnuPaintWin (pgwRUNNING, 0xFFFF);
   GnuPaintWin (pgwCOMMAND, 0xFFFF);
   GnuPaintWin (pgwLOG  , 0xFFFF);
   GnuPaintBorder (pgwQUEUE  );
   GnuPaintBorder (pgwRUNNING);
   GnuPaintBorder (pgwCOMMAND);
   GnuPaintBorder (pgwLOG  );
   }


void ClearLog (void)
   {
   USHORT i;

   DosSemRequest (&semLogWindowMux, SEM_INDEFINITE_WAIT);
   for (i=0; i<pgwLOG->uItemCount; i++)
      if (((PSZ *)pgwLOG->pUser1)[i])
         free (((PSZ *)pgwLOG->pUser1)[i]);
   pgwLOG->uItemCount = 0;
   DosSemClear (&semLogWindowMux);
   DosSemClear (&semLogWindowEvent);
   }


/*
 * This handles keyboard input
 *
 */
VOID HandleUserIOFn (PVOID p)
   {
   while (TRUE)
      {

      /*
       * Normally this while loop is not needed
       * But if the thread is waiting on a KeyGet(), the 
       * thread cannot be terminated until a key is pressed
       */
      while (TRUE)
         {
         DosSleep (250);
         if (kbhit ())
            break;
         }

      switch (KeyGet (TRUE))
         {
         case 0x12d:  TriggerShutdown ();      return;  // <Alt-X> exit
         case 0x113:  RefreshScreen ();        break;   // <Alt-R> Refresh
         case 0x12E:  ClearLog ();             break;   // <Alt-C> Clear win
         case 0x125:  KillRunningJobs (FALSE); break;   // <Alt-K> Kill jobs
         }
      JMLog (10, "Thread 2 going ...");
      }
   }


/***********************************************************************/
/*                                                                     */
/* Internal Screen update handlers and painting functions              */
/*                                                                     */
/***********************************************************************/


/*
 * This is the paint fn for the queue window
 *
 *
 */
USHORT _cdecl pgwPaintQueueFn (PGW pgw, USHORT uIndex, USHORT uLine)
   {
   PJOB   pj;
   USHORT uAtt, uLen, i;
   char   szTmp[16];

   pj = JQAccess (TRUE);
   for (i=0; i<uIndex && pj; i++)
      pj = pj->next;

   uAtt = (pgw->uSelection == uIndex ? 2 : 0);
   
   if (!pj)
      {
      JQAccess (FALSE);
      return 0;
      }
   TimeString (szTmp, pj->tmQueueTime);
   GnuPaint2 (pgw, uLine, 0,  0, uAtt, szTmp,         9 );
   GnuPaint2 (pgw, uLine, 9,  0, uAtt, pj->pszUserID, 8 );
   GnuPaint2 (pgw, uLine, 18, 0, uAtt, pj->pszJobID,  8 );
   uLen = 27 + GnuPaint2 (pgw, uLine, 27, 0, uAtt, pj->pszJobTag, 11);
   if (uLen < pgw->uClientXSize)
      GnuPaintNChar (pgw, uLine, uLen, 0, uAtt, ' ', pgw->uClientXSize - uLen);
   JQAccess (FALSE);
   return 1;
   }



/*
 * This is the paint fn for the running jobs window
 *
 *
 */
USHORT _cdecl pgwPaintRunningFn (PGW pgw, USHORT uIndex, USHORT uLine)
   {
   PJOB   pj;
   USHORT uAtt, uLen, i;
   char   szTmp[16];

   pj = JLAccess (TRUE);
   for (i=0; i<uIndex && pj; i++)
      pj = pj->next;

   uAtt = (pgw->uSelection == uIndex ? 2 : 0);
   
   if (!pj)
      {
      JLAccess (FALSE);
      return 0;
      }

   TimeString (szTmp, pj->tmStartTime);
   GnuPaint2 (pgw, uLine, 0,  0, uAtt, szTmp,         9 );
   GnuPaint2 (pgw, uLine, 9,  0, uAtt, pj->pszUserID, 8 );
   GnuPaint2 (pgw, uLine, 18, 0, uAtt, pj->pszJobID,  8 );
   uLen = 27 + GnuPaint2 (pgw, uLine, 27, 0, uAtt, pj->pszJobTag, 11);
   if (uLen < pgw->uClientXSize)
      GnuPaintNChar (pgw, uLine, uLen, 0, uAtt, ' ', pgw->uClientXSize - uLen);

   JLAccess (FALSE);
   return 1;
   }


/*
 * This is where the paint Q thread resides waiting for the
 * semQueueWindowEvent semaphore and painting its window
 *
 */
VOID QueueWindowHandlerFn (PVOID p)
   {
   ScrShowCursor (FALSE);
   while (TRUE)
      {
      DosSemWait (&semQueueWindowEvent, SEM_INDEFINITE_WAIT);
      DosSemSet  (&semQueueWindowEvent);
      pgwQUEUE->uItemCount = JQSize ();
      GnuPaintWin (pgwQUEUE, 0xFFFF);

      JMLog (10, "Thread 3 going ...");
      }
   }



/*
 * This is where the paint running job window thread resides waiting for the
 * semQueueWindowEvent semaphore and painting its window
 *
 */
VOID RunningWindowHandlerFn(PVOID p)
   {
   ScrShowCursor (FALSE);
   while (TRUE)
      {
      DosSemWait (&semRunningWindowEvent, SEM_INDEFINITE_WAIT);
      DosSemSet (&semRunningWindowEvent);
      pgwRUNNING->uItemCount = JLSize ();
      GnuPaintWin (pgwRUNNING, 0xFFFF);

      JMLog (10, "Thread 4 going ...");
      }
   }


/*
 * This is where the paint command window thread resides waiting for the
 * semQueueWindowEvent semaphore and painting its window
 *
 */
VOID CommandWindowHandlerFn(PVOID p)
   {
   ScrShowCursor (FALSE);
   while (TRUE)
      {
      DosSemWait (&semCommandWindowEvent, SEM_INDEFINITE_WAIT);
      DosSemSet  (&semCommandWindowEvent);
      DosSemRequest (&semCommandWindowMux, SEM_INDEFINITE_WAIT);
      GnuPaintWin (pgwCOMMAND, 0xFFFF);
      DosSemClear (&semCommandWindowMux);

      JMLog (10, "Thread 5 going ...");
      }
   }


/*
 * This is where the paint log thread resides waiting for the
 * semQueueWindowEvent semaphore and painting its window
 *
 */
VOID LogWindowHandlerFn(PVOID p)
   {
   ScrShowCursor (FALSE);
   while (TRUE)
      {
      DosSemWait  (&semLogWindowEvent, SEM_INDEFINITE_WAIT);
      DosSemSet   (&semLogWindowEvent);
      DosSemRequest (&semLogWindowMux, SEM_INDEFINITE_WAIT);
      GnuPaintWin (pgwLOG, 0xFFFF);
      DosSemClear (&semLogWindowMux);

//      JMLog (10, "Thread 6 going ...");
      }
   }



/*
 * This fn maintains the clock in the upper right hand
 * of the display
 *
 */
VOID TimeFn (PVOID p)
   {
   char   szBuff [32];
   BOOL   bColon = FALSE;
   time_t tm;

   SetPriority ("PClass", "PDelta", 1, TRUE);

   ScrShowCursor (FALSE);
   while (TRUE)
      {
      time (&tm);

      TimeString (szBuff, tm);
      if (bColon = !bColon)
         szBuff [2] = ' ';
      GnuPaint (NULL, 0, 72, 0, 0x0A00, szBuff);
      DosSleep (1000);
      }
   }

/***********************************************************************/
/*                                                                     */
/* External functions called to modify screen data                     */
/*                                                                     */
/***********************************************************************/

/*
 * This fn performs the Fast Fourrier Transform of the lower 5 bits
 * if the frequency data maintained by the double indirected ppsz ptrs
 * Actually this fn just makes room at the beginning of the ppsz array
 *
 */
void ShiftList (PSZ *ppsz, USHORT uSize)
   {
   USHORT i;

   if (ppsz[0]) free (ppsz[0]);
   for (i=0; i<uSize; i++)
      ppsz[i] = ppsz[i+1];
   }


/*
 * Write a string onto window uWindow
 * 
 *  JM_COMMAND_WINDOW  1
 *  JM_QUEUE_WINDOW    2
 *  JM_RUNNING_WINDOW  3
 *  JM_ERROR_WINDOW    4
 *
 */
void UpdateWindow (USHORT uWindow, PSZ pszLine)
   {
   switch (uWindow)
      {
      case JM_QUEUE_WINDOW  :
         DosSemClear (&semQueueWindowEvent);
         break;
      
      case JM_RUNNING_WINDOW:
         DosSemClear (&semRunningWindowEvent);
         break;
      
      case JM_COMMAND_WINDOW:
         DosSemRequest (&semCommandWindowMux, SEM_INDEFINITE_WAIT);
         if (pgwCOMMAND->uItemCount < COMMANDWINDOWSIZE)
            {
            ((PSZ *)pgwCOMMAND->pUser1)[pgwCOMMAND->uItemCount] = strdup (pszLine);
            pgwCOMMAND->uItemCount++;
            }
         else
            {
            ShiftList (pgwCOMMAND->pUser1, COMMANDWINDOWSIZE);
            ((PSZ *)pgwCOMMAND->pUser1)[COMMANDWINDOWSIZE - 1] = strdup (pszLine);
            }

         if (pgwCOMMAND->uItemCount > pgwCOMMAND->uClientYSize)
            pgwCOMMAND->uScrollPos = pgwCOMMAND->uItemCount - pgwCOMMAND->uClientYSize;
         else
            pgwCOMMAND->uScrollPos = 0;
         DosSemClear (&semCommandWindowMux);
         DosSemClear (&semCommandWindowEvent);
         break;
      
      case JM_ERROR_WINDOW  :
         DosSemRequest (&semLogWindowMux, SEM_INDEFINITE_WAIT);
         if (pgwLOG->uItemCount < ERRORWINDOWSIZE)
            {
            ((PSZ *)pgwLOG->pUser1)[pgwLOG->uItemCount] = strdup (pszLine);
            pgwLOG->uItemCount++;
            }
         else
            {
            ShiftList (pgwLOG->pUser1, ERRORWINDOWSIZE);
            ((PSZ *)pgwLOG->pUser1)[ERRORWINDOWSIZE - 1] = strdup (pszLine);
            }

         if (pgwLOG->uItemCount > pgwLOG->uClientYSize)
            pgwLOG->uScrollPos = pgwLOG->uItemCount - pgwLOG->uClientYSize;
         else
            pgwLOG->uScrollPos = 0;

         DosSemClear (&semLogWindowMux);
         DosSemClear (&semLogWindowEvent);
         break;
      }
   }


void ScreenDump (FILE *fp)
   {
   PSZ    *ppsz;
   USHORT i;
   char   szBuff [256];
   PJOB   pj;

   /*--- dump queue ---*/
   Identify (fp, "QUEUED JOBS");
   for (pj=JQAccess (TRUE); pj; pj = pj->next)
      {
      fprintf (fp, "%s ", TimeString (szBuff, pj->tmQueueTime));
      fprintf (fp, "%-20s %-20s %s\n", pj->pszUserID, pj->pszJobID, pj->pszJobTag);
      }
   JQAccess (FALSE);

   /*--- dump running list ---*/
   Identify (fp, "RUNNING JOBS");
   for (pj=JLAccess (TRUE); pj; pj = pj->next)
      {
      fprintf (fp, "%s ", TimeString (szBuff, pj->tmStartTime));
      fprintf (fp, "%-20s %-20s %s\n", pj->pszUserID, pj->pszJobID, pj->pszJobTag);
      }
   JLAccess (FALSE);

   /*--- dump commands ---*/
   Identify (fp, "COMMANDS");
   DosSemRequest (&semCommandWindowMux, SEM_INDEFINITE_WAIT);
   ppsz = (PSZ *)pgwCOMMAND->pUser1;
   for (i=0; i< pgwCOMMAND->uItemCount; i++)
      fprintf (fp, "%s\n", ppsz[i]);
   DosSemClear (&semCommandWindowMux);

   /*--- dump messages ---*/
   Identify (fp, "MESSAGE LOG");
   DosSemRequest (&semLogWindowMux, SEM_INDEFINITE_WAIT);
   ppsz = (PSZ *)pgwLOG->pUser1;
   for (i=0; i< pgwLOG->uItemCount; i++)
      fprintf (fp, "%s\n", ppsz[i]);
   DosSemClear (&semLogWindowMux);
   }


/***********************************************************************/
/*                                                                     */
/* Startup Functions                                                   */
/*                                                                     */
/***********************************************************************/


/* 
 * This fn prepares a new window for prime time
 *
 *
 */
static void SetupWindow (PGW pgw, PSZ pszHeader)
   {
   gnuFreeDat (pgw);
   pgw->pszHeader = strdup (pszHeader);
   pgw->bShadow = FALSE;
   GnuClearWin (pgw, ' ', 0, TRUE);
   GnuPaintBorder (pgw);
   }


void CloseUserIO (void)
   {
   PMET   pmet;

   pmet = ScrGetMetrics ();
   GnuMoveCursor (NULL, pmet->uYSize-1, 0);
   }



void SetUserIOGlobals (void)
   {
   uLOG_FILE_LEVEL   = (USHORT)VarGetl (pvCFG, "LogFileLevel"  );
   uLOG_WINDOW_LEVEL = (USHORT)VarGetl (pvCFG, "LogWindowLevel");
   }

/*
 * This fn inits the user interface
 * It clears the screen, creates the windows,
 * and creates the display threads
 *
 */
BOOL InitUserIO (void)
   {
   USHORT uY1, uY2, uX;
   USHORT uLines;
   PMET   pmet;
   char   szBuff [128];
   PSZ    p;

   SetUserIOGlobals ();

   /*--- Create User Screen ---*/
   pmet = ScrInitMetrics ();

   uX = uY1 = uY2 = 0;

   if (VarGet (pvCFG, "ScreenLines", szBuff))
      {
      uLines = atoi (szBuff);
      if (uLines >= 20 && uLines <= 100)
         ScrSetMode (uLines, 80);

      if (p = strchr (szBuff, ','))
         uY1 = atoi (++p);
      if (p && (p = strchr (p, ',')))
         uY2 = atoi (++p) + uY1;
      if (p && (p = strchr (p, ',')))
         uX = atoi (++p);
      }

   uY1 = ((uY1 && uY1+4     < pmet->uYSize) ? uY1 : pmet->uYSize / 3);
   uY2 = ((uY2 && uY1+uY2+2 < pmet->uYSize) ? uY2 : uY1 + (pmet->uYSize-uY1)/2);
   uX  = ((uX  && uX       <= pmet->uXSize) ? uX  : pmet->uXSize / 2);

   GnuSetColors (NULL, 1, 15, 15);
   PaintBkg ();
   ScrShowCursor (FALSE);
   GnuMoveCursor (NULL, 0, 0);

   GnuPaintAtCreate (FALSE);

   pgwQUEUE   = GnuCreateWin2 (1,   0, uY1-1,uX,                  (PAINTPROC)pgwPaintQueueFn);
   pgwRUNNING = GnuCreateWin2 (1,   uX,uY1-1,pmet->uXSize - uX,   (PAINTPROC)pgwPaintRunningFn);
   pgwCOMMAND = GnuCreateWin2 (uY1, 0, uY2 - uY1,  pmet->uXSize,  NULL);
   pgwLOG   = GnuCreateWin2 (uY2, 0, pmet->uYSize - uY2 - 1, pmet->uXSize,NULL);

   SetupWindow (pgwQUEUE  , "Job Queue");
   SetupWindow (pgwRUNNING, "Running Jobs");
   SetupWindow (pgwCOMMAND, "Command Log");

   if (uLOG_WINDOW_LEVEL  < 2)
      SetupWindow (pgwLOG  , "Error Log");
   else if (uLOG_WINDOW_LEVEL == 2)
      SetupWindow (pgwLOG  , "Error/Warning Log");
   else if (uLOG_WINDOW_LEVEL  > 2)
      SetupWindow (pgwLOG  , "Error/Warning/Message Log");

   GnuPaintAtCreate (TRUE);

   pgwQUEUE  ->pUser1 = NULL;
   pgwRUNNING->pUser1 = NULL;
   pgwCOMMAND->pUser1 = malloc (COMMANDWINDOWSIZE * sizeof (PSZ));
   pgwLOG  ->pUser1 = malloc (ERRORWINDOWSIZE   * sizeof (PSZ));

   pgwQUEUE  ->pUser2 = NULL;
   pgwRUNNING->pUser2 = NULL;
   pgwCOMMAND->pUser2 = NULL;
   pgwLOG  ->pUser2 = NULL;

   /*--- create the threads that paint each window ---*/
   StartThread (  QueueWindowHandlerFn, 0);
   StartThread (RunningWindowHandlerFn, 0);
   StartThread (CommandWindowHandlerFn, 0);
   StartThread (    LogWindowHandlerFn, 0);
   StartThread (                TimeFn, 0);

   ScrShowCursor (FALSE);
   return TRUE;
   }


