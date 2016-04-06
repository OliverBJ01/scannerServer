//    ======== mcuServer.c ========

#include "mcuServer.h"

// =============  Globals
//unsigned int count = 0;								//count interrupts
 int spiTxErrCount = 0;								// counts spi DAC transfer errors
 int tcpStopped		= 0;								// count of tcpWorker() stops

extern SPI_Transaction SpiTransaction;	// SPI transaction structure
//extern SPI_Params SpiParams;						// SPI parameter structure
extern SPI_Handle spi;

// SPI buffer located in DMA accessible memory
 extern UShort    TxBuffer[2];					// Tx  buffer,   UShort = 16 bits

  struct statusWord serverStatus;			// server status word

//  ========================== tcpWorker()  =========================

#pragma CODE_SECTION ( tcpWorker, ".devcode")

void tcpWorker(UArg arg0, UArg arg1)
{
   SOCKET clientfd = (SOCKET)arg0;					//arg0 is socket file descriptor (sockfd)
   int nbytes;
   UShort mssgCount = 0;											// counts messages received
   bool flag = true;

   struct clientMssg clientMssg;					// client message
   struct serverMssg serverMssg;					// server message

   fdOpenSession(TaskSelf());
   System_printf("tcpWorker: start clientfd = 0x%x\n", clientfd);    System_flush();

   //========= Loop while we receive data on this connection
   // if 0 bytes received, terminates this task. tcpHandler runs OK, but
   // current client stalls, and needs amending to start again.
   // Why stop on 0?  Perhaps just go around again?
   while (flag) {

	   nbytes = recv(clientfd, & clientMssg, sizeof(clientMssg), 0);

	   // This does't work, as Connect sends 0 byte length messages
	 //  if (nbytes != sizeof(clientMssg))
	//	   continue;

	    //	  System_printf("pre-htons: %x     %x      cmd: %d   ", clientMssg.scanX, clientMssg.scanY,   clientMssg.info );  System_flush();
	   // convert words from network to host order
	   clientMssg.scanX = ntohs(clientMssg.scanX);
	   clientMssg.scanY = ntohs(clientMssg.scanY);
	   clientMssg.info 	= ntohs(clientMssg.info);

	   //System_printf("Recvd  : %x     %x      cmd: %d    ", clientMssg.scanX, clientMssg.scanY,   clientMssg.info );  System_flush();

	   switch (clientMssg.info) {

	   case CMD_LASER_ON:
			   GPIO_write(LASER_PWR,  GPIO_PIN_ON);
				serverStatus.laserPower= true;
				break;
	   case CMD_LASER_OFF:
			   GPIO_write(LASER_PWR,  GPIO_PIN_OFF);
			    serverStatus.laserPower= false;
			   break;

	   case CMD_DAC_RESET_ON:
				GPIO_write(DAC_RST, GPIO_DAC_RESET_TRUE);	// sets pin low, ENABLES reset
				serverStatus.dacReset = true;
				break;
		case CMD_DAC_RESET_OFF:
				GPIO_write(DAC_RST, GPIO_DAC_RESET_FALSE);
				serverStatus.dacReset = false;
				break;

	   case    CMD_VERBOSE_ON:
				serverStatus.verbose = true;
			   break;
	   case    CMD_VERBOSE_OFF:
				serverStatus.verbose = false;
				 System_printf("verbose off");    System_flush();
			   break;
	   case    CMD_RESET:
				resetServer();
			   break;
	   case CMD_NULL_MSSG:
			   // do nothing
				break;
	   case CMD_SLEW:
			   // check if scanner values  changed, if so send to DAC
				if ((clientMssg.scanX != TxBuffer[0] ) | (clientMssg.scanY != TxBuffer[0]))  {

						 // update SPI transfer buffer,
						 TxBuffer[0] =  clientMssg.scanX;
						 TxBuffer[1] =  clientMssg.scanY;

						//Swi_post(swi1);							// SWI1 - Initiate SPI transfer
						 bool            transferOK;
						 transferOK = SPI_transfer(spi, & SpiTransaction);
						 if(!transferOK) {
								 if (spiTxErrCount++ > 5)  {
								 	 	serverStatus.spiError = true;
										serverStatus.serverFault = FAULT;
										spiTxErrCount = 0;  }								 // reset counter
								 System_printf("Bad  SPI Tx:%d", spiTxErrCount);  	 System_flush();
						 }
						GPIO_toggle(LED3);				// show SPI activity
						// Task_sleep(2);			// for 2mS, SPI Tx takes <.4mS
			   }
				break;
	   default:					// unrecognised command
			   System_printf("Bad  command: %d     ", clientMssg.info);
   	   }

	   //======================= SEND ============================
       if (nbytes > 0) {										           			// Send response message.  nbytes is No. bytes received this message
			   // build server's response message
				serverMssg.status = serverStatus;		// send back serverState
				serverMssg.scanX = TxBuffer[0];
				serverMssg.scanY= TxBuffer[1];
				serverMssg.info = mssgCount;

				serverMssg.info = htons(serverMssg.info);	    	   // convert words from host to network
				serverMssg.scanX= htons(serverMssg.scanX);
				serverMssg.scanY= htons(serverMssg.scanY);

			   if (send(clientfd, &serverMssg, sizeof(serverMssg), 0 ) < 0)  	{
				   System_printf("send failed\n");      System_flush();			}

			   //   System_printf("Sent response\n");    System_flush();
			   // Command received, and serverStaus
			   if(serverStatus.verbose)
				   	   System_printf ("C:%d S:%d %d %d %d %d %d %d %d %d \n", clientMssg.info,
				   			serverMssg.status.serverFault, serverMssg.status.laserFault, serverMssg.status.scanXFault,
				   			serverMssg.status.scanYFault, serverMssg.status.unknownScannerFault,
				   			serverMssg.status.hwStartFail, serverMssg.status.laserPower,
				   			serverMssg.status.dacReset, serverMssg.status.spiError); System_flush();

			   mssgCount++;
       }
       else {																	// 0 bytes received, terminate this task.
			   fdClose(clientfd);
			   flag = false;													// trick, setting this flag causes break out of while loop.  Use break instead?
			   tcpStopped++;
			   System_printf("tcpWorker closed\n");      System_flush();
       }


   }   // ====================== end while loop

   System_printf("tcpWorker stop\n");    System_flush();
   fdCloseSession(TaskSelf());
   //Since deleteTerminatedTasks is set in the cfg file,  the Task will be deleted when the idle task runs.?????
   Task_exit();
}

