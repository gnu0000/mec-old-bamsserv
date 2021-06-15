/*
 *
 * EBLIB.c
 *
 *
 * (C) 1992-1994 Info Tech Inc.
 * Craig Fitzgerald
 *
 * This file is part of the EBS module
 *
 * This is the main file for the EBLib program.  This file provides the
 * functionality for handling library files.
 *
 */

#define INCL_VIO
#include <os2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <conio.h>
#include <GnuMem.h>
#include <GnuStr.h>
#include <GnuFile.h>
#include <GnuZip.h>
#include <GnuScr.h>
#include <GnuKbd.h>
#include "ReadEBL.h"
#include "EbLib.h"

#define BUFFERSIZE    35000U
#define INITCRC       12345L

PSZ   pszWorkBuff = NULL;
FTIME ftimeZero = {0, 0, 0};
FDATE fdateZero = {0, 0, 0};


/*******************************************************************/
/*                                                                 */
/*                                                                 */
/*                                                                 */
/*******************************************************************/

PGW  pgwX;
char szHeader [80];
char szErrStr [160];

void _CreateWindow (PSZ pszLib)
   {
   PSZ psz;

   pgwX = GnuCreateWin (8, 70, NULL);
   psz = (strrchr (pszLib, '\\') ? strrchr (pszLib, '\\')+1 : pszLib);

   GnuPaint (pgwX, 0, 17, 0, 0, "Processing Library:");
   GnuPaint (pgwX, 0, 37, 0, 1, psz);
   GnuPaint (pgwX, 2, 19, 0, 0, "Extracting File:");
   GnuPaintNChar (pgwX, 3, 19, 0, 0, '°', 30);
   }


/*
 * update file name
 *
 */
void _UpdateWindowFile (PSZ pszFile)
   {
   GnuPaintNChar (pgwX, 2, 36, 0, 0, ' ', 15);
   GnuPaint (pgwX, 2, 36, 0, 1, pszFile);
   GnuPaintNChar (pgwX, 3, 19, 0, 0, '°', 30);
   }


/*
 * update status bar
 *
 */
ULONG ulFILEREAD;
ULONG ulFILESIZE;

//void _UpdateWindowPct (ULONG ulRead, ULONG ulSize)
void _UpdateWindowPct (void)
   {
   USHORT uVal;

   uVal = (USHORT)(((float)ulFILEREAD / (float)ulFILESIZE) * 30.0 + 0.25);
   GnuPaintNChar (pgwX, 3, 19, 0, 1, '±', uVal);
   }                                  


/*
 * returns FALSE
 */
BOOL _DisplayError (PSZ pszErr)
   {
   GnuPaint2 (pgwX, 4, 0, 3, 1, pszErr, 50);
   GnuPaint  (pgwX, 5, 0, 3, 1, "Press <Enter> to continue");
   KeyChoose ("\x1B\x0D", "");
   GnuPaintNChar (pgwX, 4, 0, 3, 0, ' ', 50);
   GnuPaintNChar (pgwX, 5, 0, 3, 0, ' ', 50);
   return FALSE;
   }


/*
 *
 *
 */
BOOL _DestroyWindow ()
   {
   GnuDestroyWin (pgwX);
   return FALSE;
   }

/*******************************************************************/
/*                                                                 */
/*                                                                 */
/*                                                                 */
/*******************************************************************/

//USHORT Err (PSZ p1, PSZ p2)
//   {
//   printf ("EBLIB: ");
//   printf (p1, p2);
//   printf ("\n");
//   exit (1);
//   return 1;
//   }


BOOL touch (PSZ pszFileName, FDATE fdate, FTIME ftime)
   {
   HFILE      hFile;
   FILESTATUS fstsBuf;
   USHORT     uAction;

   if (DosOpen (pszFileName, &hFile, &uAction, 0UL,
                FILE_NORMAL, FILE_OPEN, 
                OPEN_ACCESS_READWRITE | OPEN_SHARE_DENYNONE, 0UL))
      return FALSE;

   if (DosQFileInfo (hFile, 0x0001, &fstsBuf, sizeof fstsBuf))
      return DosClose (hFile);

   /*--- we should only have to change the last write date ---*/
   fstsBuf.fdateLastWrite  = fdate;
   fstsBuf.ftimeLastWrite  = ftime;
   fstsBuf.fdateCreation   = fdateZero;
   fstsBuf.ftimeCreation   = ftimeZero;
   fstsBuf.fdateLastAccess = fdateZero;
   fstsBuf.ftimeLastAccess = ftimeZero;

   if (DosSetFileInfo (hFile, 0x0001, (PVOID)&fstsBuf, sizeof fstsBuf))
      return DosClose (hFile);

   return !DosClose (hFile);
   }



/*******************************************************************/
/*                                                                 */
/* Compression / Uncompression routines                            */
/*                                                                 */
/*******************************************************************/



/*
 * returns:
 *  0 - ok
 *  5 - file size error
 */
USHORT UncompressFile (FILE *fpIn, FILE *fpOut, ULONG ulInSize, ULONG ulOutSize)
   {
   USHORT uInSize, uOutSize, uErr = 0;
   ULONG  ulSrcRead, ulTotalOut;

   ulSrcRead = ulTotalOut = 0;

   ulFILEREAD = 0;
   ulFILESIZE = ulOutSize;

   while (ulSrcRead < ulInSize)
      {

      uErr = Cmp2fpEfp (fpOut, &uOutSize, fpIn, &uInSize);
      ulSrcRead  += uInSize;
      ulTotalOut += uOutSize;

//    _UpdateWindowPct (ulTotalOut, ulOutSize);
      }
   if (ulTotalOut != ulOutSize)
      {
      sprintf (szErrStr, "Size Err: Expected:%ld  Got:%ld\n", ulOutSize, ulTotalOut);
      _DisplayError (szErrStr);
      return 1;
      }
   else if (uErr)
      {
      if (uErr == 9)
         sprintf (szErrStr, "Cannot Write: Disk Full?");
      else
         sprintf (szErrStr, "Error Uncompressing file");
      _DisplayError (szErrStr);
      return 1;
      }
   return 0;
   }



