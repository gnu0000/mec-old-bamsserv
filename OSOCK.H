/*
 *        10 Mar 95
 *
 *        * * * * *   This is Prelim Release 2.6   * * * * * 
 *
 *        This is a prototype header file for the 'wrapping' functions only.
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


/******************************************************************************/


#ifndef OSOCK

#define OSOCK
#define TCP 1
#define UDP 0
#define LISTEN_QUEUE 5

#define BUFF_SIZE   32000
/*
#define BUFF_SIZE   32768
*/
#define RETURN_MSG  128

#define DONE 1
#define HANDSHAKE_OK 1
#define NAK  0

#define RETURN_OK             0
#define RETURN_NET_ERROR      1
#define RETURN_DISK_ERROR     2
#define RETURN_MEMORY_ERROR   3
#define RETURN_GENERAL_ERROR  4

typedef long    SOCKET;
typedef SOCKET *PSOCKET;

#endif


/******************************************************************************/


/* This function will create a socket and bind it to the address passed in 
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

SOCKET SokCreateServer(long lPortNum, long lSocketType);


/******************************************************************************/


/* This function will create a socket to be used on the client machine.
 *
 *   Input parameters are as follows:
 *      lSocketType The type of socket to be created, either TCP(stream) or UDP
 *                  (datagram).
 *
 *   Returned parameters are as follows:
 *      function value the newly created socket.
 *
*/

SOCKET SokCreateClient(long lSocketType);


/******************************************************************************/


/* This function will establish a connection to the specified machine.
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

long    SokConnectTo(SOCKET sSocket, char * pszTarget, unsigned long lPortNum);


/******************************************************************************/


/* This function will close any connections on the socket and disconnect them.
 *
 *   Input parameters are as follows :
 *      sSocket - local socket to disconnect.
 *
 *   Returned parameters are as follows:
 *      Standard return codes used.
 *
*/

long    SokDisconnect(SOCKET sSocket);


/******************************************************************************/


/* This function will send a buffer using the given socket. 
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

long    SokBuffSend(SOCKET sSocket, void * pBuff, long lLength);


/******************************************************************************/


/* This function will send a message using the given socket. 
 *   
 *   Input parameters are as follows:
 *      sSocket - local socket to use.
 *      pMsg - pointer to a null terminated string to be sent.
 *      pszTarget - pointer the character string containing the name of
 *               the target machine.
 *      lPortNum  - the port number to connect to on the target.
 *
 *   Returned parameters are as follows:
 *      function value  -1 indicates error, any other value indicates number 
 *                      of characters sent.
 *
*/

long    SokDgramSend(SOCKET sSocket, void * pMsg, 
                     char * pszTarget, long lPortNum);


/******************************************************************************/


/* This function will receive a buffer using the given socket. 
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

long    SokBuffRecv(SOCKET sSocket, void * pBuff, long lLength);


/******************************************************************************/


/* This function will receive a message using the given socket. 
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

long    SokDgramRecv(SOCKET sSocket, void * pMsg, 
                     long lLength, char * pszTarget);


/******************************************************************************/


/* This function will initialize the socket subsystems and prepare them for
 *   use by the application.
 *
 *   Input parameters are as follows:
 *      None.
 *
 *   Returned parameters are as follows:
 *      Standard return codes are used.
 *
*/

long   SokInit (void);


/******************************************************************************/


/* This function will translate a network address in the Internet style(IP)
 *   dotted quad form to the corresponding network representation.
 *
 *   Input parameters are as follows:
 *      pszDottedAddr  a character string containing the address to be translated.
 *
 *   Returned parameters are as follows:
 *      function value  the address in network form.
 *
*/

unsigned long  SokNetworkAddr (char * pszDottedAddr);


/******************************************************************************/


/* This function will translate a character string containing the name of a
 *   network host to its corresponding address in network form.
 *
 *   Input parameters are as follows:
 *      pszName  a character string containing the name for which to find an address.
 *
 *   Returned parameters are as follows:
 *      function value  the address of the host in network form.
 *
*/

unsigned long  SokNameToIP (char * pszName);


/******************************************************************************/


/* This function will convert an integer in the host's natural byte order
 *   to an integer in network byte order.
 *
 *   Input parameters are as follows:
 *      sHost  the number to be converted.
 *
 *   Returned parameters are as follows:
 *      function value  the number after conversion.
 *
*/

unsigned short SokHostToNetShort ( unsigned short sHost);


/******************************************************************************/


/* This function will convert an integer from network byte order to an 
 *   integer in the host's byte order.
 *
 *   Input parameters are as follows:
 *      sNet  the number to be converted.
 *
 *   Returned parameters are as follows:
 *      function value  the number after conversion.
 *
*/

unsigned short SokNetToHostShort ( unsigned short sNet);


/******************************************************************************/