//
// ================== scannerErrFxn() ==============================
// This is called by GPIO Hwi
// The GPIO call back structure (gpioPortBCallbacks) is populated so that
// enabling GPIO pins  SCANXFAULT and SCANYFAULT invokes Hwi calling
// this function

#pragma CODE_SECTION ( scannerErrFxn, ".devcode")

Void scannerErrFxn(void)  {

	GPIO_disableInt(SCANXFAULT);						// disable interrupt on this pin
	GPIO_disableInt(SCANYFAULT);

	serverStatus.serverFault = FAULT;					// set generic fault flag
    serverStatus.laserPower = false;
    // turn laser off and scanners on
    GPIO_write(LASER_PWR, GPIO_PIN_OFF);

    // determine which scanner failed
	if (GPIO_read(SCANXFAULT)) {		// true >=1
			serverStatus.scanXFault = FAULT;
			System_printf("ScannerX fail\n");  	 System_flush();
	}  	else   serverStatus.scanXFault = NO_FAULT;

	if (GPIO_read(SCANYFAULT)  )	{	// returns 2 if set
			serverStatus.scanYFault = FAULT;
			System_printf("ScannerY fail\n");  	 System_flush();
	}  	else  		serverStatus.scanYFault = NO_FAULT;

    // we got here but both scanners now show no fault
	if (!serverStatus.scanXFault && !serverStatus.scanYFault) {
			serverStatus.unknownScannerFault = FAULT;
			System_printf("Unknown Scan problem\n");  	 System_flush();  	}
}

//
//  ======================= main() ========================
//
#pragma CODE_SECTION ( main, ".devcode")

