/*
 *
 * bamsserv.c
 * Wednesday, 12/21/1994.
 *
 * (C) 1995 Info Tech Inc.
 * Craig Fitzgerald
 * 
 * This is the main file for the BAMS Job Server
 * 
 *
 */

#define INCL_DOS
#include <os2.h>
#include <stdio.h>
#include <stdlib.h>
#include <Process.h>
#include <stddef.h>
#include <string.h>
#include <time.h>
#include <direct.h>
#include <io.h>
#include <GnuArg.h>
#include <GnuCfg.h>
#include <GnuMisc.h>
#include <GnuKbd.h>
#include <GnuFile.h>
#include <GnuStr.h>
#include <GnuRes.h>
#include <Direct.h>
#include "Var.h"
#include "BamsServ.h"
#undef ENAMETOOLONG
#undef ENOTEMPTY
#include "Syssock.h"
#include "osock.h"
#include "Service.h"
#include "JobQ.h"
#include "UserIO.h"
#include "Restart.h"
#include "Util.h"
#include "Stat.h"

#define TIME  __TIME__
#define DATED __DATE__

BOOL bDEBUG    = FALSE;
BOOL bMEMDEBUG = FALSE;
BOOL bLOG      = FALSE;
BOOL bSHUTDOWN = FALSE;

FILE *fpLOGFILE;
FILE *fpMEMFILE;

SOCKET sokWKP = 0;    // this is the Well Known port socket
PVAR   pvCFG  = NULL; // handle to cfg params

ULONG  semShutdownEvent = 0; // event sem - shutdown app


/*
 * Usage info
 *
 */
static void Usage (void)
   {
   printf ("BAMSSERV    BAMS Job Server    release 1.00                   %s\n\n", DATED);
   printf ("\n");
   printf ("USAGE: BAMSSERV [options]\n");
   printf ("\n");
   printf ("WHERE: [options] are 0 or more of:\n");
   printf ("\n");
   printf ("   /Help ............. This help screen.\n");
   printf ("   /Restart........... Reload any unfinished jobs\n");
   printf ("   /MaxProcesses=#.... Set max # of jobs that can run at one time.\n");
   printf ("   /WKP=#............. Set Server connection port.\n");
   printf ("   /Lines=#........... Set screen lines.\n");
   printf ("   /LogFile=file...... Log messages to this file.\n");
   printf ("   /CfgFile=file...... Use this cfg file. (default is BAMSSERV%s)\n", CFG_FILE_EXT);
   printf ("   /JobDir=path....... Set Job Directory path.\n");
   printf ("   /Debug ............ Turn on debugging mode.\n");
   printf ("   /DefaultJob=file... Set default job to be this.\n");
   printf ("   /ExecCmd=cmd....... Set exec command to be this.\n");
   printf ("   /PrintCmd=cmd...... Set print command to be this.\n");
   printf ("   /PClass=#.......... Set Priority Class (0-4).\n");
   printf ("   /PDelta=#.......... Set Priority Delta (0-31).\n");
   printf ("   /Generate.......... Create a sample Bamsserv.cfg file.\n");
   exit (0);
   }


/*
 * This fn will re-create the default cfg file.
 * The fn first checks to make sure the cfg doesn't already exist.
 * This fn also requires the resource to be attached to the exe
 */
void GenerateCfg (PSZ pszExeFile)
   {
   char szBuff[256], szPath[256];
   char sz1[256],    sz2[256];
   PSZ  psz, pszCfg, pszTmp;
   FILE *fp;

   /*--- Load the cfg file name (overrides defaults)---*/
   if (!(pszCfg = ArgGet("CfgFile", 0)))
      {
      if (psz = strrchr (strcpy (szBuff, pszExeFile), '.'))
         *psz = '\0';
      pszCfg = strcat (szBuff, CFG_FILE_EXT);
      }

   /*--- Find the bamsserv path ---*/
   if (!(psz = strrchr (strcpy (szPath, pszExeFile), '\\')))
      {
      /* For some reason, OS/2 will not have path info as part of argv[0]
       * on some machines.  This may be ICBM Cset++ specific
       */
      if (access ("BAMSSERV.EXE", 0))
         Error ("You must be in the same dir as the executable to run this command");
      if (!_getdcwd (0, szPath, sizeof (szPath)))
         strcpy (szPath, ".");
      }
   else
      *psz = '\0';
   
   /*--- Loook for a tmp dir ---*/
   if (!(pszTmp = getenv ("TEMP")))
      if (!(pszTmp = getenv ("TMP")))
         pszTmp = szPath;

   if (!access (pszCfg, 0))
      Error ("Config File %s already exists", pszCfg);

   if (!(psz = ResLoadData2 (NULL, NULL, NULL, 0, NULL)))
      Error ("Cannot find resource '%s'", pszCfg);

   if (!(fp = my_fopen (pszCfg, "w")))
      Error ("Cannot create resource file '%s'", pszCfg);

   fprintf (fp, 
            psz, 
            VERSTR,                             // Version string
            cat (sz1, szPath, "\\Jobs", NULL),  // Job dir
            cat (sz2, szPath, "\\Files", NULL), // Files Dir
            szPath,                             // Log File Dir
            pszTmp,                             // Temp Dir
            szPath, "", "", "");                // root of cinstall ...

   fclose (fp);
   printf ("Configuration file: %s created.\n", pszCfg);
   exit (0);
   }



