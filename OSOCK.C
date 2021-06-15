/*
 *   16 Jan 95
 *   Preliminary Version 2.6
 *
 *   This is the code for the OS/2 socket library.
 *
 *
*/

#ifndef BSD_SELECT
#define BSD_SELECT
#endif

#include "syssock.h"
#include "osock.h"

#ifndef OSOCK

#define OSOCK
#define TCP 1
#define UDP 0
#define LISTEN_QUEUE 5

#define BUFF_SIZE   1024
#define RETURN_MSG  128

#define DONE 1
#define HANDSHAKE_OK 1
#define NAK  0

#define RETURN_OK             0
#define RETURN_NET_ERROR      1
#define RETURN_DISK_ERROR     2
#define RETURN_MEMORY_ERROR   3
#define RETURN_GENERAL_ERROR  4

#define SOCKET long

#endif


/*
 *
 *        General notes :
 *          1. Unless Otherwise specified, all functions use a standard
 *             integer return code where 0 means success and -1 means
 *             failure.
 *
 *          2. Input parameter types are designed to allow the maximum
 *             ability to hide information from the the user. Some changes
 *             may be needed.
 *
 *       
 */

/****************************************************************************/


/* 
 *   This function will create a socket and bind it to the address passed in 
 *   port_num. The function will bind the socket for all interfaces that
 *   exist on the machine.
 *
 *   Input parameters are as follows:
 *      lPortNum  The port number to associate with this socket.
 *      lSocketType The type of socket to be created, either TCP(stream) or UDP 
 *                  (datagram).
 *
 *   Returned parameters are as follows:
 *      function value the newly created socket.
 *
*/

SOCKET SokCreateServer(long lPortNum, long lSocketType)

   {
   SOCKET sktNewSock;
   int  iRetCode, iProto, iLength, iDomain;
   struct sockaddr_in saName;
   
   iDomain = AF_INET;

   if (lSocketType == TCP)
      {
      lSocketType = SOCK_STREAM;
      iProto = IPPROTO_TCP;
      } 
   else if (lSocketType == UDP)
      {
      lSocketType = SOCK_DGRAM;
      iProto = IPPROTO_UDP;
      }
   else 
      {  /* Error Condition */
         return(-1);
      }
   
   sktNewSock = socket(iDomain, lSocketType, iProto);
   if (sktNewSock == -1) 
      {    /* Error Condition */
      return(-1);
      }

   memset (&saName, 0, sizeof(saName));
   saName.sin_family = AF_INET;
   saName.sin_addr.s_addr = INADDR_ANY;
   saName.sin_port = SokHostToNetShort((unsigned short)lPortNum);
   iRetCode = bind (sktNewSock, (struct sockaddr *) &saName, (sizeof(saName)));
   if (iRetCode == -1)
      {     /* Error Condition */
         return(-1);
      }
   
   iRetCode = listen(sktNewSock, LISTEN_QUEUE);
   if (iRetCode == -1)
      {     /* Error Condition */
             return(-1);
      }

   return (sktNewSock);

   } /* end CreateServerSocket */

   
   
/****************************************************************************/


/* 
 *   This function will create a socket to be used on the client machine.
 *
 *   Input parameters are as follows:
 *      lSocketType The type of socket to be created, either TCP(stream) or UDP
 *                  (datagram).
 *
 *   Returned parameters are as follows:
 *      function value the newly created socket.
 *
*/

SOCKET SokCreateClient(long lSocketType)

   {
   SOCKET sktNewSock;
   int iDomain, iRetCode, iProto;
   struct sockaddr_in saName;

   iDomain = AF_INET;
   if (lSocketType == TCP)
      {
      lSocketType = SOCK_STREAM;
      iProto = IPPROTO_TCP;
      } 
   else if (lSocketType == UDP)
      {
      lSocketType = SOCK_DGRAM;
      iProto = IPPROTO_UDP;
      }
   else 
      {  /* Error Condition */
         return(-1);
      }

   sktNewSock = socket(iDomain, lSocketType, iProto);
   if (sktNewSock == -1)
      {  /* Error Condition */
          return(-1);
      }

/*
 *   memset(&saName, 0, sizeof(saName));
 *   saName.sin_family = iDomain;
 *   saName.sin_addr.s_addr = INADDR_ANY;
 *   saName.sin_port = SokHostToNetShort(0);
 *   iRetCode = bind(sktNewSock, (struct sockaddr *) &saName, sizeof(saName));
 *   if (iRetCode == -1)
 *      { 
 *         return(iRetCode);
 *      }
*/

   return (sktNewSock);

   }


/****************************************************************************/


/* 
 *   This function will establish a connection to the specified machine.
 *
 *   Input parameters are as follows:
 *      sSocket - the local socket to use.
 *      pszTarget - the machine to connect to, this will be an IP address in 
 *                  dotted quad notation.
 *      lPortNum - the target port to connect to.
 *
 *   Returned parameters are as follows:
 *      Standard return codes used.
 *
*/