/* This function will convert an integer in the host's natural byte order
 *   to an integer in network byte order.
 *
 *   Input parameters are as follows:
 *      lHost  the number to be converted.
 *
 *   Returned parameters are as follows:
 *      function value  the number after conversion.
 *
*/

unsigned long SokHostToNetLong ( unsigned long lHost);


/******************************************************************************/


/* This function will convert an integer from network byte order to an 
 *   integer in the host's byte order.
 *
 *   Input parameters are as follows:
 *      lNet  the number to be converted.
 *
 *   Returned parameters are as follows:
 *      function value  the number after conversion.
 *
*/

unsigned long SokNetToHostLong ( unsigned long lNet);


/******************************************************************************/


/* This function will return the error code that was last generated.
 *
 *   Input parameters are as follows:
 *      There are no input parameters at this time.
 *
 *   Return parameters are as follows:
 *      function value  the return value is the error code last generated.
 *
*/

long SokGetError();


/******************************************************************************/


/* This function is used to control socket behavior by changing parameters
 *   on the socket itself.
 *
 *   Input parameters are as follows:
 *       sSocket - the socket to change.
 *       lLevel - the level at which this option is defined. Currently this
 *               must be SOL_SOCKET.
 *       lName - the option that is being set or changed.
 *       pValue - pointer to the buffer that contains the value of the option.
 *       lLength - the length of the buffer.
 *
 *   Return parameters are as follows:
 *       Standard return codes are used.
 *
*/

long SokSetOpt (SOCKET sSocket, long lLevel, long lName, 
                void * pValue, long lLength);


/******************************************************************************/


/* This function is used to control socket behavior by changing parameters
 *   on the socket itself.
 *
 *   Input parameters are as follows:
 *       sSocket - the socket to change.
 *       lLevel - the level at which this option is defined. Currently this
 *               must be SOL_SOCKET.
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

long SokGetOpt (SOCKET sSocket, long lLevel, long lName,
                void * pValue, long * lLength);


/******************************************************************************/


/* This function is used to control the mode of a socket.
 *
 *   Input parameters are as follows:
 *      Both -
 *        sSocket   the socket to change.
 *        lCmd  the command to perform
 *        pData the pointer to the parameter associated with the command.
 *        iLength the length of the area pointed to by data.
 *
 *   Return parameters are as follows:
 *      Standard return codes are used.
 *
*/

long SokIoctl ( SOCKET sSocket, long lCmd, void * pData, long lLength);


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

long SokSelect (long lNFDS, fd_set * fsReadfds, fd_set * fsWritefds, 
                fd_set * fsExceptfds, struct timeval * ptTimeout);


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

SOCKET    SokAcceptCon(SOCKET sSocket);


/******************************************************************************/


/*
 *    This function will bind a socket to the interface indicated by pszAddr,
 *    an IP address in the dotted quad form.
 *
 *    Input parameters are as follows:
 *       sSocket   the socket which is to be bound.
 *       pszAddr   a pointer to the string containing the name assigned
 *                 to the address to which the socket should be bound.
 *       lPortnum  an integer indicating which port number to bind the socket at.
 *
 *    Return parameters are as follows:
 *       Standard return codes are used.
 *
*/

long   SokBind(SOCKET sSocket, char * pszAddr, long lPortNum);


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
 *                 directory if passed in as NULL. Must have a trailing '\' if
 *                 used.
 *
 *   Return parameters are as follows :
 *       function value - Will be one of : RETURN_OK
 *                                         RETURN_NET_ERROR
 *                                         RETURN_DISK_ERROR
 *                                         RETURN_MEMORY_ERROR
 *                                         RETURN_GENERAL_ERROR
*/

unsigned short SokFileRecv(SOCKET sSocket, char * pszFileName, char * pszPath);


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

unsigned short SokFileSend(SOCKET sSocket, char* pszFileName);


/******************************************************************************/

/*
 *   This function is a newer version of the AcceptCon that will place the 
 *   IP address of the client in the second paramter.
 *
 *   Input parameters are as follows:
 *        sSocket - The Well Known Port socket that will be listened to to 
 *                obtain the new connection.
 *
 *   Output parameters are as follows:
 *        function value - A socket that is connected to the incoming client.
 *        pszIP - a character string containing the dotted quad form of the
 *                client's IP address.
 *
*/

SOCKET    SokAcceptConIP(SOCKET sSocket, char * pszIP);

/******************************************************************************/

/* 
 *   This function will return the IP address of the peer connected on the
 *   socket sSocket as a character string containing the dotted quad form.
 *
 *   Input parameters are as follows:
 *         sSocket - the socket to get the peer address from.
 *   Output parameters are as follows:
 *         function value - character string containing the dotted quad form of
 *	                  the IP address.
 *
*/

char * SokGetPeerIP(SOCKET sSocket, char * pszName);

/******************************************************************************/