/* 
 * This fn ensures that certain needed params actually exist either as
 *  a default, a cfg file value or as part of the command line.
 *
 * If the entry was not found and bRequired=TRUE this fn dies with an error
 * If bTestDir=TRUE this fn assumes the param is a dir and tests to see
 *  that the dir actually exists
 *
 */
static void LookForEntry (PSZ pszEntry, BOOL bRequired, BOOL bTestDir)
   {
   PSZ    psz;
   char   szCurrPath[256];
//   USHORT uKey;

   psz = VarGet (pvCFG, pszEntry, NULL);

   if (bRequired && !psz)
      Error ("Entry not found in config file: %s\n", pszEntry);

   if (!bTestDir || !psz)
      return;

   StrClip (psz, "\\ \t");
   getcwd (szCurrPath, sizeof szCurrPath);
   if (!PathExists (psz))
      {
//
// This code asks the user questions
// ---------------------------------
//      printf ("Cfg Variable %s references the path %s\n", pszEntry, psz);
//      printf ("This path does not exist\n");
//      printf ("Do you want to create it? (Y or N)");
//      fflush (stdout); // Aint ICBM C/C++ great?
//      uKey = KeyChoose ("YyNn \x0D\x1B", NULL);
//      printf ("\n");
//      if (uKey == 'N' || uKey == 'n' || uKey == '\x1B')
//         Error ("Path: %s does not exist for entry: %s", psz, pszEntry);
//      if (!FilMakePath (psz))
//         Error ("Unable to create path: %s", psz);

      Error ("Path does not exist: %s", psz);
      }
   }



void SetBamsServGlobals (void)
   {
   bDEBUG  = VarTrue (pvCFG, "Debug");
   bLOG    = VarTrue (pvCFG, "LogFile");
   }


/*
 * This fn sets up the system variables needed by the system
 *
 * Loads default values
 * Loads the server config file
 * Loads the command line variables
 * Tests to make sure all important vars are present
 */
