/*
 *
 * security.h
 * Friday, 2/24/1995.
 *
 * (C) 1995 Info Tech Inc.
 * Craig Fitzgerald
 *
 * Part of the BAMS Job Server
 * This module contains the fn to negotiate security with the client
 */


/*
 * This fn negotiates security with the client
 * It returns FALSE if security fails
 */
BOOL SecurityCheck (SOCKET sock);
