/*
 *
 * respond.h
 * Tuesday, 2/21/1995.
 *
 * (C) 1995 Info Tech Inc.
 * Craig Fitzgerald
 *
 * Part of the BAMS Job Server
 * This is for use by Client Modules
 * This module contains the fn to respond to the security request
 */

/*
 * This fn encodes the challenge string from the server
 * This fn is fully self contained (with des.c)
 * Do NOT make this an exportable DLL function
 *
 */
PSZ Respond (PSZ pszDest, PSZ pszSrc);
