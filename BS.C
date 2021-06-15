/*
 *
 * BS.c
 * Tuesday, 2/7/1995.
 *
 * (C) 1995 Info Tech Inc.
 * Craig Fitzgerald
 *
 *
 * Part of the BAMS Job Server
 * This is the main file for BS.exe utility
 * This prog simulates client commands that are sent to the server
 */

#define INCL_DOS
#include <os2.h>
#include <stdio.h>
#include <io.h>
#include <stdlib.h>
#include <time.h>
#include <GnuArg.h>
#include <GnuMisc.h>
#include <GnuFile.h>
#include <GnuCfg.h>
#include <GnuStr.h>
#include <GnuRes.h>
#include "Var.h"
#include "BamsServ.h"
#include "Syssock.h"
#include "osock.h"
#include "Service.h"
#include "SokUtil.h"
#include "Respond.h"


LONG lWKP     = 2000;
LONG lSYSTEM  = 1;
BOOL bVERBOSE = FALSE;
BOOL bDEBUG   = FALSE;

#define CONFIGFILE "BS.CFG"


void Usage (void)
   {
   printf ("BS     BAMS Job Monitor Status Facility       %s %s\n\n", __TIME__, __DATE__);
   printf ("USAGE  BS command [options]\n\n");
   printf ("WHERE  command is one of:\n");
   printf ("   ACCEPT...... Accept/Refuse specific commands\n");
   printf ("   COPY........ Copy a file from server to client\n");
   printf ("   DELETEJOB... Delete a job's files\n");
   printf ("   DELETEFILE.. Delete a file from the server\n");
   printf ("   GETINFO..... Get status information\n");
   printf ("   LISTJOBS.... Get Job Listing for a user\n");
   printf ("   LISTFILES... Get a file listing for a User/Job\n");
   printf ("   PING........ Ping a server\n");
   printf ("   PRINT....... Print a file remotely\n");
   printf ("   PURGE....... Purge jobs based on age.  Specific user or all users\n");
   printf ("   SETDEFAULT.. Change a system variable\n");
   printf ("   SHUTDOWN.... Shutdoen server - possibly kill jobs\n");
   printf ("   SUBMIT...... Submit a job\n");
   printf ("   INSTALLFILES Get File List for Bootstrapping\n");
   printf ("   TEST........ Perform system test\n");
   printf ("   TEST2....... Perform system test #2\n");
   printf ("\n");
   printf ("     [options] vary depending on the command.  Here's the full option list:\n");
   printf ("   /Help............This help         /WKP=#...........server port\n");
   printf ("   /Cfg=file........Use this cfg      /Verbose.........verbose mode\n");
   printf ("   /Server=quad.....server address    /Debug...........debug mode\n");
   printf ("   /Generate........make default cfg\n");
   printf ("\n");
   printf ("   /User=str........user name         /Printer=str.....printer name\n");
   printf ("   /ClientID=quad...client address    /Forms=str.......forms string\n");
   printf ("   /ClientPort=#....client port       /Copies=#........copies to print\n");
   printf ("   /JobID=str.......job id            /LocalFile=file..local file name\n");
   printf ("   /JobName=str.....job name          /Var=str.........variable name\n");
   printf ("   /JobTag=str......users job name    /Value=str.......variable value\n");
   printf ("   /JobDesc=str.....job description   /Flags=#.........flags\n");
   printf ("   /ParmFile=file...parameter file    /Enable=#........enable flag\n");
   printf ("   /SubsetFile=file.subset file       /Command=#.......command #\n");
   printf ("   /File=file.......general file      /DateTime=#......date time value\n");
   printf ("   \n");
   printf ("See the config file BS.CFG to see which options are used for each command\n");
   printf ("You can also use the env variable 'BS' to specify options\n");

   exit (0);
   }


/*
 * This fn will re-create the default cfg file.
 * The fn first checks to make sure the cfg doesn't already exist.
 * This fn also requires the resource to be attached to the exe
 */
