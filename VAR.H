/*
 *
 * Var.h
 * Monday, 1/9/1995.
 *
 * (C) 1995 Info Tech Inc.
 * Craig Fitzgerald
 *
 * Part of the BAMS Job Server
 * This mod implements a method to store name/value lists
 * This module is self contained (with GnuLib)
 */

#define VAR_H

typedef struct _vars
   {
   PSZ          pszVar;
   PSZ          pszVal;
   LONG         lVal;
   struct _vars *next;
   } VAR;
typedef VAR *PVAR;


/*
 * sets a var in the var list
 * creates a new var if needed
 * changes a value if already present
 */
PSZ VarSet (PVAR *ppvList, PSZ pszVar, PSZ pszVal);


/*
 * gets d var from the var list
 * returns NULL if it aint there
 *
 */
PSZ VarGet (PVAR pvList, PSZ pszVar, PSZ pszVal);


/*
 * This is an easy way to get a numeric val
 * but there is no way to tell if there was an error
 */
LONG VarGetl (PVAR pvList, PSZ pszVar);


/*
 * Retrieves a bool var
 * if the var isnt there or is blank then it's FALSE
 * if the var is 'no' or 'false' or '0' or 'null' then it's FALSE
 * else its TRUE
 */
BOOL VarTrue (PVAR pvList, PSZ pszVar);



/*
 * frees a var list
 */
PVAR FreeVar (PVAR pv);
PVAR FreeVar2 (PVAR pv);
PVAR FreeVar3 (PVAR pv);
PVAR FreeVar4 (PVAR pv);
PVAR FreeVar5 (PVAR pv);


/*
 * this reads a param file
 * the file is like a ini file without [] sections
 */
BOOL VarReadFile (PVAR *ppvList, PSZ pszFile);


/*
 * reads a section from an ini file
 *
 */
BOOL VarReadCfg (PVAR *ppvList, PSZ pszFile, PSZ pszSection);


/*
 * reads a pszDelim delimited string
 * elements are name=value elements
 */
BOOL VarReadStr (PVAR *ppvList, PSZ pszString, PSZ pszDelim);


/*
 * replaces @vars with thier values within the string
 */
PSZ VarResolve (PSZ pszDest, PSZ pszSrc, PVAR pvVars);


/*
 * for debug: dumps vars to fp
 *
 */
void VarDump (FILE *fp, PVAR pvList);

