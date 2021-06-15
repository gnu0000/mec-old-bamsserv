/*
 *
 * userio.h
 * Monday, 1/9/1995.
 *
 * (C) 1995 Info Tech Inc.
 * Craig Fitzgerald
 *
 * Part of the BAMS Job Server
 * This module handles screen painting and server user input
 */

/*
 * These values are used by UpdateWindow
 * They determine which window to update
 */
#define JM_QUEUE_WINDOW    1
#define JM_RUNNING_WINDOW  2
#define JM_COMMAND_WINDOW  3
#define JM_ERROR_WINDOW    4

#define COMMANDWINDOWSIZE  60
#define ERRORWINDOWSIZE    60


/*
 * Startup and closedown procedures
 * They aint optional
 */
BOOL InitUserIO (void);

void CloseUserIO (void);

void ScreenDump (FILE *fp);

void SetUserIOGlobals (void);

VOID HandleUserIOFn (PVOID p);


/*
 * Write a string onto window uWindow
 * The values for uWindow are defined above
 */
void UpdateWindow (USHORT uWindow, PSZ pszLine);


/*
 *  These values are set here at startup
 *  They are used here, and in util.c
 *
 */
extern USHORT uLOG_WINDOW_LEVEL;
extern USHORT uLOG_FILE_LEVEL  ;