USHORT CopyFile (FILE *fpIn, FILE *fpOut, ULONG ulSize)
   {
   USHORT uPiece, uIOBytes;
   ULONG  ulWrote;

   ulWrote = 0;
   ulFILESIZE = ulSize;

   while (ulWrote < ulSize)
      {
      uPiece = (USHORT) min ((ULONG)BUFFERSIZE, ulSize);
      uIOBytes = fread (pszWorkBuff, 1, uPiece, fpIn);
      if (uPiece != uIOBytes)
         return 1;

      uIOBytes = fwrite (pszWorkBuff, 1, uPiece, fpOut);
      if (uPiece != uIOBytes)
         return 2;

      ulWrote += uPiece;

      ulFILEREAD = ulWrote;
      _UpdateWindowPct ();

      if (bGENWRITECRC)
         ulWRITECRC = CRC_BUFF (ulWRITECRC, pszWorkBuff, uPiece);
      if (bGENREADCRC)
         ulREADCRC = CRC_BUFF (ulREADCRC, pszWorkBuff, uPiece);
      }
   return 0;
   }



/*
 * This fn writes a file's data from a lib to a file
 * This fn assumes the file pointer is pointing
 * to the start of the file data area unless bSetFilePos 
 */
USHORT WriteToFile (PFDESC pfd, PSZ pszPath, BOOL bSetFilePos)
   {
   FILE   *fpOut;
   USHORT uErr;
   char   szFile [256];

   if (!pszPath)
      strcpy (szFile, pfd->szName);
   else if (*pszPath && pszPath[strlen (pszPath)-1] != '\\')
      sprintf (szFile, "%s\\%s", pszPath, pfd->szName);
   else
      sprintf (szFile, "%s%s", pszPath, pfd->szName);

   if (!(fpOut = fopen (szFile, "wb")))
      {
      sprintf (szErrStr, "ERROR: can't open file: %s\n", szFile);
      _DisplayError (szErrStr);
      return FALSE;
      }

   if (bSetFilePos)
      fseek (pfd->pld->fp, pfd->ulOffset, SEEK_SET);

   ReadMark (pfd->pld->fp);

   _UpdateWindowFile (pfd->szName);

   /*--- compression module vars ---*/
   bGENREADCRC  = TRUE;
   bGENWRITECRC = FALSE;
   ulREADCRC    = INITCRC;

   if (pfd->uMethod)
      uErr = UncompressFile (pfd->pld->fp, fpOut, pfd->ulSize, pfd->ulLen);
   else
      uErr = CopyFile (pfd->pld->fp, fpOut, pfd->ulSize);

//   _UpdateWindowPct (1, 1);

   fclose (fpOut);
   if (ulREADCRC != pfd->ulCRC)
      {
      sprintf (szErrStr, "Error: file fails CRC check.");
      _DisplayError (szErrStr);
      }
   else if (uErr)
      {
      sprintf (szErrStr, "Error: bad file size.");
      _DisplayError (szErrStr);
      }

   /*--- set date / time ---*/
   touch (szFile, pfd->fDate, pfd->fTime);

   /*--- set file mode ---*/
   DosSetFileMode (szFile, pfd->uAtt, 0);

   /*-- write 4dos descriptions --*/
   FilPut4DosDesc (szFile, pfd->szDesc);

   return uErr;
   }


/*******************************************************************/
/*                                                                 */
/*                                                                 */
/*                                                                 */
/*******************************************************************/

//PVOID fpUPDATEFN   = NULL;

void _cdecl UpdateFn (USHORT uSize)
   {
   ulFILEREAD += uSize;
   _UpdateWindowPct ();
   }


/**************************************************************************/
/*                                                                        */
/*                                                                        */
/*                                                                        */
/**************************************************************************/


PSZ InitEbLib (PSZ psz)
   {
   pszWorkBuff = (psz ? psz : malloc (35256U));
   Cmp2Init (pszWorkBuff, 3, 1);
   return pszWorkBuff;
   }


BOOL ExtractLib (PSZ pszLib, PSZ pszOutputPath)
   {
   PFDESC pfd;
   PLDESC pldIn;
   USHORT j;


   pfnUPDATEFN = UpdateFn;

   _CreateWindow (pszLib);

   if (!(pldIn = OpenLib (pszLib)))
      {
      _DisplayError (szLIBERR);
      _DestroyWindow ();
      return FALSE;
      }

   /*--- read in file descriptors ---*/
   for (j=0; j<pldIn->uCount; j++)
      {
      if (!(pfd = ReadFileInfo (pldIn, FALSE)))
         {
         _DisplayError (szLIBERR);
         _DestroyWindow ();
         fclose (pldIn->fp);
         return FALSE;
         }

      if (WriteToFile (pfd, pszOutputPath, FALSE))
         {
         _DestroyWindow ();
         fclose (pldIn->fp);
         return FALSE;
         }
      }
   fclose (pldIn->fp);
   _DestroyWindow ();
   return TRUE;
   }

