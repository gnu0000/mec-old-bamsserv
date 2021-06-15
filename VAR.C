/*
 *
 * var.c
 * Monday, 1/9/1995.
 *
 * (C) 1995 Info Tech Inc.
 * Craig Fitzgerald
 *
 * Part of the BAMS Job Server
 * This mod implements a method to store name/value lists
 * This module is self contained (with GnuLib)
 */

#define INCL_DOS
#include <os2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <GnuFile.h>
#include <GnuCfg.h>
#include <GnuStr.h>
#include "Var.h"
#include "Bamsserv.h"
#include "Util.h"


PVAR MakeVar (void)
   {
   PVAR pv;

   pv = calloc (1, sizeof (VAR) + 256);  // for debugging
   pv->pszVar = NULL;
   pv->pszVal = NULL;
   pv->next   = NULL;
//   pv->ulTest = 12345678;        // for debug only!
   return pv;
   }



/*
 * gets d var from the var list
 * returns NULL if it aint there
 *
 */
 PSZ VarGet (PVAR pvList, PSZ pszVar, PSZ pszVal)
   {
   PVAR pv;

   StrStrip (StrClip (pszVar, " \t"), " \t");

   for (pv=pvList; pv; pv=pv->next)
      {
      if (stricmp (pszVar, pv->pszVar))
         continue;

      if (!pszVal || !pv->pszVal)
         return pv->pszVal;
      strcpy (pszVal, pv->pszVal);
      return pszVal;
      }
   if (pszVal)
      *pszVal = '\0';
   return NULL;
   }



/*
 * This is an easy way to get a numeric val
 * but there is no way to tell if there was an error
 */
LONG VarGetl (PVAR pvList, PSZ pszVar)
   {
   PVAR pv;

   StrStrip (StrClip (pszVar, " \t"), " \t");

   for (pv=pvList; pv; pv=pv->next)
      {
      if (stricmp (pszVar, pv->pszVar))
         continue;

      return pv->lVal;
      }
   return 0;
   }



/*
 * Retrieves a bool var
 * if the var isnt there or is blank then it's FALSE
 * if the var is 'no' or 'false' or '0' or 'null' then it's FALSE
 * else its TRUE
 */
BOOL VarTrue (PVAR pvList, PSZ pszVar)
   {
   PSZ psz;

   StrStrip (StrClip (pszVar, " \t"), " \t");

   if (!(psz = VarGet (pvList, pszVar, NULL)) || !*psz)
      return FALSE;
//   if (!stricmp (psz, "false") || !stricmp (psz, "no"))
//      return FALSE;
   if (!stricmp (psz, "null")  || psz[0] == '0' || 
                 psz[0] == 'N' || psz[0] == 'n' ||
                 psz[0] == 'F' || psz[0] == 'f')
      return FALSE;
   return TRUE;
   }


/*
 * sets a var in the var list
 * creates a new var if needed
 * changes a value if already present
 */
PSZ VarSet (PVAR *ppvList, PSZ pszVar, PSZ pszVal)
   {
   PVAR pv, pvPrev = NULL;

   StrStrip (StrClip (pszVar, " \t"), " \t");
   StrStrip (StrClip (pszVal, " \t"), " \t");

   /*--- look for existing var ---*/
   for (pv=*ppvList; pv; pv=pv->next)
      {
      if (!stricmp (pszVar, pv->pszVar))
         break;
      pvPrev = pv;
      }

   if (pv)  // var found so free up val
      {
      if (pv->pszVal) 
         free (pv->pszVal);
      pv->pszVal = NULL;
      }
   else      // var not found, add it to list
      {
      pv = MakeVar ();
      pv->pszVar = strdup (pszVar);

      if (pvPrev)
         pvPrev->next = pv;   // add to end
      else
         *ppvList = pv;       // new list
      }
   pv->pszVal = (pszVal ? strdup (pszVal) : NULL);
   pv->lVal   = (pszVal ? atol (pszVal) : 0);
   return pszVal;
   }


/*
 * converts an cfg element (name=value) into
 * its component name and value strings
 *
 */
PSZ NameValue (PSZ pszVar, PSZ pszVal, PSZ pszElement)
   {
   PSZ psz;

   StrGetWord (&pszElement, pszVar, " \t", "\n=", FALSE, FALSE);
   StrEatChar (&pszElement, "=", " \t\n");

   /*--- an inline comment ---*/
   if (psz = strchr (pszElement, ';'))
      *psz = '\0';

   strcpy (pszVal, pszElement);
   StrStrip (pszVal, " \t");
   return pszVar;
   }


/*
 * reads an ini file section
 * this assumes we are already there
 *
 */
static BOOL _VarReadFileSection (PVAR *ppvList, FILE *fp)
   {
   char szLine [256];
   char szVar  [256];
   char szVal  [256];
   PSZ  psz;

   while (FilReadLine (fp, szLine, ";", sizeof (szLine)) != 0xFFFF)
      {
      psz = szLine;

      if (CfgEndOfSection (psz))
         break;

      if (StrBlankLine (psz))
         continue;

      NameValue (szVar, szVal, psz);
      VarSet (ppvList, szVar, szVal);
      }
   return TRUE;
   }


