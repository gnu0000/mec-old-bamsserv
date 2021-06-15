/*
 * GnuIBM.h
 *
 * IBM No longer includes these in their header files
 *
 */

#if !defined (DosSemClear)

   /*
    * 16-bit Semaphore support
    * added by clf 2/3/95
    *
    */
   #define DosSemClear     DOS16SEMCLEAR  
   #define DosSemSet       DOS16SEMSET    
   #define DosSemWait      DOS16SEMWAIT   
   #define DosSemSetWait   DOS16SEMSETWAIT
   #define DosSemRequest   DOS16SEMREQUEST

   APIRET16 APIENTRY16 DosSemClear(HSEM hsem);
   APIRET16 APIENTRY16 DosSemSet(HSEM hsem);
   APIRET16 APIENTRY16 DosSemWait(HSEM hsem, LONG lTimeOut);
   APIRET16 APIENTRY16 DosSemSetWait(HSEM hsem, LONG lTimeOut);
   APIRET16 APIENTRY16 DosSemRequest(HSEM hsem, LONG lTimeOut);

#endif