long    SokConnectTo(SOCKET sSocket, char * pszTarget, unsigned long lPortNum)

   {
   struct sockaddr_in saName;
   unsigned long  lAddress;
   int    iRetCode;

   lAddress = inet_addr(pszTarget);

   memset(&saName, 0, sizeof(saName));
   saName.sin_family = AF_INET;
   saName.sin_addr.s_addr = lAddress;
   saName.sin_port = SokHostToNetShort((unsigned short)lPortNum);
   iRetCode = connect(sSocket, (struct sockaddr *) &saName, sizeof(saName));
   if (iRetCode == -1)
      {   /* Error Condition */
      }

   return (iRetCode);

   }



/****************************************************************************/


/* 
 *This function will close any connections on the socket and disconnect them.
 *
 *   Input parameters are as follows :
 *      sSocket - local socket to disconnect.
 *
 *   Returned parameters are as follows:
 *      Standard return codes used.
 *
*/

long    SokDisconnect(SOCKET sSocket)

   {
   int iHow = 2;
   int iRetCode;

   iRetCode = shutdown(sSocket, iHow);
   if (iRetCode == -1)
      {  /* Error Condition */
      }

   iRetCode = soclose(sSocket);
   if (iRetCode == -1)
      {  /* Error Condition */
      }

   return (iRetCode);

   }

  

/******************************************************************************/


/* 
 *   This function will send a buffer using the given socket. 
 *
 *   Input parameters are as follows:
 *      sSocket - local socket to use.
 *      pBuff - pointer to the area that the buffer resides in.
 *      lLength - the length of the buffer to be sent.
 *
 *   Returned parameters are as follows:
 *      function value  -1 indicates error, any other value indicates number 
 *                      of characters sent.
 *
*/

long    SokBuffSend(SOCKET sSocket, void * pBuff, long lLength)
	{
	void *pSendBuff, *pTemp;
	unsigned long ulLoop;
	int iRc;

   /*
    *  This section of the function was changed to attempt to eliminate the
    *  transmission and reception of "garbage" characters when the socket
    *  does not actually transmit/receive all the data requested in a 
    *  single cycle. Let's hope it works.
    */

	if ((pSendBuff = malloc (lLength + sizeof (unsigned long))) == NULL)
		return 0;

	pTemp = pSendBuff;
	*(unsigned long *)pSendBuff = SokHostToNetLong (lLength);
	pTemp = (unsigned long *)pTemp + 1;
	memcpy (pTemp, pBuff, (size_t)lLength);

	for (ulLoop = 0; ulLoop < lLength + sizeof (unsigned long);
			  ulLoop += iRc)
		{
		iRc = send(sSocket, (char far *)pSendBuff + ulLoop,
						  lLength + sizeof (unsigned long) - ulLoop, 0);
		if (iRc == -1)
			{
			free (pSendBuff);
			return -1;
			}
		}

	free (pSendBuff);
	if (ulLoop != lLength + sizeof (unsigned long))
		return -2;
	else
		return lLength;
	}





/******************************************************************************/


/* 
 *   This function will send a message using the given socket. 
 *   
 *   Input parameters are as follows:
 *      sSocket - local socket to use.
 *      pMsg - pointer to the area that the message resides in.
 *      pszTarget - pointer to the character string containing the name of 
 *               the target machine.
 *
 *   Returned parameters are as follows:
 *      function value  -1 indicates error, any other value indicates number 
 *                      of characters sent.
 *
*/

long SokDgramSend(SOCKET sSocket, void * pMsg, char * pszTarget, long lPortNum)

   {
   int iRetCode, iToLen;
   long lLength, lFlags;
   struct sockaddr_in saTarget;



   saTarget.sin_family = AF_INET;


//   saTarget.sin_addr.s_addr = SokNameToIP(pszTarget);

//
//
// this is a test -clf
//
   saTarget.sin_addr.s_addr = inet_addr(pszTarget);


   saTarget.sin_port = SokHostToNetLong(lPortNum);

   lLength = strlen (pMsg);
   lFlags = 0;
   iToLen = sizeof saTarget;
   iRetCode = sendto (sSocket, pMsg, lLength, lFlags,(struct sockaddr *) &saTarget, iToLen);
   if (iRetCode == -1)
      {   /* Error Condition */
      }

   return iRetCode;

   }




/******************************************************************************/


/* 
 *   This function will receive a buffer using the given socket. 
 *
 *   Input parameters are as follows:
 *      sSocket - local socket to use.
 *      pBuff - pointer to the buffer to place the incoming data.
 *      lLength - the maximum length of the buffer to be received.
 *
 *   Returned parameters are as follows:
 *      function value - The length of the buffer received, in bytes. -1 is
 *                       returned if there is an error. 0 is returned if the
 *                       connection has been closed.
 *      pBuff- Holds the data that was received from the socket. Up to the length
 *           indicated by the length input parameter.
 *
*/