/*
 * this reads a param file
 * the file is like a ini file without [] sections
 *
 */
BOOL VarReadFile (PVAR *ppvList, PSZ pszFile)
   {
   FILE *fp;
   BOOL bRet;

   if (!(fp = fopen (pszFile, "r")))
      return FALSE;
   bRet = _VarReadFileSection (ppvList, fp);
   fclose (fp);
   return bRet;
   }


/*
 * reads a section from an ini file
 *
 *
 */
BOOL VarReadCfg (PVAR *ppvList, PSZ pszFile, PSZ pszSection)
   {
   FILE *fp;
   BOOL bRet;

   if (!(fp = CfgFindSection (pszFile, pszSection)))
      return FALSE;

   bRet = _VarReadFileSection (ppvList, fp);
   fclose (fp);
   return bRet;
   }



/*
 * frees a var list
 *
 *
 */
PVAR FreeVar (PVAR pv)
   {
   PVAR pvNext;

   for (; pv; pv=pvNext)
      {
      pvNext = pv->next;

//      if (pv->ulTest != 12345678)
//         {
//         JMError ("***PVAR has been hosed! Name=%s Val=%s", 
//                  pv->pszVar, pv->pszVal);
//         }
//      JMLog (9, "PV: Freeing pv at: %p", pv);
//      JMLog (9, "PV: pv[%p]->pszVar[%p]=%s", pv, pv->pszVar, pv->pszVar);
//      JMLog (9, "PV: pv[%p]->pszVal[%p]=%s", pv, pv->pszVal, pv->pszVal);
//      JMLog (9, "PV: pv[%p]->lVal=%l", pv, pv->lVal, pv->lVal);
//      JMLog (9, "PV: pv[%p]->next[%p]=%p", pv, pv->next, pv->next);

      if (pv->pszVar) 
         free (pv->pszVar);
      pv->pszVar = NULL;

      if (pv->pszVal) 
         free (pv->pszVal);
      pv->pszVal = NULL;

      free (pv);
      }
   return NULL;
   }


PVAR FreeVar2 (PVAR pv)
   {
   PVAR pvNext;

   for (; pv; pv=pvNext)
      {
      pvNext = pv->next;

      if (pv->pszVar) 
         free (pv->pszVar);
      pv->pszVar = NULL;

      if (pv->pszVal) 
         free (pv->pszVal);
      pv->pszVal = NULL;

      free (pv);
      }
   return NULL;
   }


PVAR FreeVar3 (PVAR pv)
   {
   PVAR pvNext;

   for (; pv; pv=pvNext)
      {
      pvNext = pv->next;

      if (pv->pszVar) 
         free (pv->pszVar);
      pv->pszVar = NULL;

      if (pv->pszVal) 
         free (pv->pszVal);
      pv->pszVal = NULL;

      free (pv);
      }
   return NULL;
   }


PVAR FreeVar4 (PVAR pv)
   {
   PVAR pvNext;

   for (; pv; pv=pvNext)
      {
      pvNext = pv->next;

      if (pv->pszVar) 
         free (pv->pszVar);
      pv->pszVar = NULL;

      if (pv->pszVal) 
         free (pv->pszVal);
      pv->pszVal = NULL;

      free (pv);
      }
   return NULL;
   }


PVAR FreeVar5 (PVAR pv)
   {
   PVAR pvNext;

   for (; pv; pv=pvNext)
      {
      pvNext = pv->next;

      if (pv->pszVar) 
         free (pv->pszVar);
      pv->pszVar = NULL;

      if (pv->pszVal) 
         free (pv->pszVal);
      pv->pszVal = NULL;

      free (pv);
      }
   return NULL;
   }



/*
 * reads a pszDelim delimited string
 * elements are name=value elements
 *
 *
 */
BOOL VarReadStr (PVAR *ppvList, PSZ pszString, PSZ pszDelim)
   {
   char szElement [256];
   char szVar     [256];
   char szVal     [256];

   while (StrGetWord (&pszString, szElement, "", pszDelim, FALSE, TRUE) != 0xFFFF)
      {
      NameValue (szVar, szVal, szElement);
      VarSet (ppvList, szVar, szVal);
      }
   return TRUE;
   }



/*
 * Replaces @vars with thier values within the string
 * @vars that are not matched are kept
 */
PSZ VarResolve (PSZ pszDest, PSZ pszSrc, PVAR pvVars)
   {
   PSZ ps, pd, psz;
   char szVar[128];
   char szVal[256];

   pd = pszDest;
   ps = pszSrc;
   while (*ps)
      {
      if (*ps != '@')
         {
         *pd++ = *ps++;
         continue;
         }

      *szVar = *ps++;   // manually copy over the '@'
      StrGetWord (&ps, szVar+1, "", " ,\\:\t;@", FALSE, FALSE);
      psz = (VarGet (pvVars, szVar+1, szVal) ? szVal : szVar);
      strcpy (pd, psz);
      pd += strlen (psz);
      }
   *pd = '\0';
   return pszDest;
   }


void VarDump (FILE *fp, PVAR pvList)
   {
   for (; pvList; pvList=pvList->next)
      fprintf (fp, "%s=%s\n", pvList->pszVar, pvList->pszVal);
   }
