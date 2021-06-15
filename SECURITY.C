/*
 *
 * security.c
 * Friday, 2/24/1995.
 *
 * (C) 1995 Info Tech Inc.
 * Craig Fitzgerald
 *
 * Part of the BAMS Job Server
 * This module contains the fn to negotiate security with the client
 */

#define INCL_DOS
#include <os2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <GnuDes.h>
#include "Var.h"
#include "BamsServ.h"
#include "Syssock.h"
#include "osock.h"
#include "Util.h"
#include "SokUtil.h"

/*
 * The string value below is intentionally misleading
 * Do not change it.  It must be in sync with the client key string
 *
 */
char  szCrud[] = "Ini File Not Found";


static PSZ GenerateKey (PSZ pszKey)
   {
   strcpy (pszKey, szCrud);
   pszKey [9] = 'G';
   pszKey [5] = 'N';
   pszKey [3] = 'U';
   return pszKey;
   }



static PSZ GenerateString (PSZ pszString)
   {
   USHORT i;

   /* 
    * random number seeding needs to be done here rather than in an
    * init fn, or in main, because number generation is thread dependent
    */
   srand ((USHORT) time (NULL) % 32760);

   for (i=0; i<24; i++)
      pszString[i] = (Rnd(2) ? 'A' : 'a') + Rnd(26);
   pszString[i] = '\0';

   return pszString;
   }



BOOL SecurityCheck (SOCKET sock)
   {
   char szKey    [64];
   char szString [64];
   char szDest   [128];
   char szClient [128];
   USHORT uRet;

   GenerateString (szString);
   GenerateKey (szKey);
   DesEncrypt (szDest, szString, szKey);

   JMLog (7, "Negotiating security ...");

   if (MySokBuffSend(sock, szString, strlen (szString)+1) == -1)
      return JMError ("Unable to send security string to client");

   if (MySokBuffRecv(sock, szClient, sizeof (szClient)) == -1)
      return JMError ("Unable to get security string from client");

   JMLog (11, "Security: Server [%2.2d]: '%s'", strlen(szDest), szDest);
   JMLog (11, "Security: Client [%2.2d]: '%s'", strlen(szClient), szClient);

   if (!(uRet = !strnicmp (szDest, szClient, strlen (szDest))))
      JMError ("Client security clearance failed");

   *(PUSHORT)szKey = uRet;
   if (MySokBuffSend(sock, szKey, sizeof (USHORT)) == -1)   // 1=ok
      JMError ("Unable to send security status to client");

   return uRet;
   }