int main(void){
    Board_initGeneral();
    Board_initGPIO();
	System_printf("GPIO init.\n");  	System_flush();

	//clear status bits
	serverStatus.verbose =  false;
	serverStatus.spiError =  false;


    if  (checkHardware() )	 {				// ensure scanner powered up
    	 	 	 System_printf("Scanner start fail\n");     System_flush();
    	 		serverStatus.serverFault = FAULT;
    			}
    else {
				System_printf("Hardware OK\n");  	 System_flush();

				// GPIO callbacks not initialised 'cause they would go immediatly
				//  ***** appears not needed **********
				//GPIO_setupCallbacks(&gpioPortBCallbacks);


				// install call back functions
				GPIO_setCallback(SCANXFAULT, scannerErrFxn);
				GPIO_setCallback(SCANYFAULT, scannerErrFxn);
				//GPIO_setCallback(SCANXFAULT, dacErrFxn);

				//GPIO_enableInt(SCANXFAULT, GPIO_INT_HIGH);		// interrupt when high
				//GPIO_enableInt(SCANYFAULT, GPIO_INT_HIGH);
				GPIO_enableInt(SCANXFAULT);		// interrupt when high
				GPIO_enableInt(SCANYFAULT);

				 //set laser and DAC GPIO control pins.  They come out of boot = LOW.
				GPIO_write(DAC_RST, 		GPIO_PIN_ON);				// sets DAC ~RST pin HI, disables reset
				GPIO_write(LASER_PWR,  GPIO_PIN_OFF);			// laser off
				serverStatus.laserPower  = false;

				GPIO_write(LED2,  BOARD_LED_ON);
    }

    Board_initSPI();					// SPI writes to DAC
    setupSPI();
    System_printf("SPI init\n");     System_flush();

   Board_initEMAC();				// Ethernet subsystem
    System_printf("EMAC init\n");     System_flush();

    BIOS_start();							//launches tcpHandlerFxn() at Priority 3

    return (0);
}




//======================checkHardware() ===========
// During startup the scanner board will report fault (hi) until approx. 3 seconds from power on
// Spins here until both scanners are ready
// GPIO_read() returns index of pin if set (i.e. 2 for Port B Pin 1), else 0

#pragma CODE_SECTION ( checkHardware, ".devcode")

int checkHardware(void) {
	int hwStatus = FAULT;
	int loopCount = 0;

	 //set GPIO scanner and DAC control pins.  They come out of boot = LOW.
	//GPIO_write(DAC_RST, GPIO_PIN_ON);						// sets DAC ~RST pin HI, disables DAC reset

	GPIO_write(LASER_PWR,  GPIO_PIN_OFF);			// laser off
	serverStatus.laserPower  = 	false;

	GPIO_write(LED2,  BOARD_LED_ON);

	// not implemented yet
	serverStatus.laserFault = 	false;

	// init the server fault bits that don't get touched in this function
    serverStatus.spiError = NO_FAULT;
    serverStatus.unknownScannerFault  = 	NO_FAULT;

  	System_printf("Waiting for Scanners\n");  	 System_flush();
	while (hwStatus == FAULT)  	{
			if (GPIO_read(SCANXFAULT)) {		// true >=1
				serverStatus.scanXFault = FAULT;
			  	goto zzz;
			}
			else
				serverStatus.scanXFault = NO_FAULT;

			// falls through once scanner X is ready
//			loopCount = 0;
			if (GPIO_read(SCANYFAULT)  )		// returns 2 if set
				serverStatus.scanYFault = FAULT;
			else {
				serverStatus.scanYFault = NO_FAULT;
				hwStatus = NO_FAULT;			// causes exit from while
			}

			// =========== BROKEN =================
zzz:	loopCount++;
			Task_sleep(500);		//			tick = 1000uS (timer needs Swi

			//  timeout to fault after 5 seconds
			if (loopCount  == 10)	{						// 5 seconds
				serverStatus.hwStartFail = FAULT;
				serverStatus.serverFault = FAULT;
				return (FAULT);
			}

	}  // while,   falls through on no fault
	serverStatus.hwStartFail = NO_FAULT;
	serverStatus.serverFault = NO_FAULT;
  	System_printf("No scanner fault\n");  	 System_flush();
	return (NO_FAULT);
}

// ================ resetServer ()=============
//  may want to rename, all this does is reset the scanners
//continues in main loop

int resetServer(void) {
	System_printf("Resetting scanners...\n");  	 System_flush();

    if  (checkHardware() )	 {				// ensure scanner powered up
    	 	 	 System_printf("Scanner start fail\n");     System_flush();
    	 		serverStatus.serverFault = FAULT;
    	 		return 1;  				//return bad
    			}
    else {
				System_printf("Hardware OK\n");  	 System_flush();

				GPIO_enableInt(SCANXFAULT);		// interrupt when high
				GPIO_enableInt(SCANYFAULT);

				// ensure ~DAC_Reset is disabled
				GPIO_write(DAC_RST, GPIO_PIN_ON);						// sets DAC ~RST pin HI, disables DAC reset

				GPIO_write(LED2,  BOARD_LED_ON);

				System_printf("Reset OK\n");  	 System_flush();
				return 0;				// return good
    }
}
