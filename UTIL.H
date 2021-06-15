/*
 *
 * util.h
 * Thursday, 1/5/1995.
 *
 * (C) 1995 Info Tech Inc.
 * Craig Fitzgerald
 *
 * Part of the BAMS Job Server
 * This mod contains various utility functions
 */


/*
 * This is the stack size that all the cute little
 * threads get when they are created
 */
#define STACK_SIZE      16384


//typedef VOID (*_Optlink THREADFN) (PVOID p);

//typedef VOID (_Optlink *THREADFN) (PVOID p);

typedef VOID (*THREADFN) (PVOID p);


#if defined (_MSC_VER)
#define  FILEFINDBUF3 FILEFINDBUF
#define  FNTYPE _cdecl
#else
#define  FNTYPE
#endif



/*
 * This is basically a fopen that
 * does not allow hard errors
 *
 */
FILE *my_fopen (PSZ pszFile, PSZ pszMode);



/*
 * This is the fn to call to start a new thread
 *
 * thread fn format:
 *    VOID APIENTRY FNTHREAD(ULONG)
 */
ULONG StartThread (THREADFN pfn, ULONG ulArg);


/*
 * general string fn.
 * concatenates n string to pszDest, pszDest is returned
 */
PSZ FNTYPE cat (PSZ pszDest, PSZ pszSrc, ...);


/*
 * Use these fn to log information to the log windows and log files
 * for JMLog, uLogVal is the minimum log level to display/write the message
 */
USHORT FNTYPE JMError  (PSZ psz, ...);
USHORT FNTYPE JMWarning(PSZ psz, ...);
USHORT FNTYPE JMLog (USHORT uLogVal, PSZ psz, ...);


/*
 * These fn's return the requested directory given several
 * param formats.
 * UserDir returns the path to the users base file dir
 * FilesDir fn's return the path to the OutFiles or SysFiles dir
 *   depending on bSysFiles
 *
 */
PSZ UserDir (PSZ pszPath, PSZ pszUserID);
PSZ FilesDir0 (PSZ pszBuff, PSZ pszUserID, PSZ pszJobID, USHORT bSysFiles);
PSZ FilesDir (PSZ pszPath, PCOMMAND pcmd, BOOL bSysFiles);
PSZ FilesDir2 (PSZ pszPath, PJOB pjob, BOOL bSysFiles);

/*
 * malloc structures
 *
 *
 *
 */
PJOB MakeJob (void);


/*
 * free structures
 *
 *
 */
PJOB FreeJob (PJOB pjob);
PCOMMAND FreeCommand (PCOMMAND pcmd);


/*
 * converts time, date to a string
 * and returns the string
 */
PSZ TimeString (PSZ pszBuff, TIME time);
PSZ DateString (PSZ pszBuff, TIME time);
PSZ DateTimeString (PSZ pszBuff, TIME time);
PSZ DeltaTimeString (PSZ pszBuff, TIME time, USHORT uDiv);


/*
 * creates a temporary file, opens it, and returns its handle
 * pszBase, if non NULL, should be no longer than 4 chars
 *
 */
FILE *CreateTempFile (PSZ pszMode, PSZ pszDir, PSZ pszBase, PSZ pszTempFile);


/*
 * This fn takes a string, and works it over until it is
 * a valid file/dir name
 *
 */
PSZ MakeValidDirName (PSZ pszDest, PSZ pszSrc);

/*
 * returns TRUE if it exists
 *
 */
BOOL PathExists (PSZ pszPath);


/*
 * Strips away leading path info about a file
 * optionally strips away extension
 *
 */
PSZ BaseName (PSZ pszBase, PSZ pszFullPath, BOOL bKeepExt);


/*
 *
 *
 */
PSZ NewExtension (PSZ pszNewPath, PSZ pszOldPath, PSZ pszExt);


/*
 * Deletes all files in the dir
 * and then removes the dir
 */
BOOL RemoveDir (PSZ pszDir, BOOL bKillChildren);


/*
 * This is a findfirst/findnext user dir
 * This fn is re-entrant
 * the calling fn must create a hdir = 0; for the first call
 * The last param may be null.
 *
 */
PSZ FindAFile (PSZ          pszBuff, // buffer to hold new name
               HDIR         *phdir,  // handle, start with 0
               PSZ          pszBase, // base dir to search
               USHORT       uAtts,   // attributes to match
               FILEFINDBUF3 *pfbuf); // file struct, can be NULL



/*
 * This file deletes all closed, empty files in pszDir
 *
 *
 */
void RemoveEmptyFiles (PSZ pszDir);


/*
 * returns a 'random' number from 0 to iLimit-1
 *
 */
SHORT Rnd (SHORT iLimit);


/*
 * Change priority of process or thread
 * lDelta is added to the priority delta
 *
 */
void SetPriority (PSZ pszPClass, PSZ pszPDelta, LONG lDelta, BOOL bThread);


//
//PVOID MyMalloc (USHORT uSize);
//
//PVOID MyCalloc (USHORT uCount, USHORT uSize);
//
//PSZ MyStrdup (PSZ pszOrg);
//
//PVOID MyFree (PVOID p);

PSZ NumStr (PSZ psz, USHORT i);


void Identify (FILE *fp, PSZ pszStr);


/*
 *  This function converts a relative time to an absolute time
 *
 *  if tm is <= 1000 it is considered a relative time which is
 *  expressed in terms of hours from the present.
 *
 *  example tm=1 is converted to 1 hour ago
 *
 */
TIME FixTime (TIME tm);


extern int XlateReturnCode (PJOB pjob, int iCStat, int iStat, int errnocpy);


extern PSZ ReturnCodeString (int iStat);



