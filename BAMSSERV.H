/*
 * BAMSSERV.H
 *
 * (C) 1995 Info Tech Inc.
 * Craig Fitzgerald
 *
 * Part of the BAMS Job Server
 */


#define VERSTR "1.1a Beta"


/*
 * make control
 */
#if !defined (VAR_H)
#error You need to include var.h before bamsserv.h
#endif
#define BAMSSERV_H


/*
 * These are filenames created and/or used by the JobMon system
 *
 */
#define PARAMS_FILE      "JobParm.dat"
#define SUBSET_FILE      "ShlSubs.dat"
#define JOB_CTL_FILE     "Job.ctl"
#define LAST_ID_FILE     "LastID.ctl"
#define FILE_LIST_FILE   "FILELIST.dat"
#define JOB_LIST_FILE    "JOBLIST.dat"
#define BANNER_FILE      "Banner.txt"
#define CFG_FILE_EXT     ".CFG"
#define STAT_FILE        "Stats.dat"
#define INSTALLLIST_FILE "FILELIST.CSV"


/*
 * Status returns sent to client
 * If these values change, you must also
 * change the error strings in SokUtil2.c
 */
#define JSE_REFUSED             1  // Unable to handle command
#define JSE_INVALID_CMD         2  // Invalid Command
#define JSE_CANNOT_FIND_FILE    3  // File/Job Not Found
#define JSE_CANNOT_CREATE_FILE  4  // Unable to create File/Directory
#define JSE_CANNOT_CREATE_DIR   5  // Unable to create File/Directory
#define JSE_CANNOT_RUN          6  // Unable to run job
#define JSE_CANNOT_PRINT        7  // Unable to print
#define JSE_CANNOT_DELETE       8  // Unable to delete/kill file/job
#define JSE_INSUFFICIENT_MEMORY 9  // Insufficient Memory
#define JSE_INSUFFICIENT_DISK   10 // Insufficient Disk Space
#define JSE_INSUFFICIENT_INFO   11 // Insufficient info to exec command.
#define JSE_SOCKET_RECV_ERROR   12 // Unable to recieve data from socket
#define JSE_SOCKET_SEND_ERROR   13 // Unable to recieve data from socket
#define JSE_INTERNAL_ERROR      14 // Internal Error
#define JSE_UNKNOWN_ERROR       15 // Unknown Error

#define MAX_ERROR JSE_UNKNOWN_ERROR


/*
 * Command identifiers
 */
#define JSC_PING            1  // ping server 
#define JSC_SUBMIT          2  // submit a job
#define JSC_LISTJOBS        3  // list jobs for a given user
#define JSC_LISTFILES       4  // list files for a given user/job
#define JSC_PRINT           5  // print a file for a given user/job/file
#define JSC_COPY            6  // copy a file to the user
#define JSC_DELETEJOB       7  // delete a job for a given user
#define JSC_DELETEFILE      8  // delete a file for a given user/job/file
#define JSC_ACCEPT          9  // ADMIN: accept/deny specific commands
#define JSC_PURGE           10 // ADMIN: purge user/all files before a date
#define JSC_SETDEFAULT      11 // ADMIN: change default values
#define JSC_GETINFO         12 // ADMIN: get default values (incl performance)
#define JSC_SHUTDOWN        13 // ADMIN: shutdown the server
#define JSC_INSTALLFILES    14 // ADMIN: shutdown the server
#define JSC_TEST            15 // test mode
#define JSC_TEST2           16 // test mode #2


#define JSC_LASTCMD  JSC_TEST2 // last cmd (highest value)


/*
 * Job Status identifiers
 * used by JSC_LISTJOBS
 */
#define JSS_PENDING         1
#define JSS_RUNNING         2
#define JSS_DONE            3


/*
 * Job Priority identifiers
 * used by JSC_SUBMIT
 */
#define JSP_LOW_PRIORITY    1
#define JSP_MEDIUM_PRIORITY 2
#define JSP_HIGH_PRIORITY   3


/*
 * File type identifiers
 * used by JSC_LISTFILES
 */
#define JS_OUTPUTFILE      0
#define JS_SYSTEMFILE      1


/*
 * Data types used in structures, etc...
 *
 */
typedef LONG TIME;
typedef LONG DATE;


