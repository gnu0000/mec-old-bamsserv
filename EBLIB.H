/*
 * EBLIB.h
 *
 *
 * (C) 1992-1994 Info Tech Inc.
 *
 * Craig Fitzgerald
 *
 * This file is part of the EBS module
 *
 *
 *
 */


extern FDESC fdesc;

extern BOOL bDEBUGMODE;

PSZ InitEbLib (PSZ psz);

BOOL ExtractLib (PSZ pszLib, PSZ pszOutputPath);