void GenerateCfg (PSZ pszExeFile)
   {
   char szBuff[256];
   char szPath[256];
   PSZ  psz, pszCfg;
   FILE *fp;

   /*--- Load the cfg file (overrides defaults)---*/
   if (!(psz = strrchr (strcpy (szPath, pszExeFile), '\\')))
      strcpy (szPath, ".");
   else
      *psz = '\0';
   
   if (!access (CONFIGFILE, 0))
      Error ("Config File %s already exists", CONFIGFILE);

   if (!(psz = ResLoadData2 (NULL, NULL, NULL, 0, NULL)))
      Error ("Cannot find resource '%s'", CONFIGFILE);

   if (!(fp = fopen (CONFIGFILE, "w")))
      Error ("Cannot create resource file '%s'", CONFIGFILE);
      
   fprintf (fp, psz, VERSTR, szPath, szPath, szPath, szPath, szPath);
   fclose (fp);
   printf ("Configuration file: %s created.\n", CONFIGFILE);
   exit (0);
   }




PCOMMAND MakeCmd (void)
   {
   PCOMMAND pcmd;

   pcmd = calloc (1, sizeof (COMMAND));

   pcmd->uCommand     = 0;
   pcmd->uPriority    = 1;
   pcmd->pszUserID    = (PSZ) strdup (ArgIs ("User")     ? ArgGet ("User",   0)  : "FakeUser");
   pcmd->pszClientID  = (PSZ) strdup (ArgIs ("ClientID") ? ArgGet ("ClientID", 0): "204.146.114.8");
   pcmd->pszServerID  = (PSZ) strdup (ArgIs ("Server")   ? ArgGet ("Server", 0)  : "204.146.114.8");
   pcmd->pszJobID     = (PSZ) strdup (ArgIs ("JobID")    ? ArgGet ("JobID", 0)   : "00000001");
   pcmd->lMessagePort = (ArgIs ("ClientPort") ? atoi( ArgGet ("ClientPort", 0)) : lWKP);
   pcmd->pszUser1     = NULL;
   pcmd->pszUser2     = NULL;
   pcmd->pszUser3     = NULL;
   pcmd->lUser1       = 0;
   pcmd->lUser2       = 0;
   pcmd->lUser3       = 0;

   return pcmd;
   }



USHORT GetTheStatus (SOCKET sock, PSZ pszCmdType)
   {
   char   szBuff[256];
   long   lBuffLen;
   USHORT uStat;

   if ((lBuffLen = SokBuffRecv (sock, szBuff, sizeof szBuff)) == -1)
      Error ("unable to obtain %s status return\n", pszCmdType);

   uStat = *(PUSHORT)szBuff;
   if (lBuffLen == 2)
      printf ("%s status return=%d\n", pszCmdType, uStat);
   else
      printf ("%s status return=%d Msg=%s\n", pszCmdType, uStat, szBuff+2);

   if (uStat)
      exit (uStat);
   return uStat;
   }


USHORT SendTheStatus (SOCKET sock, USHORT uStat, PSZ pszCmdType)
   {
   char   szBuff[256];
   long   lBuffLen;

   *(PUSHORT)szBuff = uStat;

   if (SokBuffSend(sock, szBuff, sizeof (USHORT)) == -1)
      Error ("Unable to send status val %d from %s", uStat, pszCmdType);
   return uStat;
   }


USHORT SendTheCmd (SOCKET sock, PCOMMAND pcmd)
   {
   USHORT uStat;

   if (bVERBOSE)
      ShowTheCmd (pcmd);

   if (SokSendCmd (sock, pcmd))
      Error ("Socket send command error\n");
   return GetTheStatus (sock, "Send Command");
   }



USHORT SendTheFile (SOCKET sock, PSZ pszFile)
   {
   USHORT uStat, uRet;

   printf ("Sending file %s\n", pszFile);
   if (access (pszFile, 0))
      printf ("By the way, the file %s doesn't exist.\n", pszFile);

   if ((uRet = SokFileSend (sock, pszFile)) != RETURN_OK)
      Error ("SokFileSend error %d\n", uRet);
   return GetTheStatus (sock, "Send File");
   }



USHORT GetTheFile (SOCKET sock, PSZ pszFile)
   {
   USHORT uStat, uRet;

   if (access (pszFile, 0))
      printf ("Creating local file: %s\n", pszFile);
   else
      printf ("over-writing local file: %s\n", pszFile);

   if ((uRet = SokFileRecv (sock, pszFile, ".")) != RETURN_OK)
      Error ("SokFileRecv error %d\n", uRet);
   return SendTheStatus (sock, 0, "Get File");
   }



BOOL SendPINGCmd      (SOCKET sock, PCOMMAND pcmd)
   {
   SendTheCmd (sock, pcmd);
   return TRUE;
   }



