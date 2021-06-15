
/*
 * STAT Structure
 * This struct type stores various juicy stats about the server
 * This struct type is intended to be used for a single global structure
 *
 */
typedef struct
   {
   TIME   tmServerStart;      // DateTime server was started

   /*--- Stats for completed jobs ---*/
   TIME   tmTotalQueueTime;
   TIME   tmTotalRunTime;

   /*--- Stats for all submitted jobs ---*/
   USHORT uJobsQueued;
   USHORT uJobsStarted;
   USHORT uJobsCompleted;
   USHORT uJobsPrinted;
   
   USHORT uMaxJobsInQueue;
   USHORT uJobsRestarted;
   USHORT uJobsRefused;
   USHORT uSecurityFailures;
   USHORT uCommands [JSC_LASTCMD+1];
   } STAT;
typedef STAT *PSTAT;



void InitStat (void);

PSTAT StatAccess (BOOL bAccess);

BOOL StatDump (FILE *fp);
