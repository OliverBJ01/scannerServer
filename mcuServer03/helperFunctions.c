//  tcpFunctions.cc

#include "mcuServer.h"



//
//  =================== tcpHandlerFxn() ==================
//  Creates new Task to handle new TCP connections.
// Passive socket (server) P1156 Linux Prog. Interface
// receives port no. in arg0
//#pragma CODE_SECTION ( tcpHandlerFxn, ".devcode");

Void tcpHandlerFxn(UArg arg0, UArg arg1)
{
   SOCKET lSocket;
   struct sockaddr_in sLocalAddr;		// local address struct
   SOCKET clientfd;
   struct sockaddr_in client_addr;		// client address struct
  int addrlen=sizeof(client_addr);
  // const int addrlen=sizeof(client_addr);
   int optval;
  const  int optlen = sizeof(optval);
   int status;
   Task_Handle taskHandle;
   Task_Params taskParams;
   Error_Block eb;

   //  Allocate the file descriptor environment for this task
   fdOpenSession(TaskSelf());

   // create TCP socket
   lSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
   if (lSocket < 0) {
       System_printf("tcpHandler: socket failed\n");    System_flush();
       Task_exit();
       return;
   }

   //  IPAddress set in .cfg under network layer/IP  (or set DHCP here)
   memset((char *)&sLocalAddr, 0, sizeof(sLocalAddr));
   sLocalAddr.sin_family			 		= AF_INET;
   //sLocalAddr.sin_len 						= sizeof(sLocalAddr);
   sLocalAddr.sin_addr.s_addr = htonl(INADDR_ANY);
   sLocalAddr.sin_port						= htons(SERVERPORT);			// Port No.

	// bind own address to the socket
   status = bind(lSocket, (struct sockaddr *)&sLocalAddr, sizeof(sLocalAddr));
   if (status < 0) {
       System_printf("tcpHandler: bind failed\n");    System_flush();
       fdClose(lSocket);
       Task_exit();
       return;
   }

   // completes binding and creates connection request queue for incoming requests
   if (listen(lSocket, NUMTCPWORKERS) != 0){
       System_printf("tcpHandler: listen failed\n");      System_flush();
       fdClose(lSocket);
       Task_exit();
       return;
   }

   if (setsockopt(lSocket, SOL_SOCKET, SO_KEEPALIVE, &optval, optlen) < 0) {
       System_printf("tcpHandler: setsockopt failed\n");      System_flush();
       fdClose(lSocket);
       Task_exit();
       return;
   }
   System_printf("tcpHandler: setsockopt() complete\n");   System_flush();

   // =================== waits forever for a connection =========================
   // invokes tcpWorker() on new connection
   while (true) {
			   /* Accept a connection. The connection is returned on a new
				* socket, clientfd; the listening socket lSocket remains open
				* and can be used to accept further connections*/
       	   	   System_printf("Waiting for client\n");      System_flush();

			   clientfd = accept(lSocket, (struct sockaddr *)&client_addr, &addrlen);
			   System_printf("tcpHandler: Creating thread clientfd = %d\n", clientfd);    System_flush();

			   // Create the tcpWorker() task that handles the connection
			   Error_init(&eb);
			   Task_Params_init(&taskParams);
			   taskParams.arg0 = (UArg)clientfd;				// pass socket file descriptor  (sockfd)
			   taskParams.stackSize = 1024;
		//       taskParams.stackSize = 2048;
			   taskHandle = Task_create((Task_FuncPtr)tcpWorker, &taskParams, &eb);
			   if (taskHandle == NULL) {
				   System_printf("Failed to create tcpHandler()\n");   System_flush();
			   }
   }
}


// ==================== idleFxn() =====================
// 1/2 second activity heartbeat on LED2
void idleFxn(UArg arg0, UArg arg1)
{
    //period is set sysbios->timer control->tick period (1000uS)
	Task_sleep(500);
    GPIO_toggle(LED2);
}