long    SokBuffRecv(SOCKET sSocket, void * pBuff, long lLength)
	{
	void *pRecvBuff, *pTemp;
	BOOL bLoop, bSize;
	int iRc, iTot, iSentLen;

   /*
    *  This section of the function was changed to attempt to eliminate the
    *  transmission and reception of "garbage" characters when the socket
    *  does not actually transmit/receive all the data requested in a 
    *  single cycle. Let's hope it works.
    */

	if ((pRecvBuff = malloc (lLength + sizeof (unsigned long))) == NULL)
		return -2;

	iTot = 0;
	iSentLen = 0;
	bSize = FALSE;
	bLoop = TRUE;

   /*
    *  Get all of the up to the expected size.
    */ 

	while (bLoop)
		{
		iRc = recv (sSocket, (char far *)pRecvBuff + iTot, lLength + sizeof (unsigned long) - iTot, 0);

                if (iRc != -1)
                   {
                   iTot += iRc;
		   if (iTot >= sizeof (unsigned long) && !bSize)
			   {
			   iSentLen = (int) SokNetToHostLong(*(unsigned long *)pRecvBuff);
			   bSize = TRUE;
			   if (iSentLen > lLength)
				   return -3;
			   }

		   if (iTot >= iSentLen + sizeof (unsigned long))
			   bLoop = FALSE;
		   }
                }
	pTemp = (unsigned long *)pRecvBuff + 1;
	memcpy (pBuff, pTemp, (size_t)iSentLen);
	free (pRecvBuff);
	return iSentLen;
	}





/******************************************************************************/


/* 
 *   This function will receive a message using the given socket. 
 *
 *   Input parameters are as follows:
 *      sSocket - local socket to use.
 *      pMsg - pointer to the message to place the incoming data.
 *      lLength - the maximum length of the message to be received.
 *      saTarget - point to a sockaddr structure containing information about
 *               the target machine.
 *
 *
 *   Returned parameters are as follows:
 *      function value - The length of the message received, in bytes. -1 is
 *                       returned if there is an error. 0 is returned if the
 *                       connection has been closed.
 *      pMsg - Holds the data that was received from the socket. Up to the length
 *            indicated by the length input parameter.
 *
*/

long    SokDgramRecv(SOCKET sSocket, void * pMsg, long lLength, char * pszTarget)

   {
      int iRetCode, iSize;
      struct sockaddr_in  saIncoming;

      iSize = sizeof(saIncoming);
      iRetCode = recvfrom(sSocket, pMsg, lLength, 0,(struct sockaddr *) &saIncoming, &iSize);
      if (iRetCode == -1)
         {    /*  Error Condition */
         }

      return (iRetCode);

   }


/******************************************************************************/


/* 
 *   This function will initialize the socket subsystems and prepare them for
 *   use by the application.
 *
 *   Input parameters are as follows:
 *      None.
 *
 *   Returned parameters are as follows:
 *      Standard return codes are used.
 *
*/

long   SokInit()

   {
   int iRetCode;

   iRetCode = sock_init();
   if (iRetCode == 1) 
      {
      iRetCode == -1;
      }

   return(iRetCode);

   }


/******************************************************************************/


/* 
 *   This function will translate a network address in the Internet style(IP)
 *   dotted quad form to the corresponding network representation.
 *
 *   Input parameters are as follows:
 *      pszDottedAddr  a character string containing the address to be translated.
 *
 *   Returned parameters are as follows:
 *      function value  the address in network form.
 *
*/

unsigned long  SokNetworkAddr (char * pszDottedAddr)

   {
   unsigned long lNetAddr;

   lNetAddr = inet_addr(pszDottedAddr);
   return(lNetAddr);
   }


/******************************************************************************/


/* 
 *   This function will translate a character string containing the name of a
 *   network host to its corresponding address in network form.
 *
 *   Input parameters are as follows:
 *      pszName  a character string containing the name for which to find an address.
 *
 *   Returned parameters are as follows:
 *      function value  the address of the host in network form.
 *
*/

ULONG SokNameToIP (PSZ pszName)
   {
   struct hostent *pheHost;
   
   if (!(pheHost = gethostbyname(pszName)))
      return -1;

   return (pheHost->h_addr ? *(PULONG)pheHost->h_addr : -1);
   }


//unsigned long  SokNameToIP (char * pszName)
//   {
//   unsigned long lAddr;
//   struct hostent *pheHost;
//   
//   pheHost = gethostbyname(pszName);
//   if (pheHost == NULL)
//      { /* Error or Not Found */
//      lAddr = (unsigned long) -1;
//      }
//   else 
//      {
//
//      lAddr = * (unsigned long *) pheHost->h_addr;
///*
//      if (pheHost->h_addr == NULL)
//         { 
//         lAddr = (unsigned long) -1;
//         }
//      else
//         {
//         lAddr = * (unsigned long *) pheHost->h_addr;
//         }
//*/
//      }
//
//   return(lAddr);
//
//   }




     


