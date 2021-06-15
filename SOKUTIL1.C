/*
 *
 * sockutil1.c
 * Friday, 2/3/1995.
 *
 * (C) 1995 Info Tech Inc.
 * Craig Fitzgerald
 *
 * Part of the BAMS Job Server
 * This mod contains socket I/O routines and debug routines that
 * are used by the server and the send utility
 */

#include <os2.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "var.h"
#include "bamsserv.h"
#include "syssock.h"
#include "osock.h"

#define BUFLEN 1024





///*
// * for debugging
// *
// */
//static void PrintABuffer (PSZ pszBuff, int iLen)
//   {
//   int i, j;
//
//   printf ("\n");
//   printf ("BufferLen = %d\n", iLen);
//   for (i=0; i*16 < iLen+16; i++)
//      {
//      printf ("   ");
//      for (j=0; j<16 && i*16+j < iLen; j++)
//         printf ("%2.2x ", pszBuff[i*16+j]);
//      printf ("   ");
//      for (j=0; j<16 && i*16+j < iLen; j++)
//         printf ("%c", pszBuff[i*16+j]);
//      printf ("\n");
//      }
//   }


/*
 * for debugging
 *
 */
void ShowTheCmd (PCOMMAND pcmd)
   {
   printf ("---command---\n");
   printf ("uCommand    =%u\n",  pcmd->uCommand    );
   printf ("uPriority   =%u\n",  pcmd->uPriority   );
   printf ("pszUserID   =%s\n",  pcmd->pszUserID   );
   printf ("pszClientID =%s\n",  pcmd->pszClientID );
   printf ("pszServerID =%s\n",  pcmd->pszServerID );
   printf ("lMessagePort=%lu\n", pcmd->lMessagePort);
   printf ("pszJobID    =%s\n",  pcmd->pszJobID    );
   printf ("pszUser1    =%s\n",  pcmd->pszUser1    );
   printf ("pszUser2    =%s\n",  pcmd->pszUser2    );
   printf ("pszUser3    =%s\n",  pcmd->pszUser3    );
   printf ("lUser1      =%lu\n", pcmd->lUser1      );
   printf ("lUser2      =%lu\n", pcmd->lUser2      );
   printf ("lUser3      =%lu\n", pcmd->lUser3      );
   }


LONG SokCmdRecv (SOCKET sock, PCOMMAND pcmd)
   {
   char szBuff [BUFLEN];
   int  iLen;
   PSZ  pszBuff;

   if ((iLen = SokBuffRecv (sock, szBuff, BUFLEN)) == -1)
      return -1L;

   if (iLen < 27)
      return 1;

   pszBuff = szBuff;

   pcmd->uCommand = SokNetToHostShort (*((PUSHORT)pszBuff)); 
   pszBuff += sizeof (USHORT);

   pcmd->uPriority = SokNetToHostShort (*((PUSHORT)pszBuff));
   pszBuff += sizeof (USHORT);

   pcmd->pszUserID = strdup (pszBuff); 
   pszBuff = strchr (pszBuff, '\0') + 1;

   pcmd->pszClientID = strdup (pszBuff); 
   pszBuff = strchr (pszBuff, '\0') + 1;

   pcmd->pszServerID = strdup (pszBuff); 
   pszBuff = strchr (pszBuff, '\0') + 1;

   pcmd->lMessagePort = SokNetToHostLong (*((PLONG)pszBuff)); 
   pszBuff += sizeof (ULONG);

   pcmd->pszJobID = strdup (pszBuff); 
   pszBuff = strchr (pszBuff, '\0') + 1;

   pcmd->pszUser1 = strdup (pszBuff); 
   pszBuff = strchr (pszBuff, '\0') + 1;

   pcmd->pszUser2 = strdup (pszBuff); 
   pszBuff = strchr (pszBuff, '\0') + 1;

   pcmd->pszUser3 = strdup (pszBuff); 
   pszBuff = strchr (pszBuff, '\0') + 1;

   pcmd->lUser1 = SokNetToHostLong (*((PLONG)pszBuff)); 
   pszBuff += sizeof (ULONG);

   pcmd->lUser2 = SokNetToHostLong (*((PLONG)pszBuff)); 
   pszBuff += sizeof (ULONG);

   pcmd->lUser3 = SokNetToHostLong (*((PLONG)pszBuff)); 
   pszBuff += sizeof (ULONG);

   return 0;
   }


PSZ MyStrCpy (PSZ pszDest, PSZ pszSrc)
   {
   if (pszSrc)
      return strcpy (pszDest, pszSrc);
   *pszDest = '\0';
   return pszDest;
   }



USHORT SokSendCmd (SOCKET sock, PCOMMAND pcmd)
   {
   char szBuff [2048];
   PSZ  p;

   p = szBuff;

   *(PUSHORT)p = SokHostToNetShort (pcmd->uCommand);
   p += sizeof (USHORT);

   *(PUSHORT)p = SokHostToNetShort (pcmd->uPriority);
   p += sizeof (USHORT);

   MyStrCpy (p, pcmd->pszUserID);
   p = strchr (p, '\0') + 1;

   MyStrCpy (p, pcmd->pszClientID);
   p = strchr (p, '\0') + 1;

   MyStrCpy (p, pcmd->pszServerID);
   p = strchr (p, '\0') + 1;

   *(PULONG)p = SokHostToNetLong (pcmd->lMessagePort);
   p += sizeof (ULONG);

   MyStrCpy (p, pcmd->pszJobID);
   p = strchr (p, '\0') + 1;

   MyStrCpy (p, pcmd->pszUser1);
   p = strchr (p, '\0') + 1;

   MyStrCpy (p, pcmd->pszUser2);
   p = strchr (p, '\0') + 1;

   MyStrCpy (p, pcmd->pszUser3);
   p = strchr (p, '\0') + 1;

   *(PULONG)p = SokHostToNetLong (pcmd->lUser1);
   p += sizeof (ULONG);

   *(PULONG)p = SokHostToNetLong (pcmd->lUser2);
   p += sizeof (ULONG);

   *(PULONG)p = SokHostToNetLong (pcmd->lUser3);
   p += sizeof (ULONG);

   if (SokBuffSend (sock, szBuff, p - szBuff + 1) == -1)
      return 1;

   return 0;
   }