/*
 * COMMAND Structure
 * This structure is sent to the JS from the JM
 * to issue a command.
 *
 * Generic field usage:
 *
 * |---------|---------|------------------|-----------|----------|
 * |      cmd|         |                  |           | Delete   |
 * |field    | Submit  | Print            | Copy      | File     |
 * |---------|---------|------------------|-----------|----------|
 * |pszUser1 | JobName | FileName         | FileName  | FileName |
 * |pszUser2 | JobTag  | Printer[,Server] | LocalName |          |
 * |pszUser3 | JobDesc | Forms            |           |          |
 * |lUser1   |         | bSysFile         | bSysFile  | bSysFile |
 * |lUser2   |         | Copies           |           |          |
 * |---------|---------|------------------|-----------|----------|
 *
 * |---------|---------|----------|---------|----------------------|
 * |      cmd|         |          | Set     |                      |
 * |field    | Accept  | Purge    | Default | GetInfo              |
 * |---------|---------|----------|---------|----------------------|
 * |pszUser1 |         |          |Variable |       0-Stats        |
 * |pszUser2 |         |          |Value    |       1-Vars         |
 * |pszUser3 |         |          |         |       2-Screen Dumps |
 * |lUser1   | CmdVal  | Mode     |         | Flags-3-             |
 * |lUser2   | bAccept | datetime |         |       4-             |
 * |---------|---------|----------|---------|----------------------|
 *
 * The above list will need to be expanded to include the new commands ...
 *
 */
typedef struct _command
   {
   USHORT uCommand;      // The Command to execute
   USHORT uPriority;     // Command Priority
   PSZ    pszUserID;     // Client User's ID
   PSZ    pszClientID;   // Client Machine TCP ID
   PSZ    pszServerID;   // Server Machine TCP ID
   LONG   lMessagePort;  // Client Machine Status Socket
   PSZ    pszJobID;      // The Current Jobs ID
   PSZ    pszUser1;      // 
   PSZ    pszUser2;      //  command specific
   PSZ    pszUser3;      //  fields. See above list
   LONG   lUser1;        //
   LONG   lUser2;        //
   LONG   lUser3;        //
   } COMMAND;
typedef COMMAND *PCOMMAND;



/*
 * JOB Structure
 * This structure makes up the elements of the job queue linked list
 * and the running jobs linked list.
 */
typedef struct _JOB
   {
   PSZ    pszJobID;      // Job ID   (our system generated ID)
   PSZ    pszJobName;    // Job Name (the bams name)           {pszUser1}
   PSZ    pszJobTag;     // Job Tag  (the user specified name) {pszUser2}
   PSZ    pszJobDesc;    // Job Description (from the client)  {pszUser3}
   PSZ    pszUserID;     // User name
   PSZ    pszClientID;   // The client machine
   LONG   lMessagePort;  // The Client message port
   TIME   tmQueueTime;   // date / Time this job was queued
   TIME   tmStartTime;   // date / Time this job was started
   TIME   tmEndTime;     // date / Time this job was finished
   LONG   tmMaxRunTime;  // # of seconds before the job is killed
   USHORT uRetCode;      // Return code after exec
   USHORT uPriority;     // Job Priority
   int    iPID;          // Jobs process ID while running  ***NEW***
// ULONG  ulTest;        // for debug only!
   PVAR   pv;            // pre-chewed job parms. valid after pre-process job
   USHORT uFlags;        // status flags
   struct _JOB *next;    // Link to next element
   } JOB;
typedef JOB *PJOB;


/*
 * Flag values for JOB.uFlags
 * Jobs that are artificially killed may get these flags
 * On a shutdown, a job gets a JSF_RESTARTABLE so that it can restart
 * On a timeout, a job gets a JSF_TIMEDOUT so a specific msg can be
 * On a user kill, a job gets a JSF_KILLED so a specific msg can be sent
 * sent to the status monitor
 */
#define JSF_RESTARTABLE   1
#define JSF_TIMEDOUT      2
#define JSF_KILLED        3
#define JSF_ABNORMALEXIT  4


/*
 * Job execution status values
 * ---------------------------
 *
 *
 */
#define JRC_OK           0
#define JRC_APPERROR     1
#define JRC_STOP         2
#define JRC_PLIERROR     3
#define JRC_HARDERROR    4
#define JRC_TRAP         5
#define JRC_SIGTERM      6
#define JRC_TIMEDOUT     7
#define JRC_KILLED       8
#define JRC_RESTARTABLE  9
#define JRC_UNKNOWN      10
#define JRC_CANNOTEXEC   11




/*
 * Global variables used throughout the system
 *
 */
extern BOOL bDEBUG;      // This is TRUE when running in debug mode
extern BOOL bSHUTDOWN;   // This is TRUE once we are start to shutdown
extern BOOL bLOG;        // This is true if file logging is enabled
extern FILE *fpLOGFILE;  // This is the log file handle
extern PVAR pvCFG;       // This contains all system variables


/*
 * This fn simply sets bDEBUG and bLOG
 * It is a separate fn beacuse globals could later be reset via a 
 * JSC_SETDEFAULT command
 */
void SetBamsServGlobals (void);


/*
 * This fn is called to quit the server application
 *
 */
VOID TriggerShutdown (void);