/******************************************************************************/


/* 
 *   This function will convert an integer in the host's natural byte order
 *   to an integer in network byte order.
 *
 *   Input parameters are as follows:
 *      sHost  the number to be converted.
 *
 *   Returned parameters are as follows:
 *      function value  the number after conversion.
 *
*/

unsigned short SokHostToNetShort(unsigned short sHost)

   {
   unsigned short sNewInt;

   sNewInt = htons(sHost);

   return(sNewInt);
   }


/******************************************************************************/


/* 
 *   This function will convert an integer from network byte order to an 
 *   integer in the host's byte order.
 *
 *   Input parameters are as follows:
 *      sNet  the number to be converted.
 *
 *   Returned parameters are as follows:
 *      function value  the number after conversion.
 *
*/

unsigned short SokNetToHostShort ( unsigned short sNet)

   {
   unsigned short sNewInt;

   sNewInt = ntohs(sNet);

   return(sNewInt);

   }


/******************************************************************************/



/* 
 *   This function will convert an integer in the host's natural byte order
 *   to an integer in network byte order.
 *
 *   Input parameters are as follows:
 *      lHost  the number to be converted.
 *
 *   Returned parameters are as follows:
 *      function value  the number after conversion.
 *
*/

unsigned long SokHostToNetLong ( unsigned long lHost)

   {
   unsigned long lNewInt;

   lNewInt = htonl(lHost);

   return(lNewInt);
   }


/******************************************************************************/


/* 
 *   This function will convert an integer from network byte order to an 
 *   integer in the host's byte order.
 *
 *   Input parameters are as follows:
 *      lNet  the number to be converted.
 *
 *   Returned parameters are as follows:
 *      function value  the number after conversion.
 *
*/

unsigned long SokNetToHostLong ( unsigned long lNet)

   {
   unsigned long lNewInt;

   lNewInt = ntohl(lNet);

   return(lNewInt);

   }


/******************************************************************************/


/* 
 *   This function is used to get a connection from the queue of pending
 *   connections. It will create a new socket with the same parameters as
 *   the existing socket sSocket and that value is returned, already connected.
 *
 *   Input parameters are as follows:
 *      sSocket  the socket which is the well known port for this application.
 *
 *   Return parameters are as follows:
 *      function value  the descriptor of the newly created socket.
 *
*/

SOCKET    SokAcceptCon(SOCKET sSocket)

   {
      struct sockaddr_in saFrom;
      int iSize;
      SOCKET iRetSkt;

      iSize = sizeof(saFrom);
      iRetSkt = accept(sSocket, NULL, NULL);
      if (iRetSkt == -1)
         {    /* Error Condition */
         }

      return(iRetSkt);

   }



/******************************************************************************/


/*
 *    This function will bind a socket to the interface indicated by pszAddr,
 *    an IP address in the dotted quad form.
 *
 *    Input parameters are as follows:
 *       sSocket   the socket which is to be bound.
 *       pszAddr   a pointer to the string containing the address of the
 *                 interface to bind the socket to.
 *       lPortnum  an integer indicating which port number to bind the socket at.
 *
 *    Return parameters are as follows:
 *       Standard return codes are used.
 *
*/

long   SokBind(SOCKET sSocket, char * pszAddr, long lPortNum)

   {
      struct sockaddr_in saName;
      int iRetCode;

      memset(&saName, 0, sizeof(saName));
      saName.sin_family = AF_INET;
//      saName.sin_addr.s_addr = SokNameToIP(pszAddr);
      saName.sin_addr.s_addr = inet_addr(pszAddr);
      saName.sin_port = SokHostToNetShort(lPortNum);

      iRetCode = bind(sSocket, (struct sockaddr *) &saName, sizeof(saName));
      if (iRetCode == -1)
         {  /* Error Condition */
         }

      return(iRetCode);

   }


/******************************************************************************/


/* 
 *   This function will return the error code that was last generated.
 *
 *   Input parameters are as follows:
 *      There are no input parameters at this time.
 *
 *   Return parameters are as follows:
 *      function value  the return value is the error code last generated.
 *
*/

long SokGetError()

   {
   }

/******************************************************************************/


/* 
 *   This function is used to control socket behavior by changing parameters
 *   on the socket itself.
 *
 *   Input parameters are as follows:
 *       sSocket - the socket to change.
 *       lLevel - the level at which this option is defined. Currently this 
 *                must be SOL_SOCKET.
 *       lName - the option that is being set or changed.
 *       pValue - pointer to the buffer that contains the value of the option.
 *       lLength - the length of the buffer.
 *
 *   Return parameters are as follows:
 *       Standard return codes are used.
 *
*/

