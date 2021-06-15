/*
 *
 * stat.c
 * Thursday, 4/20/1995.
 *
 */

#include <os2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "Var.h"
#include "bamsserv.h"
#include "util.h"
#include "Stat.h"

STAT stat;   // This contains global job monitor statistics

ULONG psemStatMux = 0;

/**************************************************************************/
/*                                                                        */
/*                                                                        */
/*                                                                        */
/**************************************************************************/

void InitStat (void)
   {
   memset (&stat, 0, sizeof (stat));
   }


PSTAT StatAccess (BOOL bGrant)
   {
   if (bGrant)
      {
      DosSemRequest (&psemStatMux, SEM_INDEFINITE_WAIT);
      return &stat;
      }
   DosSemClear (&psemStatMux);
   return NULL;
   }




BOOL StatDump (FILE *fp)
   {
   PSTAT pstat;
   TIME  tmNow;
   char  szBuff[128];

   pstat = StatAccess (TRUE);
   time ((time_t *)&tmNow);

   fprintf (fp, "      Server Start Time: %s\n", DateTimeString (szBuff, pstat->tmServerStart));
   fprintf (fp, "           Current Time: %s\n", DateTimeString (szBuff, tmNow));

   fprintf (fp, "      Average Wait Time: %s\n", DeltaTimeString (szBuff, pstat->tmTotalQueueTime, pstat->uJobsCompleted));
   fprintf (fp, "   Average Running Time: %s\n", DeltaTimeString (szBuff, pstat->tmTotalRunTime,   pstat->uJobsCompleted));

   fprintf (fp, "      Total Jobs Queued: %d\n", pstat->uJobsQueued);
   fprintf (fp, "     Total Jobs Started: %d\n", pstat->uJobsStarted);
   fprintf (fp, "   Total Jobs Completed: %d\n", pstat->uJobsCompleted);
   fprintf (fp, "     Total Jobs Printed: %d\n", pstat->uJobsPrinted);
   fprintf (fp, "      Max Jobs in Queue: %d\n", pstat->uMaxJobsInQueue);
   fprintf (fp, "         Jobs Restarted: %d\n", pstat->uJobsRestarted);
   fprintf (fp, "     Total Jobs Refused: %d\n", pstat->uJobsRefused);
   fprintf (fp, "Total Security Failures: %d\n", pstat->uSecurityFailures);

   fprintf (fp, "    PING       Commands: %d\n", pstat->uCommands[JSC_PING      ]);
   fprintf (fp, "    SUBMIT     Commands: %d\n", pstat->uCommands[JSC_SUBMIT    ]);
   fprintf (fp, "    LISTJOBS   Commands: %d\n", pstat->uCommands[JSC_LISTJOBS  ]);
   fprintf (fp, "    LISTFILES  Commands: %d\n", pstat->uCommands[JSC_LISTFILES ]);
   fprintf (fp, "    PRINT      Commands: %d\n", pstat->uCommands[JSC_PRINT     ]);
   fprintf (fp, "    COPY       Commands: %d\n", pstat->uCommands[JSC_COPY      ]);
   fprintf (fp, "    DELETEJOB  Commands: %d\n", pstat->uCommands[JSC_DELETEJOB ]);
   fprintf (fp, "    DELETEFILE Commands: %d\n", pstat->uCommands[JSC_DELETEFILE]);
   fprintf (fp, "    ACCEPT     Commands: %d\n", pstat->uCommands[JSC_ACCEPT    ]);
   fprintf (fp, "    PURGE      Commands: %d\n", pstat->uCommands[JSC_PURGE     ]);
   fprintf (fp, "    SETDEFAULT Commands: %d\n", pstat->uCommands[JSC_SETDEFAULT]);
   fprintf (fp, "    GETINFO    Commands: %d\n", pstat->uCommands[JSC_GETINFO   ]);
   fprintf (fp, "    TEST       Commands: %d\n", pstat->uCommands[JSC_TEST      ]);
   fprintf (fp, "    TEST2      Commands: %d\n", pstat->uCommands[JSC_TEST2     ]);

   pstat = StatAccess (FALSE);
   return TRUE;
   }