BOOL SendSUBMITCmd    (SOCKET sock, PCOMMAND pcmd)
   {
   pcmd->pszUser1 = (PSZ)strdup (ArgIs ("JobName") ? ArgGet ("JobName", 0) : "PESLPRJ");  // JobName
   pcmd->pszUser2 = (PSZ)strdup (ArgIs ("JobTag")  ? ArgGet ("JobTag", 0)  : "PESLPRJTAG" );  // JobTag 
   pcmd->pszUser3 = (PSZ)strdup (ArgIs ("JobDesc") ? ArgGet ("JobDesc", 0) : "desc for PESLPRJ");  // JobDesc
   SendTheCmd (sock, pcmd);
   SendTheFile (sock, ArgIs ("ParmFile")   ? ArgGet ("ParmFile", 0)    : "ParmFile.dat");
   SendTheFile (sock, ArgIs ("SubsetFile")? ArgGet ("SubsetFile", 0) : "Shlsubs.dat");
   return TRUE;
   }



BOOL SendLISTJOBSCmd  (SOCKET sock, PCOMMAND pcmd)
   {
   SendTheCmd (sock, pcmd);
   GetTheFile (sock, ArgIs ("LocalFile") ? ArgGet ("LocalFile", 0) : "JobList.dat");
   return TRUE;
   }



BOOL SendLISTFILESCmd (SOCKET sock, PCOMMAND pcmd)
   {
   SendTheCmd (sock, pcmd);
   GetTheFile (sock, ArgIs ("LocalFile") ? ArgGet ("LocalFile", 0) : "FileList.dat");
   return TRUE;
   }



BOOL SendPRINTCmd     (SOCKET sock, PCOMMAND pcmd)
   {
   pcmd->pszUser1 = (PSZ)strdup (ArgIs ("File"   ) ? ArgGet ("File"   , 0) : "param.dat");  // fileName
   pcmd->pszUser2 = (PSZ)strdup (ArgIs ("Printer") ? ArgGet ("Printer", 0) : "prn");        // prnName
   pcmd->pszUser3 = (PSZ)strdup (ArgIs ("Forms"  ) ? ArgGet ("Forms"  , 0) : "");           // forms

   pcmd->lUser1 = (ArgIs ("Flags")  ? atoi( ArgGet ("Flags", 0)) : lSYSTEM);
   pcmd->lUser2 = (ArgIs ("Copies") ? atoi( ArgGet ("Copiess", 0)) : 1);
   SendTheCmd (sock, pcmd);
   return TRUE;
   }



BOOL SendCOPYCmd      (SOCKET sock, PCOMMAND pcmd)
   {
   pcmd->pszUser1 = (PSZ)strdup (ArgIs ("File"   )   ? ArgGet ("File"   , 0)     : "param.dat");    // filename
   pcmd->pszUser2 = (PSZ)strdup (ArgIs ("LocalFile") ? ArgGet ("LocalFile", 0)   : pcmd->pszUser1); // localname
   pcmd->lUser1 =   (ArgIs ("Flags")  ? atoi( ArgGet ("Flags", 0)) : lSYSTEM);
   SendTheCmd (sock, pcmd);

   GetTheFile (sock, ArgIs ("LocalFile") ? ArgGet ("LocalFile", 0) : "CopyFile.dat");
   return TRUE;
   }



BOOL SendDELETEJOBCmd (SOCKET sock, PCOMMAND pcmd)
   {
   SendTheCmd (sock, pcmd);
   return TRUE;
   }



BOOL SendDELETEFILECmd(SOCKET sock, PCOMMAND pcmd)
   {
   pcmd->pszUser1 = (PSZ)strdup (ArgIs ("File"   ) ? ArgGet ("File" , 0) : "param.dat");    // JobName
   pcmd->lUser1 =   (ArgIs ("Flags")  ? atoi( ArgGet ("Flags", 0)) : lSYSTEM);
   SendTheCmd (sock, pcmd);
   return TRUE;
   }


BOOL SendACCEPTCmd    (SOCKET sock, PCOMMAND pcmd)   // admin cmd
   {
   pcmd->lUser1 = (ArgIs ("Command") ? atoi( ArgGet ("Command", 0)) : 1);
   pcmd->lUser2 = (ArgIs ("Enable")  ? atoi( ArgGet ("Enable",  0)) : 1);
   SendTheCmd (sock, pcmd);
   return TRUE;
   }