long SokSetOpt (SOCKET sSocket, long lLevel, long lName, void * pValue, long lLength)

   {
      int iRetCode;

      iRetCode = setsockopt(sSocket, lLevel, lName, (char *) pValue, lLength);
      if (iRetCode == -1)
         {   /* Error Condition */
         }

      return(iRetCode);

   }

/******************************************************************************/


/* 
 *   This function is used to control socket behavior by changing parameters
 *   on the socket itself.
 *
 *   Input parameters are as follows:
 *       sSocket - the socket to change.
 *       lLevel - the level at which this option is defined. Currently this 
 *                must be SOL_SOCKET.
 *       lName - the option that is being set or changed.
 *       pValue - pointer to the buffer that contains the value of the option.
 *       lLength - the length of the buffer.
 *
 *   Return parameters are as follows:
 *       Standard return codes are used.
 *       The memory pointed to by value and length will be set upon returning
 *       from the function.
 *
*/

long SokGetOpt (SOCKET sSocket, long lLevel, long lName, void * pValue, long * lLength)

   {
      int iRetCode;

      iRetCode = getsockopt(sSocket, lLevel, lName, (char *) pValue, (int *)lLength);
      if (iRetCode == -1)
         {   /* Error Condition */
         }
      
      return(iRetCode);

   }

/******************************************************************************/


/* 
 *   This function is used to control the mode of a socket.
 *
 *   Input parameters are as follows:
 *      Both -
 *        sSocket   the socket to change.
 *        lCmd  the command to perform
 *        pData the pointer to the parameter associated with the command.
 *        lLength the length of the area pointed to by data.
 *
 *   Return parameters are as follows:
 *      Standard return codes are used.
 *
*/

long SokIoctl ( SOCKET sSocket, long lCmd, void * pData, long lLength)

   {
      int iRetCode;

      iRetCode = ioctl(sSocket, lCmd, (char *) pData, lLength);
      if (iRetCode == -1)
         {   /* Error Condition */
         }

      return(iRetCode);
   }

/******************************************************************************/


/* 
 *   This function is used to determine if any sockets are ready to be read from,
 *   written to, or have generated an exception. To set the bit in the bit mask
 *   that is used to tell which socket is to be checked, use the following 
 *   macros, from <sys\select> :
 *        int  socket;
 *        struct fd_set *bit_mask;
 *
 *        FD_SET(socket, bit_mask)        Sets the bit for socket in the mask.
 *        FD_CLR(socket, bit_mask)        Clears the bit for socket in the mask.
 *        FD_ISSET(socket, bit_mask)      Determines if the bit for socket is
 *                                        set in the bit mask.
 *        FD_ZERO(bit_mask)               Clears the entire bit mask.
 *
 *
 *   Input parameters are as follows:
 *       lNFDS - Number of sockets to check.
 *       fsReadfds - pointer to set of sockets to be checked for readablility.
 *       fsWritefds - pointer to set of sockets to be checked for writability.
 *       fsExceptfds - pointer to set of sockets to be checked for exceptions.
 *       ptTimeout - pointer to a timeval structure that gives the maximum amount
 *                 of time to wait before returning. If NULL, the call is 
 *                 blocking. If 0, the call returns immediately.
 *
 *   Return parameters are as follows:
 *       function value - -1 indicates error. 0 indicates time limit expired.
 *                        Any other number indicates the total number of 
 *                        sockets from the 3 fd_set structures that are ready.
 *
*/

long SokSelect (long lNFDS, fd_set * fsReadfds, fd_set * fsWritefds, fd_set * fsExceptfds, struct timeval * ptTimeout)

   {
      int iRetCode;

      iRetCode = select(lNFDS, fsReadfds, fsWritefds, fsExceptfds, ptTimeout);
      if (iRetCode == -1)
         {   /* Error Conditon */
         }

      return(iRetCode);

   }

/******************************************************************************/


/*
 *   This function is used to receive a file over a socket. 
 *
 *   Input parameters are as follows :
 *       sSocket - The socket to use. sSocket is assmued to be already connected.
 *       pszFileName - an optional local name for the file to be received.
 *                     if NULL, the function will use the named that it gets from
 *                     the socket connection.
 *       pszPath - an optional path to place the file. default is the current
 *                 directory if passed in as NULL.
 *
 *   Return parameters are as follows :
 *       function value - Will be one of : RETURN_OK
 *                                         RETURN_NET_ERROR
 *                                         RETURN_DISK_ERROR
 *                                         RETURN_MEMORY_ERROR
 *                                         RETURN_GENERAL_ERROR
*/

