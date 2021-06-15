/*
 *
 * exec.c
 * Friday, 3/31/1995.
 *
 * (C) 1995 Info Tech Inc.
 * Craig Fitzgerald
 *
 *
 * Part of the BAMS Job Server
 */

#include <os2.h>
#include <stdio.h>
#include <stdlib.h>



RunIt (PSZ pszProg, PSZ pszArgs)
   {
   APIRET rc;
   RESULTCODES rcResult;

   rc = DosExecPgm (szFail,              // Failure buff
                     sizeof (szFail),     // Failure Buff len
                     EXEC_ASYNCRESULT,    // Exec Flags
                     pszArgs,             // Arguments
                     NULL,                // Env
                     &rcResult,           // Result Codes
                     pszProg);            // Prog to run

   if (rc)
      {
      Error ...
      }

   uPID = rcResult->codeTerminate;

        /* DosCWait */
   rc = DosWaitChild (DCWA_PROCESS,   // exec options
                      DCWW_WAIT,      // wait options
                      &rcResult,      // term codes
                      &uPid,          // returned pid
                      uPID)           // pid



   }











StartSession ()
   {
   STARTDATA sd;

    sd.Length      =  sizeof (sd);        // USHORT 
    sd.Related     =  TRUE;                    // USHORT 
    sd.FgBg        =  FALSE;                    // USHORT 
    sd.TraceOpt    =  0;                    // USHORT 
    sd.PgmTitle    =  pszTitle;                    // PSZ    
    sd.PgmName     =  pszProg;                    // PSZ    
    sd.PgmInputs   =  pszArgs;                    // PBYTE  
    sd.TermQ       =  0;                    // PBYTE  
    sd.Environment =                      // PBYTE  
    sd.InheritOpt  =                      // USHORT 
    sd.SessionType =                      // USHORT 
    sd.IconFile    =                      // PSZ    
    sd.PgmHandle   =                      // ULONG  
    sd.PgmControl  =                      // USHORT 
    sd.InitXPos    =                      // USHORT 
    sd.InitYPos    =                      // USHORT 
    sd.InitXSize   =                      // USHORT 
    sd.InitYSize   =                      // USHORT 


    sd.Reserved;      // USHORT  
    sd.ObjectBuffer;  // PSZ     
    sd.ObjectBuffLen; // ULONG   
   }





DosStartSession