BOOL SendPURGECmd     (SOCKET sock, PCOMMAND pcmd)   // admin cmd
   {
   pcmd->lUser1 = (ArgIs ("Flags")    ? atoi( ArgGet ("Flags", 0)) : 0);
   pcmd->lUser2 = (ArgIs ("DateTime") ? atoi( ArgGet ("DateTime", 0)) : 72);
   SendTheCmd (sock, pcmd);
   return TRUE;
   }


BOOL SendSETDEFAULTCmd(SOCKET sock, PCOMMAND pcmd)   // admin cmd
   {
   pcmd->pszUser1 = (PSZ)strdup (ArgIs ("Var"   )  ? ArgGet ("Var"  , 0)   : "ServerPerformance"); // var name
   pcmd->pszUser2 = (PSZ)strdup (ArgIs ("Value")   ? ArgGet ("Value", 0)   : "200"              ); // var value
   SendTheCmd (sock, pcmd);
   return TRUE;
   }

BOOL SendGETINFOCmd   (SOCKET sock, PCOMMAND pcmd)   // admin cmd
   {
   pcmd->lUser1 = (ArgIs ("Flags") ? atoi( ArgGet ("Flags", 0)) : 0xFFFFFFFF);
   SendTheCmd (sock, pcmd);
   GetTheFile (sock, ArgIs ("LocalFile") ? ArgGet ("LocalFile", 0) : "Stat.dat");
   return TRUE;
   }

BOOL SendSHUTDOWNCmd  (SOCKET sock, PCOMMAND pcmd)   // admin cmd
   {
   pcmd->lUser1 = (ArgIs ("Flags") ? atoi( ArgGet ("Flags", 0)) : 0xFFFFFFFF);
   SendTheCmd (sock, pcmd);
   return TRUE;
   }


BOOL SendINSTALLFILESCmd (SOCKET sock, PCOMMAND pcmd)
   {
   SendTheCmd (sock, pcmd);
   return TRUE;
   }


BOOL SendTESTCmd      (SOCKET sock, PCOMMAND pcmd)
   {
   SendTheCmd (sock, pcmd);
   return TRUE;
   }


BOOL SendTEST2Cmd     (SOCKET sock, PCOMMAND pcmd)   // admin cmd
   {
   SendTheCmd (sock, pcmd);
   return TRUE;
   }



BOOL DoCmd (SOCKET sock, USHORT uCmd)
   {
   PCOMMAND pcmd;

   pcmd = MakeCmd ();
   pcmd->uCommand = uCmd;

   switch (uCmd)
      {
      case JSC_PING         : return SendPINGCmd          (sock, pcmd); break; 
      case JSC_SUBMIT       : return SendSUBMITCmd        (sock, pcmd); break; 
      case JSC_LISTJOBS     : return SendLISTJOBSCmd      (sock, pcmd); break; 
      case JSC_LISTFILES    : return SendLISTFILESCmd     (sock, pcmd); break; 
      case JSC_PRINT        : return SendPRINTCmd         (sock, pcmd); break; 
      case JSC_COPY         : return SendCOPYCmd          (sock, pcmd); break; 
      case JSC_DELETEJOB    : return SendDELETEJOBCmd     (sock, pcmd); break; 
      case JSC_DELETEFILE   : return SendDELETEFILECmd    (sock, pcmd); break; 
      case JSC_ACCEPT       : return SendACCEPTCmd        (sock, pcmd); break; 
      case JSC_PURGE        : return SendPURGECmd         (sock, pcmd); break; 
      case JSC_SETDEFAULT   : return SendSETDEFAULTCmd    (sock, pcmd); break; 
      case JSC_GETINFO      : return SendGETINFOCmd       (sock, pcmd); break; 
      case JSC_SHUTDOWN     : return SendSHUTDOWNCmd      (sock, pcmd); break; 
      case JSC_INSTALLFILES : return SendINSTALLFILESCmd  (sock, pcmd); break; 
      case JSC_TEST         : return SendTESTCmd          (sock, pcmd); break; 
      case JSC_TEST2        : return SendTEST2Cmd         (sock, pcmd); break; 
      }
   Error ("Unknown Command %s", ArgGet ("Cmd", 0));
   }