unsigned short SokFileRecv(SOCKET sSocket, char * pszFileName, char * pszPath) 

   {
      void * memBuff, *memOut;
      int  iLen, iFileCnt, iRetCode;
      FILE * fOutFile;
      char szFileSpec[256];
      unsigned long lLength, lLoop, iTot;
      unsigned short usBuffSize;

        
      memOut = (void *) malloc(RETURN_MSG);
      if (memOut == NULL)
         { /* Error Condition */
           return(RETURN_MEMORY_ERROR);
         }

   	/*--- Receive exactly RETURN_MSG bytes ---*/
	   iTot = 0;

   	while (iTot < RETURN_MSG)
	   	{
		   iLen = SokBuffRecv(sSocket, (char *)memOut + iTot,
			               		 RETURN_MSG - iTot);
		   if (iLen == -1)
			   {
		     	/* Error Condition */
			   iRetCode = RETURN_NET_ERROR;
			   }
		   else
			   {
			   iTot += iLen;
			   }
		   }

	   if (iTot != RETURN_MSG)
		   {
		   /*--- There must have been some communication error ---*/
		   return RETURN_NET_ERROR;
		   }
	   else
	    	{
		   iRetCode = RETURN_OK;
		   }


      if (iRetCode == RETURN_OK)
         {
            memBuff = memOut;
            lLength = *(unsigned long *)memBuff;
            memBuff = ((unsigned long *)memBuff) + 1;
            usBuffSize = *(unsigned short *) memBuff;
            memBuff = ((unsigned short *)memBuff) + 1;
            strcpy(szFileSpec, (char *)memBuff);

	    lLength = SokNetToHostLong(lLength);
	    usBuffSize = SokNetToHostShort(usBuffSize);
            usBuffSize = ( usBuffSize > BUFF_SIZE ) ? BUFF_SIZE : usBuffSize;
         }

      if (iRetCode == RETURN_OK)
         {  /* Ok to this point, allocate the file buffer. */
            memBuff = (void *)malloc(usBuffSize);

            if (memBuff == NULL)
               { /* Error Condition */
                 iRetCode = RETURN_MEMORY_ERROR;
               }
         }

      if (pszPath != NULL)
         { /* We have a path to use. */
           if (pszFileName == NULL)
              {  /* We need to prepend the path to the filename sent to us. */
                 strcpy(memOut, pszPath);
                 if (pszPath[strlen(pszPath-1)] != '\\')
                    strcat(memOut, "\\");
                 strcat(memOut, szFileSpec);
                 strcpy(szFileSpec, memOut);
              }
            else
              {  /* We have a filename that is locally specified. */
                 strcpy(szFileSpec, pszPath);
                 if (pszPath[strlen(pszPath-1)] != '\\')
                    strcat(szFileSpec, "\\");
                 strcat(szFileSpec, pszFileName);
              }
         }
       else
         { /* We will put the file in the current directory. */
           if (pszFileName != NULL)
              {  /* We will use the filename passed in to us by the caller.
                  *   Otherwise, we will use szFileSpec as received from the
                  *   network.
                 */
                 strcpy(szFileSpec, pszFileName);
              }
           /* Else the value in szFileSpec is already correct. */
         }
            
      /* Open the file for writing in binary mode, so we can bail if the call 
       *   fails. Don't do it if we have already encountered an error.
      */


      if (iRetCode != RETURN_NET_ERROR && iRetCode != RETURN_MEMORY_ERROR)
          {
            fOutFile = my_fopen(szFileSpec, "wb");
            if (fOutFile == NULL)
               { /* Error Condition */
                 iRetCode = RETURN_DISK_ERROR;
               }
          }


      /* Now we need to build our acknowledgement buffer and send it back
       *   to let the sender know that we are ready to receive a file.
      */
      if ( iRetCode == RETURN_NET_ERROR || iRetCode == RETURN_DISK_ERROR ||
           iRetCode == RETURN_MEMORY_ERROR )
            {  /* We have encountered an error and need to abort this
                *   call, send an error buffer over and return with an error,
                *   closing the file if necessary.
               */
               *(unsigned long *)memOut = SokHostToNetLong((unsigned long) NAK);
               *((unsigned short *) ((unsigned long *)memOut + 1)) = SokHostToNetShort(0);
               lLength = SokBuffSend(sSocket, memOut, RETURN_MSG);

               /*--- we must send exactly RETURN_MSG bytes here ---*/
               if (lLength == -1 || lLength != RETURN_MSG)
                  { /* The net is acting up, return the network error. */
                    return(iRetCode);
                  }
	       free(memOut);
	       free(memBuff);
               fclose(fOutFile);
               return(iRetCode);
            }
       else
            {  /* Everything looks good so far, send ok and prepare to get
                *   the file data.
               */
               *(unsigned long *)memOut = SokHostToNetLong((unsigned long) HANDSHAKE_OK);
               *((unsigned short *) ((unsigned long *)memOut + 1)) = SokHostToNetShort(usBuffSize);
               iRetCode = SokBuffSend(sSocket, memOut, RETURN_MSG);
               /*--- check if send completed OK ---*/
               if (iRetCode == -1 || iRetCode != RETURN_MSG)
                  { /* NetWork Error Condition. */
                    iRetCode = RETURN_NET_ERROR;
                  }
                else
                  {
                    iRetCode = RETURN_OK;
                  }
            }

      /* Start Getting Data over sSocket, loop until filesize < usBuffSize.
      */

      iTot = lLength;

    {
      /* This is a test to see if we can fix the problem we have been
       * having with fragmentation of buffers that were sent.
       * Sync it after every recv.
       */

      int i;

      i = fflush(fOutFile);
      if ( i != 0 )
         { /* Error Condition */
           iRetCode = RETURN_DISK_ERROR;
         }
    }

      while ( (iRetCode == RETURN_OK) && (iTot >= usBuffSize) )
         {
            /*
             * The following modification was made by GTL on 2-26-95.
             *
             * Because ther is no way of knowing how the underlying
             * communications layers break up the buffer that was sent,
             * this SokBuffRecv has to loop until exactly usBuffSize
             * bytes are received. If this loop would not be here, a
             * fraction of usBuffSize (the number in lLength) would be
             * subtracted from iTot in the statement below, and the condition
             * iTot >= usBuffSize might be met earlier then we expected!!
             */
            for (lLoop = 0; lLoop < usBuffSize; lLoop += lLength)
               {
               lLength = SokBuffRecv( sSocket, memBuff, usBuffSize);
               if (lLength == -1)
                  { /* Error Condition */
                    iRetCode = RETURN_NET_ERROR;
                    continue;
                  }

               iLen = fwrite(memBuff,(size_t) 1, (size_t)lLength, fOutFile);

               if (iLen != lLength) 
                  { /* Error Condition */
                  iRetCode = RETURN_DISK_ERROR;
                  continue;
                  }
               }

            iTot -= lLoop;
 
        {
	    /* This is a test to see if we can fix the problem we have been
        * having with fragmentation of buffers that were sent.
	     * Sync it after every recv.
	     */
              int i;

              i = fflush(fOutFile);
              if ( i != 0 )
                 { /* Error Condition */
                   iRetCode = RETURN_DISK_ERROR;
                   continue;
                 }
            }

           
         }  /* End While */

      if ( (iTot > 0)  && (iRetCode == RETURN_OK) )
           /* There is still more data. */
         {  

            for (lLoop = 0; lLoop < iTot; lLoop += lLength)
               {
               lLength = SokBuffRecv( sSocket, memBuff, iTot);
               if (lLength == -1)
                  { /* Error Condition */
                    iRetCode = RETURN_NET_ERROR;
                  }

               iLen = fwrite(memBuff, 1, lLength, fOutFile);

               if (iLen != lLength) 
                  { /* Error Condition */
                    iRetCode = RETURN_DISK_ERROR;
                  }
               }

            iTot -= lLoop;
            if (iTot != 0)
               {  /* This should never happen!! This means we dropped out
                   *   of the loop above and then did not get the correct
                   *   amount of data on our next read. Big problem somewhere!
                  */
                  iRetCode = RETURN_GENERAL_ERROR;
               }

          }

      iLen = fclose(fOutFile);
      if ( iLen == EOF )
         {  /* Error Condition of some sort. Disk problems. */
            iRetCode = RETURN_DISK_ERROR;
         }
      free(memBuff);
      free(memOut);
      return(iRetCode);
   }