static void LoadVariables (PSZ pszExeFile)
   {
   PSZ  psz, pszCfg;
   char szBuff[256];

   /*--- preset vars that have defaults here ---*/
   VarSet (&pvCFG, "MaxProcesses",  "5"    );
   VarSet (&pvCFG, "MaxQueueSize",  "100"  ); 
   VarSet (&pvCFG, "WellKnownPort", "2000" );
   VarSet (&pvCFG, "Debug",         "false");
   VarSet (&pvCFG, "LogFile",       "false");
   VarSet (&pvCFG, "Restart",       "false");
   VarSet (&pvCFG, "SystemFilesDir" ,""    );
   VarSet (&pvCFG, "OutputFilesDir" ,""    );
   VarSet (&pvCFG, "ExecCmd"        ,""    );
   VarSet (&pvCFG, "PrintCmd"       ,""    );
   VarSet (&pvCFG, "LogFileLevel",  "4"    );
   VarSet (&pvCFG, "LogWindowLevel","3"    );
   VarSet (&pvCFG, "PrintWait",     "TRUE" );
   VarSet (&pvCFG, "PriorityBumpTime","3600" );
   VarSet (&pvCFG, "ServerPerformance","100" );
   VarSet (&pvCFG, "NotifyClient",     "FALSE");
   VarSet (&pvCFG, "JobTimeout",       "0");
   VarSet (&pvCFG, "PrintFileHeaders", "TRUE");
   VarSet (&pvCFG, "FixFormFeeds",     "TRUE");


   /*--- Load the cfg file (overrides defaults)---*/
   if (!(pszCfg = ArgGet("CfgFile", 0)))
      {
      if (psz = strrchr (strcpy (szBuff, pszExeFile), '.'))
         *psz = '\0';
      pszCfg = strcat (szBuff, CFG_FILE_EXT);
      }

   if (!VarReadCfg (&pvCFG, pszCfg, "BAMSSERV STARTUP"))
      {
      if (!strchr (pszExeFile, '\\')) // bad env???
         {
         printf (" Error: Unable to find Configuration file: %s\n", pszCfg);
         printf (" Your environment canot determine the location of the executable.\n");
         printf (" Either this file does not exist, or you are not in the directory \n");
         printf (" containing this program.  If the file does not exist, you can use the \n");
         printf (" /Generate option to create a sample configuration file.  You may then\n");
         printf (" Edit this configuration file to change default settings.\n");
         }
      else
         {
         printf (" Error: Unable to find Configuration file: %s\n", pszCfg);
         printf (" Either this file does not exist, or it is not located in the same\n");
         printf (" directory as the executable file.  If the file is located in a\n");
         printf (" different location, you can use the /Cfg=file command line option\n");
         printf (" to specify its name and location.  Otherwise, you can use the\n");
         printf (" /Generate option to create a sample configuration file.  You may\n");
         printf (" then Edit this configuration file to change default settings.\n");
         }
      exit (1);
      }

   /*--- load command line args (overrides cfg file) ---*/
   if (ArgIs ("MaxProcesses"))
      VarSet (&pvCFG, "MaxProcesses", ArgGet ("MaxProcesses", 0));
   if (ArgIs ("WKP"))
      VarSet (&pvCFG, "WKP",          ArgGet ("WKP", 0));
   if (ArgIs ("Debug"))
      VarSet (&pvCFG, "Debug",        "TRUE");
   if (ArgIs ("LogFile"))
      VarSet (&pvCFG, "LogFile",      ArgGet ("LogFile", 0));
   if (ArgIs ("Restart"))
      VarSet (&pvCFG, "Restart",      "TRUE");
   if (ArgIs ("Lines"))
      VarSet (&pvCFG, "ScreenLines",  ArgGet ("Lines", 0));
   if (ArgIs ("JobDir"))
      VarSet (&pvCFG, "JobDir",       ArgGet ("JobDir", 0));

   if (ArgIs ("DefaultJob"))
      VarSet (&pvCFG, "DefaultJob",   ArgGet ("DefaultJob", 0));
   if (ArgIs ("ExecCmd"))
      VarSet (&pvCFG, "ExecCmd",      ArgGet ("ExecCmd", 0));
   if (ArgIs ("PrintCmd"))
      VarSet (&pvCFG, "PrintCmd",     ArgGet ("PrintCmd", 0));

   if (ArgIs ("PClass"))
      VarSet (&pvCFG, "PClass",       ArgGet ("PClass", 0));
   if (ArgIs ("PDelta"))
      VarSet (&pvCFG, "PDelta",       ArgGet ("PDelta", 0));
   if (ArgIs ("ServicePClass"))
      VarSet (&pvCFG, "ServicePClass", ArgGet ("ServicePClass", 0));
   if (ArgIs ("ServicePDelta"))
      VarSet (&pvCFG, "ServicePDelta", ArgGet ("ServicePDelta", 0));

   /*--- test to ensure non defaulted vals are present and valid ---*/
   LookForEntry ("JobDir",             TRUE,  TRUE);
   LookForEntry ("FilesDir",           TRUE,  TRUE);
   LookForEntry ("TempDir",            FALSE, TRUE);
   LookForEntry ("ClientInstallFiles", TRUE,  TRUE);

   /*--- Set globals ---*/
   SetBamsServGlobals ();
   }



VOID TriggerShutdown (void)
   {
   DosSemClear (&semShutdownEvent);
   }


VOID WaitOnShutdown (void)
   {
   int iRet;

   DosSemWait (&semShutdownEvent, SEM_INDEFINITE_WAIT);

   bSHUTDOWN = TRUE;
   flushall ();
   JMLog (1, "Shutting Down...");
   DosSleep (1000);                   // Let Messages come through         
   iRet = SokDisconnect (sokWKP);
   JMLog (5, "SokDisconnect returns %d", iRet);
   DosSleep (1000);                   // Let Messages come through         
   CloseUserIO ();

   SokDisconnect (sokWKP);            // keep trying ...
// exit (0);
   DosExit (EXIT_PROCESS, 0);
   }



/*
 * Do any startup
 *
 */