BOOL HandleSecurity (SOCKET sock)
   {
   char szSrc  [128];
   char szDest [128];
   LONG lBuffLen;
   BOOL bOK;

   if ((lBuffLen = SokBuffRecv (sock, szSrc, sizeof szSrc)) == -1)
      Error ("unable to obtain Security request string\n");

   if (lBuffLen < 8)
      Error ("Invalid Security request string\n");

   Respond (szDest, szSrc);

   if (bVERBOSE)
      {
      printf ("Security In : '%s'\n",szSrc);
      printf ("Security Out: '%s'\n",szDest);
      }

   if (SokBuffSend(sock, szDest, strlen (szDest) + 1) == -1)
      Error ("Cannot send security string to client!");

   if ((lBuffLen = SokBuffRecv (sock, szSrc, sizeof szSrc)) == -1)
      Error ("unable to obtain Security  Status from server\n");

   return (BOOL)*((PUSHORT)szSrc);
   }


void ReadSection (FILE *fp) 
   {
   char szLine [512];
   PSZ  psz, pszTmp;

   while (FilReadLine (fp, szLine, ";", sizeof (szLine)) != 0xFFFF)
      {
      psz = szLine;
      if (CfgEndOfSection (psz))
         break;

      if (StrBlankLine (psz))
         continue;

      if (pszTmp = strchr (psz, ';'))
         *pszTmp = '\0';

      if (ArgFillBlk2 (psz))
         printf ("BS CFG FILE ERROR: %s : %s", psz, ArgGetErr ());
      }
   fclose (fp);
   }


void ReadCfgFile (PSZ pszCfg, USHORT uCmd) 
   {
   FILE *fp;
   PSZ  pszCmd;

   pszCfg = (pszCfg ? pszCfg : CONFIGFILE);

   if (access (pszCfg, 0))  // does file exist?
      return;
   if (fp = CfgFindSection (pszCfg, "COMMON"))
      ReadSection (fp);

   pszCmd = "JSC_PING";
   switch (uCmd)
      {
      case JSC_PING         : pszCmd = "JSC_PING";         break; 
      case JSC_SUBMIT       : pszCmd = "JSC_SUBMIT";       break; 
      case JSC_LISTJOBS     : pszCmd = "JSC_LISTJOBS";     break; 
      case JSC_LISTFILES    : pszCmd = "JSC_LISTFILES";    break; 
      case JSC_PRINT        : pszCmd = "JSC_PRINT";        break; 
      case JSC_COPY         : pszCmd = "JSC_COPY";         break; 
      case JSC_DELETEJOB    : pszCmd = "JSC_DELETEJOB";    break; 
      case JSC_DELETEFILE   : pszCmd = "JSC_DELETEFILE";   break; 
      case JSC_ACCEPT       : pszCmd = "JSC_ACCEPT";       break; 
      case JSC_PURGE        : pszCmd = "JSC_PURGE";        break; 
      case JSC_SETDEFAULT   : pszCmd = "JSC_SETDEFAULT";   break; 
      case JSC_GETINFO      : pszCmd = "JSC_GETINFO";      break; 
      case JSC_SHUTDOWN     : pszCmd = "JSC_SHUTDOWN";     break; 
      case JSC_INSTALLFILES : pszCmd = "JSC_INSTALLFILES"; break; 
      case JSC_TEST         : pszCmd = "JSC_TEST";         break; 
      case JSC_TEST2        : pszCmd = "JSC_TEST2";        break; 
      }
   if (fp = CfgFindSection (pszCfg, pszCmd))
      ReadSection (fp);
   }


