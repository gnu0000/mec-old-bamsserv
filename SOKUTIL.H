/*
 *
 * sokutil.h
 * Friday, 2/3/1995.
 *
 * (C) 1995 Info Tech Inc.
 * Craig Fitzgerald
 *
 * Part of the BAMS Job Server
 * This mod contains socket I/O routines and debug routines
 */


/*
 * these are wrappers that print diagnostic info
 * They print diagnostic info
 *
 */
LONG MySokBuffSend   (SOCKET sSocket, PVOID pBuff, LONG lLength);
LONG MySokBuffRecv   (SOCKET sSocket, PVOID pBuff, LONG lLength);
USHORT MySokFileSend (SOCKET sSocket, PSZ pszFileName);
USHORT MySokFileRecv (SOCKET sSocket, PSZ pszFileName, PSZ pszPath);


/*
 * This debug fn writes the command struct (text form)
 * to stdout
 */
void ShowTheCmd (PCOMMAND pcmd);


/*
 * Reads the command struct from the socket
 *
 */
LONG SokCmdRecv (SOCKET sock, PCOMMAND pcmd);


/*
 * Writes the command struct to the socket
 *
 */
USHORT SokSendCmd (SOCKET sock, PCOMMAND pcmd);


/*
 * Sends status value/string to over the socket
 * also may display message to the log
 */
USHORT SendStatus (SOCKET sock, USHORT uVal, PSZ pszError, ...);


/*
 * This fn gets the status return from the socket
 * The client will send a status message when it gets a file
 * this fn returns FALSE if an error occurs
 */
BOOL RecvStatus (SOCKET sock, PUSHORT puStatus);


/*
 * This fn notifies the client of job completion
 *
 */
BOOL NotifyClient (PJOB pjob);

