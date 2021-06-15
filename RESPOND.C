/*
 *
 * respond.c
 * Tuesday, 2/21/1995.
 *
 * (C) 1995 Info Tech Inc.
 * Craig Fitzgerald
 *
 * Part of the BAMS Job Server
 * This is for use by Client Modules
 * This module contains the fn to respond to the security request
 */

#include <os2.h>
#include <GnuDes.h>

/*
 *    CLIENT                         SERVER
 * ---------------------------------------------------------
 *    Sok connect 
 * ---------------------------------------------------------
 *                                   create challenge string
 *                                   send challenge string
 * ---------------------------------------------------------
 *    get challenge string
 *    encode challenge string
 *    send encoded string back
 * ---------------------------------------------------------
 *                                   get encoded string
 *                                   send status value (BOOL)
 * ---------------------------------------------------------
 *    get status value
 * ---------------------------------------------------------
 *
 * Challenge and Response strings are null terminated
 * Challenge string is <= 32 chars
 * Response string is <= 64 chars
 * Status=TRUE means string is accepted
 * sok errors abort connection
 *
 */


/*
 * The string value below is
 * intentionally misleading
 */
char  szCrud[] = "Ini File Not Found";


/*
 * This fn encodes the challenge string from the server
 * This fn is fully self contained (with des.c)
 * Do NOT make this an exportable DLL function
 *
 */

PSZ Respond (PSZ pszDest, PSZ pszSrc)
   {
   szCrud [9] = 'G';
   szCrud [5] = 'N';
   szCrud [3] = 'U';

   return DesEncrypt (pszDest, pszSrc, szCrud);
   }


