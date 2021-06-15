/*
 *
 * setdb.c
 * Thursday, 3/16/1995.
 * 
 * (C) 1995 Info Tech Inc.
 * Craig Fitzgerald
 *
 *
 * Part of BAMS 
 */

#define INCL_VIO
#include <os2.h>
#include <stdio.h>
#include <io.h>
#include <stdlib.h>
#include <string.h>
#include <GnuCfg.h>
#include <GnuScr.h>
#include <GnuStr.h>
#include <GnuDes.h>
#include <GnuRes.h>
#include <GnuKbd.h>
#include <GnuMisc.h>


#define ENTRIES   9
#define FIELDSIZE 53

#define DEFINIFILE   "BAMSDB.INI"

char szINIFILE [256];

char ppszField [ENTRIES][80];

char  szClientKey[] = "Database";
char  szBatchKey []  = "IniUFNle Got Found";



//[Client]
//Vendors=ODBC
//UserName=Joe
//Password=DGWWFWDFSDFASDVAVAVASASDG
//DBParm=Connectstring='DSN=BAMSDB;UID=DBA;PWD=SQL'
//
//
//[Batch]
//UserName=Harry
//Password=RTYTDGHSHSFBHSHSFGHFS$EEE
//DBParm=Connectstring='DSN=BAMSDB;UID=DBA;PWD=SQL'





void UpdateIni (PSZ pszFile)
   {
   char szP1[256], szP2[256];

   CfgSetLine (pszFile, "Client", "Vendor",   ppszField[0]);
   CfgSetLine (pszFile, "Client", "DBMS",     ppszField[1]);
   CfgSetLine (pszFile, "Client", "UserName", ppszField[2]);
   DesEncrypt (szP1, ppszField[3], szClientKey);
   CfgSetLine (pszFile, "Client", "Password", szP1);
   CfgSetLine (pszFile, "Client", "DBString", ppszField[4]);
   CfgSetLine (pszFile, "Batch",  "UserName", ppszField[5]);
   DesEncrypt (szP2, ppszField[6], szBatchKey );
   CfgSetLine (pszFile, "Batch",  "Password", szP2);
   CfgSetLine (pszFile, "Batch",  "Database", ppszField[7]);
   CfgSetLine (pszFile, "Batch",  "DBString", ppszField[8]);
   }



void CreateNewIni (PSZ pszFile)
   {
   FILE *fp;
   char szP1[256], szP2[256];

   if (!(fp = fopen (pszFile, "w")))
      Error ("Cannot open output file %s", pszFile);

   fprintf (fp, ";\n");
   fprintf (fp, "; Database Login for BAMS clients\n");
   fprintf (fp, ";\n");
   fprintf (fp, "[Client]\n");
   fprintf (fp, "Vendor=%s\n", ppszField[0]);
   fprintf (fp, "DBMS=%s\n", ppszField[1]);
   fprintf (fp, "UserName=%s\n", ppszField[2]);
   DesEncrypt (szP1, ppszField[3], szClientKey);
   fprintf (fp, "Password=%s\n", szP1);
   fprintf (fp, "DBString=%s\n", ppszField[4]);
   fprintf (fp, "\n");
   fprintf (fp, "\n");
   fprintf (fp, ";\n");
   fprintf (fp, "; Database login for BAMS batch programs\n");
   fprintf (fp, ";\n");
   fprintf (fp, "[Batch]\n");
   fprintf (fp, "UserName=%s\n", ppszField[5]);
   DesEncrypt (szP2, ppszField[6], szBatchKey );
   fprintf (fp, "Password=%s\n", szP2);
   fprintf (fp, "Database=%s\n", ppszField[7]);
   fprintf (fp, "DBString=%s\n", ppszField[8]);

   fclose (fp);
   }




void WriteFile (PSZ pszFile)
   {
   if (access (pszFile, 0))
      CreateNewIni (pszFile);
   else
      UpdateIni (pszFile);
   }   



void PreLoad (PSZ pszFile)
   {
   USHORT i;

   for (i=0; i<ENTRIES; i++)
      ppszField[i][0] = '\0';
   strcpy (ppszField[0], "ORACLE");
   strcpy (ppszField[1], "OR7 ORACLE v7.x");

   strcpy (ppszField[4], "@tns:BAMS");
   strcpy (ppszField[7], "BAMS");
   strcpy (ppszField[8], "tns:BAMS");

   if (access (pszFile, 0))
      return;

   CfgGetLine (pszFile, "Client", "Vendor",   ppszField[0]);
   CfgGetLine (pszFile, "Client", "DBMS",     ppszField[1]);

   CfgGetLine (pszFile, "Client", "UserName", ppszField[2]);
   CfgGetLine (pszFile, "Client", "DBString", ppszField[4]);
   CfgGetLine (pszFile, "Batch",  "UserName", ppszField[5]);
   CfgGetLine (pszFile, "Batch",  "Database", ppszField[7]);
   CfgGetLine (pszFile, "Batch",  "DBString", ppszField[8]);
   }




