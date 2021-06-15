/*
 * ReadEBL.h
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


#define EXT           ".EBL"
#define LIBVER        100
#define LIBHEADER     "This is an EBS library file.\n\x1A"
#define HEADERSIZE    30

#define EBLMARK       0x21554E47UL
#define EBLMARKSIZE   4

#define FILELENOFFSET HEADERSIZE + 4
#define SIZEOFFSET    12

#define STORE         0
#define HOSE          1
#define UNHOSE        -1


/*
 * Library Volume Information
 */
typedef struct
   {
   FILE   *fp;        // Lib File Handle
   ULONG  ulOffset;   // offset into Header of first file
   ULONG  ulSize;     // Size of Lib File
   USHORT uCount;     // count of proposal files in lib file
   USHORT uLibVer;    // Version of lib file
   PSZ    pszDesc;    // Description of file
   } LDESC;
typedef LDESC *PLDESC;


/*
 * File Entry Header Information
 */
typedef struct _fd
   {
   ULONG  ulOffset;     // Starting Position in File data relative to file
   ULONG  ulLen;        // Size of uncompressed file data
   ULONG  ulSize;       // Size of compressed file data
   ULONG  ulCRC;        // CRC check of file data
   USHORT uMethod;      // Compression method
   FDATE  fDate;        // Date of file
   FTIME  fTime;        // Time of file
   USHORT uAtt;
   char   szName [256]; // Name of file
   char   szDesc [256]; // Description of proposal file

   /*--- The following are not kept in the file ---*/
   PLDESC pld;          // ptr to owning lib Volume info
   USHORT uMode;        // processing mode
   struct _fd *Next;    // used when building processing chains
   } FDESC;
typedef FDESC *PFDESC;


extern USHORT uLIBERR;
extern PSZ    szLIBERR;

extern char szPath [];  // used by EbOpen
extern char szLib  [];  // used by EbOpen
extern char szFile [];  // used by EbOpen
extern PFDESC PFD;      // used by EbOpen
extern PLDESC PLD;      // used by EbOpen


/*
 * [path][lib][:][file]
 * path: c: c:\ c:\dir\dir\ \dir\ dir\ <none>
 * lib   lib.exe lib
 *
 * c:\libs\liba.ebl:file7.txt
 * c:liba.ebl:file7.txt
 */
FILE *EbOpen (PSZ pszFile, PSZ pszMode);

PFDESC GetNextFile (PLDESC pld, PFDESC pfdPrev, BOOL bFreeOld);

PFDESC ReadFileInfo (PLDESC pld, BOOL bSkipData);

PLDESC OpenLib (PSZ pszLib);

PVOID FreePFD (PFDESC pfd);

PVOID FreePLD (PLDESC pld);

PSZ DateStr (FDATE fDate);

PSZ TimeStr (FTIME fTime);

PSZ AttStr (USHORT uAtt);

ULONG Ratio (ULONG ulSmall, ULONG ulBig);


//int FilReadShort (FILE *fp);
//ULONG FilReadLong (FILE *fp);
//PSZ FilReadStr (FILE *fp, PSZ psz);
BOOL ReadMark (FILE *fp);
PVOID SetLibErr (USHORT i);
void SkipFileData (PFDESC pfd);