/******************************************************************************/ 

/*
 *   This function is used to send a file over a socket that is assumed to be
 *   already connected.
 *
 *   Input parameters are as follows:
 *       sSocket - The socket to send the file over.
 *       pszFileName - the name of the file to send. If it is not in the current 
 *                     directory, it should be fully qualified.
 *
 *   Return parameters are as follows :
 *       function value - Will be one of : RETURN_OK
 *                                         RETURN_NET_ERROR
 *                                         RETURN_DISK_ERROR
 *                                         RETURN_MEMORY_ERROR
 *                                         RETURN_GENERAL_ERROR
 *
*/

unsigned short SokFileSend(SOCKET sSocket, char * pszFileName)

   {
       void * memBuff, *memIn;
       FILE * fp;
       int iRetCode, iLength, iTotal;
       unsigned long lFileSize;
       unsigned short usBuffSize;
       char * pszName, *p;


       iTotal = 0;
       iRetCode = RETURN_OK;

       fp = my_fopen(pszFileName,"rb");
       if (fp == NULL)
          {
             return(RETURN_DISK_ERROR);
          }

          /* Craig's hack to find the filename from a 
           * possibly fully qualified path.
          */

       if (p = strrchr (pszFileName, '\\'))
          pszName = p+1;
       else if (p = strrchr (pszFileName, ':'))
          pszName = p+1;
       else
          pszName = pszFileName;

          /* 
           * The following three paragraphs are used to determine the
           * size of the file in bytes. We seek to the end of file,
           * use ftell to tell us the position in bytes from the start
           * of the file, and then seek back to the beginning of the 
           * file. This is a lot of work, but should be portable in the
           * future to other platforms.
           */

       iRetCode = fseek(fp, 0, SEEK_END);
       if (iRetCode != 0)
          {  /* Error Condition */
             return(RETURN_DISK_ERROR);
          }

       lFileSize = ftell(fp);
       if (lFileSize == -1)
          {  /* Error Condition */
             return(RETURN_DISK_ERROR);
          }

       iRetCode = fseek(fp, 0, SEEK_SET);
       if (iRetCode != 0)
          {  /* Error Condition */
             return(RETURN_DISK_ERROR);
          }


          /* This is the allocation of the local handshake buffer.
          */

       memIn = (void *) malloc(RETURN_MSG);
       if (memIn == NULL)
          {  /* Error Condition */
             return(RETURN_MEMORY_ERROR);
          }

          /* This is where we build the first outgoing message, 
           *   containing the filesize, buffer size, and filename that
           *   we are sending over the connection.
          */
       lFileSize = SokHostToNetLong(lFileSize);
       iRetCode = RETURN_OK;
       memBuff = memIn;
       *(unsigned long *) memBuff = lFileSize;
       memBuff = ((unsigned long *)memBuff) + 1;
       *(unsigned short *) memBuff = SokHostToNetShort(BUFF_SIZE);
       memBuff = ((unsigned short *)memBuff) + 1;
       strcpy(memBuff, pszName);

          /*--- Send exactly RETURN_MSG bytes ---*/

       iLength = SokBuffSend(sSocket, memIn, RETURN_MSG);
       if (iLength == -1 || iLength != RETURN_MSG)
          {  /* Error Condition */
             iRetCode = RETURN_NET_ERROR;
          }

	   /* 
       * Wait for a reply. Synchronize the FileSend and FileRecv calls: Read until
	    * the buffer is full (this should be the exact number of bytes that the
	    * FileRecv call sends back as an acknowledgement).
	    */
	   iTotal = 0;

	   while (iTotal < RETURN_MSG)
		   {
	     	iLength = SokBuffRecv (sSocket, (char *)memIn + iTotal,
										     RETURN_MSG - iTotal);
		   if (iLength == -1)
			   {
			   /* Error Condition */
			   iRetCode = RETURN_NET_ERROR;
			   }
		   else
			   {
			   iTotal += iLength;
			   }
		   }

	   if (iTotal != RETURN_MSG)
		   {
		   /* There must have been some communication error */
		   return RETURN_NET_ERROR;
		   }


          /* Extract the buffer size if this is an OK msg, abort otherwise.
          */
       if ((iRetCode == RETURN_OK) && ((*(unsigned long *)memIn) == SokHostToNetLong(HANDSHAKE_OK) ))
          {  /* Everything is ok. */
             memBuff = ((unsigned long *)memIn) + 1;
             usBuffSize = *(unsigned short *) memBuff;
	     usBuffSize = SokNetToHostShort(usBuffSize);
          }
        else
          {  /* There is a problem, leave the function with an
              *   error code of RETURN_NET_ERROR.
             */
             fclose (fp);
           free(memIn);
             return (RETURN_NET_ERROR);
          }

       if (iRetCode == RETURN_OK)
          {  /* We are OK so far, allocate memory */
             usBuffSize = (SokNetToHostLong(lFileSize) < usBuffSize) ?
                              SokNetToHostLong (lFileSize) : usBuffSize;

          if (usBuffSize)
             {
             memBuff = (void *) malloc(usBuffSize);
             if (memBuff == NULL)
                {
                /*--- Error Condition ---*/
		          free(memIn);
                return(RETURN_MEMORY_ERROR);
                }
             }
          }


       iTotal = SokNetToHostLong(lFileSize);
       while ( (iLength = fread(memBuff, 1, usBuffSize, fp)) )
         {
           if (iLength == -1)
              {  /* Error Condition */
                 iRetCode = RETURN_DISK_ERROR;
              }
           iTotal -= iLength;

           iRetCode = SokBuffSend(sSocket, memBuff, iLength);
           if (iRetCode == -1 || iRetCode != iLength)
              {  /* Error Condition */
                 iRetCode = RETURN_NET_ERROR;
                 continue;
              }
           else 
              {  /* We are ok, reset iRetCode. */
                 iRetCode = RETURN_OK;
              }
  

         } /* End While */

      if (iTotal != 0)
         {  /* This should never happen! We have obtained an EOF from
             *   fread, but not processed all of the data we think is in
             *   the file, return a disk error.
            */
            iRetCode = RETURN_DISK_ERROR;
         }
         
      iLength = fclose (fp);
      if (iLength == EOF)
         {  /* Error Condition */
            iRetCode = RETURN_DISK_ERROR;
         }

      if (usBuffSize)
         free(memBuff);
      free(memIn);
      return(iRetCode);
    } /* End File Send */

/****************************************************************************/ 