BOOL MakeIni (PSZ pszFile)
   {
   USHORT c, i, uField, uRet, uLen;
   PGW    pgw;

   pgw = GnuCreateWin (20, 75, NULL);
   pgw->pszFooter = "[<Tab>,Arrows=Move, <Enter>=Done, <Esc>=Abort]";
   GnuPaintBorder (pgw);

   GnuPaint (pgw,  1, 0,  3, 1, "DATABASE LOGON INFORMATION");
   GnuPaint (pgw,  2, 0,  3, 1, "ßßßßßßßßßßßßßßßßßßßßßßßßßß");

   GnuPaintBig (pgw, 3, 2, 3, 70, 0, 0,
      "Please enter the database information below.  See installation notes "
      "for information regarding these settings.");


   GnuPaintBig (pgw, 6, 0, ENTRIES, 17, 1, 0,
       "Vendor:\nDBMS:\n"
       "Client UserName:\nClient Password:\nClient DBString:\n"
       "Batch UserName:\nBatch Password:\nBatch Database:\nBatch DBString:");

   KeyEditCellMode ("\t\x0D", "\x0F\x50\x48", FALSE);

   for (i=0; i<ENTRIES; i++)
      GnuPaint (pgw, i+6, 18, 0, 1, ppszField[i]);

   for (c = 0; c != 'Y' && c != 'y';)  
      {
      ScrShowCursor (TRUE);
      for (uRet=uField=0; uRet != '\x0D'; )
         {
//         KeyEditCellMode (NULL, NULL, uField == 0);
         GnuPaintNChar (pgw, uField+6, 18, 0, (15<<8)|15, ' ', 1);

         uRet = KeyEditCell (ppszField[uField], TopOf(pgw)+uField+7, LeftOf(pgw)+19, FIELDSIZE, 1);
         GnuPaint (pgw, uField+6, 18, 0, 1, ppszField[uField]);
         uLen = strlen (ppszField[uField]);
         GnuPaintNChar (pgw, uField+6, 18 + uLen, 0, 1, ' ', FIELDSIZE-uLen);

         switch (uRet)
            {
            case 0x150: /*--- Dn Arrow ---*/
            case '\t':  /*--- tab      ---*/
               uField = (uField + 1) % ENTRIES;
               break;

            case 0x148: /*--- Up Arrow ---*/
            case 0x10F:
               uField = (uField ? uField-1 : ENTRIES-1);
               break;

            case 0:
               GnuDestroyWin (pgw);
               return FALSE;
            }
         }
      ScrShowCursor (FALSE);

      if (StrBlankLine (ppszField[2]) ||
          StrBlankLine (ppszField[3]) ||
          StrBlankLine (ppszField[5]) ||
          StrBlankLine (ppszField[6]) )
         {
         Beep (0);
         c = 'N';
         }
      else
         {
         GnuPaintNChar (pgw,  16, 2,  3, 1, ' ', 35);
         GnuPaint (pgw, 16, 2, 3, 1, "Is everything correct ? <Y or N>");
         c = KeyChoose ("YyNn", "");
         }
      }
   KeyEditCellMode (NULL, NULL, 2);
   GnuDestroyWin (pgw);
   WriteFile (pszFile);
   return TRUE;
   }


void Quit (void)
   {
   PMET pmet;

// ScrRestoreMode ();
   pmet = ScrGetMetrics ();
   VioScrollDn (0, 0, 0xFFFF, 0xFFFF, 0xFFFF, pmet->bcOriginal, 0);
   VioSetCurPos (0, 0, 0);
   ScrShowCursor (TRUE);
   exit (0);
   }




int _cdecl main (int argc, char *argv[])
   {
   ScrInitMetrics ();

   /*--- cls ---*/
   GnuClearWin (NULL, '\xB0', 0x0A00, FALSE);
   strcpy (szINIFILE, DEFINIFILE);

   PreLoad (szINIFILE);
   MakeIni (szINIFILE);
   Quit ();
   return 0;
   }