BOOL InitServer (void)
   {
   LONG lWkp, lTime;
   PSZ  pszLog;
   char sz [256];
   char sz1[256];
   char sz2[256];

   if (bLOG)
      {
      pszLog = VarGet (pvCFG, "LogFile", NULL);
      if (!(fpLOGFILE = my_fopen (pszLog, "a")))
         if (!(fpLOGFILE = my_fopen (pszLog, "w")))
            JMError ("*** Unable to open log file: %s *** ", pszLog);

      if (fpLOGFILE)
         setvbuf (fpLOGFILE, NULL, _IONBF, 0);

      lTime = time (NULL);
      fprintf (fpLOGFILE, "\n");
      sprintf (sz, "BAMS Job Server %s Startup    LogLevel=%d    %s  %s",
                            VERSTR, (USHORT)VarGetl (pvCFG, "LogFileLevel"),
                            DateString (sz1, lTime), TimeString (sz2, lTime));
      Identify (fpLOGFILE, sz);
      }
   lWkp = VarGetl (pvCFG, "WellKnownPort");

   if (SokInit ())
      {
      JMError ("Unable to Initialize Socket interface!");
      TriggerShutdown ();
      return FALSE;
      }
   if ((sokWKP = SokCreateServer (lWkp, TCP)) == -1)
      {
      JMError ("Unable to Create Socket interface at port %d", lWkp);
      TriggerShutdown ();
      return FALSE;
      }
   JMLog (6, "WKP Socket Address=%d SocketVal=%d", lWkp, sokWKP);
   return TRUE;
   }




/*
 * This is the main routine for the server
 * This is where the well known port is handled
 *
 *
 * ICK
 * This will need to be fixed.
 * sSOCK can be changed before it is referenced by a child thread
 *
 */

VOID WKPHandlerFn (PVOID p)
   {
   SOCKET  sock;

   /*--- Set priority for socket service handler ---*/
   SetPriority ("ServicePClass", "ServicePDelta", 0, TRUE);
   DosSleep (500);

   while (TRUE)
      {
      JMLog (5, "Waiting for a connection ...");
      sock = SokAcceptCon (sokWKP);

      if (sock == -1)  
         {
         if (!bSHUTDOWN)
            JMError ("Accept Connection failed. Is this a shutdown?");
         DosSleep (2000);
         }
      else
         {
         JMLog (7, "initiating a connection ...");
         StartThread (AcceptFn, sock);
         }
      JMLog (10, "Thread 1 going ...");
      }
   }



/*
 * This is the main
 *
 *
 */
int _cdecl main (int argc, char *argv[])
   {
   long  lTime;

   if (ArgBuildBlk ("? *^Help *^Debug *^LogFile% *^Restart *^CfgFile% "
                    "*^MaxProcesses% *^WKP% *^Lines% *^Generate "
                    "*^JobDir% *^DefaultJob% *^ExecCmd% "
                    "*^PrintCmd *^PClass% *^PDelta%"))
      Error ("%s", ArgGetErr ());

   if (ArgFillBlk (argv))
      Error ("%s", ArgGetErr ());

   if (ArgIs ("Help") || ArgIs ("?"))
      Usage ();

   if (ArgIs ("Generate"))
      GenerateCfg (argv[0]);

   LoadVariables (argv[0]); // Load variables from cfg file and cmd line

// MemSetDebugMode (MEM_ERRORS);

   /*--- Set priority for entire job monitor ---*/
   SetPriority ("PClass", "PDelta", 0, FALSE);

   /*--- do this before inits ---*/
   DosSemSet (&semShutdownEvent);

   if (!InitUserIO ())    // Init Screen and user input threads
      WaitOnShutdown ();  // This must be done before any JMError calls etc.

   if (!InitServices ())  // Init command services
      WaitOnShutdown ();
   if (!InitServer ())    // Init tcpip connection, and open log file
      WaitOnShutdown ();
   if (!InitQReader ())   // Init Job Q reader threads
      WaitOnShutdown ();

   if (bDEBUG)
      JMLog (3, "*** Job Server is Running in DEBUG mode ***");

   if ((VarGetl (pvCFG, "LogFileLevel") > 5) && bLOG)
      {
      fprintf (fpLOGFILE, "============ Variable Dump START ============\n");
      VarDump (fpLOGFILE, pvCFG);
      fprintf (fpLOGFILE, "============ Variable Dump END ============\n");
      }

   /*--- conditionally re-load queued jobs ---*/
   if (VarTrue (pvCFG, "Restart"))
      RestartServer ();

   /*--- conditionally start purge daemon ---*/
   if (lTime = VarGetl (pvCFG, "AutoPurge"))
      StartThread (PurgeDaemonFn, lTime);

   /*--- start the grim reaper (kills runaway jobs) ---*/
   StartThread (GrimReaperFn, lTime);

   /*--- Create WKP handler thread ---*/
   StartThread (WKPHandlerFn, 0);

   /*--- Send the main thread off to handle the user interface ---*/
   StartThread (HandleUserIOFn, 0);

   WaitOnShutdown ();
   return 0;
   }

