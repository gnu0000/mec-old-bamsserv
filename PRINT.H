/*
 *
 * print.h
 * Thursday, 4/6/1995.
 *
 */


/*
 * spools a file to the spool file
 *
 */
static BOOL SpoolFile (FILE *fpSpool, PSZ pszDir, PSZ pszFile);


/*
 * Writes a file header to the output file
 *
 */
void IdentifyFile (FILE *fp, PSZ pszFile);


/*
 * Prints a job to the printer
 *
 */
BOOL PrintJob (PJOB pjob, BOOL bDebug);


/*
 * Prints a file to the printer
 *
 */
USHORT PrintFile (PSZ    pszFile, 
                  PSZ    pszPrinter, 
                  PSZ    pszServer, 
                  PSZ    pszForms, 
                  USHORT uCopies, 
                  BOOL   bDeleteFile);


/*
 *
 *
 *
 */
BOOL WritePrinterCodes (PSZ pszPrinter, PSZ pszForms, FILE *fp, BOOL bInit);

