/*
 *
 * sockutil2.c
 * Friday, 2/3/1995.
 *
 * (C) 1995 Info Tech Inc.
 * Craig Fitzgerald
 *
 * Part of the BAMS Job Server
 * This mod contains socket I/O routines and debug routines that
 * are used by the server utility
 */

#include <os2.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <GnuMisc.h>
#include "var.h"
#include "bamsserv.h"
#include "syssock.h"
#include "osock.h"
#include "util.h"

#define BUFLEN 1024

char *JMSErrors[] = 
         {"No Error",                 // 0  
          "Command Refused",          // 1  JSE_REFUSED            
          "Invalid Command",          // 2  JSE_INVALID_CMD        
          "Cannot Find File",         // 3  JSE_CANNOT_FIND_FILE   
          "Cannot Create File",       // 4  JSE_CANNOT_CREATE_FILE 
          "Cannot Create Dir",        // 5  JSE_CANNOT_CREATE_DIR  
          "Cannot Run",               // 6  JSE_CANNOT_RUN         
          "Cannot Print",             // 7  JSE_CANNOT_PRINT       
          "Cannot Delete",            // 8  JSE_CANNOT_DELETE
          "Insufficient Memory",      // 9  JSE_INSUFFICIENT_MEMORY
          "Insufficient Disk Space",  // 10 JSE_INSUFFICIENT_DISK  
          "Insufficient Information", // 11 JSE_INSUFFICIENT_INFO  
          "Socket Recv Error",        // 12 JSE_SOCKET_RECV_ERROR  
          "Socket Send Error",        // 13 JSE_SOCKET_SEND_ERROR  
          "INTERNAL ERROR!",          // 14 JSE_INTERNAL_ERROR     
          "No Error",                 // 15 ???
          NULL};



LONG MySokBuffSend (SOCKET sSocket, PVOID pBuff, LONG lLength)
   {
   LONG lRet, lLocalLen;

   lLocalLen = lLength;
   JMLog (7, "SOK: About To Send Buffer of length %lu", lLocalLen);
   lRet = SokBuffSend (sSocket, pBuff, lLocalLen);
   JMLog (7, "SOK: Send Buffer return length is %lu", lRet);
   return lRet;
   }


LONG MySokBuffRecv (SOCKET sSocket, PVOID pBuff, LONG lLength)
   {
   LONG lRet;

   JMLog (7, "SOK: About To Get Buffer");
   lRet = SokBuffRecv (sSocket, pBuff, lLength);
   JMLog (7, "SOK: Recv Buffer return is %ld", lRet);
   return lRet;
   }


USHORT MySokFileSend (SOCKET sSocket, PSZ pszFileName)
   {
   USHORT uRet;

   JMLog (7, "SOK: About To Send File %s", pszFileName);
   uRet = SokFileSend (sSocket, pszFileName);
   JMLog (7, "SOK: Send File return is %u", uRet);
   return uRet;
   }


USHORT MySokFileRecv (SOCKET sSocket, PSZ pszFileName, PSZ pszPath) 
   {
   USHORT uRet;
   char   szTmp [256]; // ask me not!
   char   szTmp2[256]; // ask me not!

   strcpy (szTmp, pszFileName);
   sprintf(szTmp2, "SOK: About To Recv File=%s, Path=%s", szTmp, pszPath);

   JMLog (7, szTmp2);
   uRet = SokFileRecv (sSocket, pszFileName, pszPath);
   JMLog (7, "SOK: Recv File return is %u", uRet);
   return uRet;
   }



/*
 * sends a return code to the client
 * Also displays an error string to screen and log file
 *
 * return code not sent if socket = -1
 * message not printed if error return uErr is 0 or string is null
 * returns the uErr value
 */
USHORT SendStatus (SOCKET sock, USHORT uErr, PSZ pszError, ...)
   {
   char szBuff [256];
   char szTmp [256];
   LONG lLen;
   va_list vlst;

   if (uErr && pszError && *pszError)
      {
      va_start (vlst, pszError);
      vsprintf (szBuff, pszError, vlst);
      va_end (vlst);
      sprintf (szTmp, "Send Status: %s", szBuff); // Don't ask
      JMError (szTmp);                            //
      }

   if (sock != -1)
      {
      *(PUSHORT) szBuff = SokHostToNetShort (uErr);
      sprintf (szBuff+2, "%s", JMSErrors [min (MAX_ERROR, uErr)]);
      lLen = strlen (szBuff+2) + 3;

      if (MySokBuffSend(sock, szBuff, lLen) == -1)
         JMError ("Cannot send return value to client!");
      }

   return uErr;
   }



/*
 * This fn gets the status return from the client
 * The client will send a status message when it gets a file
 * this fn returns FALSE if an error occurs
 */
BOOL RecvStatus (SOCKET sock, PUSHORT puStatus)
   {
   char szBuff [512];

   *puStatus = 0;
   if (MySokBuffRecv(sock, szBuff, sizeof (szBuff)) == -1)
      return FALSE;

   *puStatus = *((PUSHORT)szBuff);
   return TRUE;
   }



BOOL NotifyClient (PJOB pjob)
   {
   char   szMsg [256];
   SOCKET sok;

   if ((sok = SokCreateClient (UDP)) == -1)
      return !JMError ("Unable to create client socket to send DataGram");

   if (pjob->uRetCode)
      sprintf (szMsg, "Report %s Status: %s", pjob->pszJobTag, ReturnCodeString (pjob->uRetCode));
   else
      sprintf (szMsg, "Report %s has completed", pjob->pszJobTag);
      
//   if (IsFlag (pjob->uFlags, JSF_TIMEDOUT))
//      sprintf (szMsg, "Report %s has timed out", pjob->pszJobTag);
//   else if (IsFlag (pjob->uFlags, JSF_KILLED))
//      sprintf (szMsg, "Report %s was killed", pjob->pszJobTag);
//   else if (IsFlag (pjob->uFlags, JSF_ABNORMALEXIT))
//      sprintf (szMsg, "Report %s aborted abnormally", pjob->pszJobTag);
//   else if (!pjob->uRetCode)
//      sprintf (szMsg, "Report %s has completed", pjob->pszJobTag);
//   else
//      sprintf (szMsg, "Report %s has failed with status of %d", pjob->pszJobTag, pjob->uRetCode);

   if (SokDgramSend (sok, szMsg, pjob->pszClientID, pjob->lMessagePort) == -1)
      JMError ("Unable to send datagram to %s at %s %ld", pjob->pszUserID, pjob->pszClientID, pjob->lMessagePort);

   SokDisconnect (sok);
   return TRUE;
   }