USHORT GetCmd (PSZ pszCmd)
   {
   USHORT uCmd, uLen, uMatch;

   if (!pszCmd)
      return 0;
   if (uCmd = atoi (pszCmd))
      return uCmd;
   if (!strnicmp (pszCmd, "JM_", 3))
      pszCmd += 3;

   uLen = strlen (pszCmd);
   uMatch = uCmd = 0;
   if (!strnicmp (pszCmd, "ACCEPT"      , uLen)) uCmd= JSC_ACCEPT      , uMatch++;
   if (!strnicmp (pszCmd, "COPY"        , uLen)) uCmd= JSC_COPY        , uMatch++;
   if (!strnicmp (pszCmd, "DELETEJOB"   , uLen)) uCmd= JSC_DELETEJOB   , uMatch++;
   if (!strnicmp (pszCmd, "DELETEFILE"  , uLen)) uCmd= JSC_DELETEFILE  , uMatch++;
   if (!strnicmp (pszCmd, "GETINFO"     , uLen)) uCmd= JSC_GETINFO     , uMatch++;
   if (!strnicmp (pszCmd, "INSTALLFILES", uLen)) uCmd= JSC_INSTALLFILES, uMatch++;
   if (!strnicmp (pszCmd, "LISTJOBS"    , uLen)) uCmd= JSC_LISTJOBS    , uMatch++;
   if (!strnicmp (pszCmd, "LISTFILES"   , uLen)) uCmd= JSC_LISTFILES   , uMatch++;
   if (!strnicmp (pszCmd, "PING"        , uLen)) uCmd= JSC_PING        , uMatch++;
   if (!strnicmp (pszCmd, "PRINT"       , uLen)) uCmd= JSC_PRINT       , uMatch++;
   if (!strnicmp (pszCmd, "PURGE"       , uLen)) uCmd= JSC_PURGE       , uMatch++;
   if (!strnicmp (pszCmd, "SETDEFAULT"  , uLen)) uCmd= JSC_SETDEFAULT  , uMatch++;
   if (!strnicmp (pszCmd, "SHUTDOWN"    , uLen)) uCmd= JSC_SHUTDOWN    , uMatch++;
   if (!strnicmp (pszCmd, "SUBMIT"      , uLen)) uCmd= JSC_SUBMIT      , uMatch++;
   if (!strnicmp (pszCmd, "TEST"        , uLen)) uCmd= JSC_TEST        , uMatch++;
   if (!strnicmp (pszCmd, "TEST2"       , uLen)) uCmd= JSC_TEST2       , uMatch++;

   /*--- a few extras ---*/
   if (!strnicmp (pszCmd, "STATUS"   , uLen)) uCmd= JSC_GETINFO   , uMatch++;
   if (!strnicmp (pszCmd, "KILL"     , uLen)) uCmd= JSC_SHUTDOWN  , uMatch++;
                                                                 
   if (!uMatch)
      Error ("Unknown Command: %s", pszCmd);
   if (uMatch > 1)
      Error ("Ambiguous Command: %s", pszCmd);
   return uCmd;
   }



int main (int argc, char *argv[])
   {
   SOCKET  sock;
   char    szBuff [1024];
   COMMAND cmd;
   USHORT  uRet, uCmd;
   PSZ     pszAddr;

   if (ArgBuildBlk (
         "? *^Help *^Cfg% *^Verbose *^Debug *^Generate "
         "*^Server% *^WKP% "
         "*^User% *^ClientID% *^ClientPort% "
         "*^JobID% *^JobName% *^JobTag% *^JobDesc% "
         "*^ParmFile% *^SubsetFile% "
         "*^File% *^Printer% *^Forms% *^Copies% *^LocalFile% "
         "*^Var% *^Value% "
         "*^Flags% *^Enable% *^Command% *^DateTime%"))
      Error ("%s", ArgGetErr ());
      
   if (ArgFillBlk (argv))
      Error ("%s", ArgGetErr ());

   if (ArgFillBlk2 (getenv ("BS")))
      return printf ("BS ENV ERROR: %s", ArgGetErr ());

   if (ArgIs ("Generate"))
      GenerateCfg (argv[0]);

   if (ArgIs ("Help") || ArgIs ("?") || argc == 1 || !ArgIs (NULL))
      Usage ();

   uCmd = GetCmd (ArgGet (NULL, 0));
   bVERBOSE = ArgIs ("Verbose");
   bDEBUG   = ArgIs ("Debug");

   ReadCfgFile (ArgGet ("Cfg", 0), uCmd);

   if (bDEBUG)
      ArgDump ();

   if (ArgIs ("WKP"))
      lWKP = (LONG) atoi (ArgGet ("WKP", 0));

   if ((sock = SokCreateClient (TCP)) == -1)
      Error ("Unable to create client socket");

   pszAddr = (ArgIs("Server") ? ArgGet("Server", 0) : "204.146.114.8");

   printf ("Establishing connection...\n");
   if (SokConnectTo (sock, pszAddr, lWKP) == -1)
      Error ("Unable to connect to server at %s\n", pszAddr);
   if (bVERBOSE)
      printf ("sock=%d connected to %s at port %d\n", sock, pszAddr, lWKP);

   printf ("Negotiating security...\n");
   if (!HandleSecurity (sock))
      Error ("Security failed\n", pszAddr);

   DoCmd (sock, uCmd);
   }

