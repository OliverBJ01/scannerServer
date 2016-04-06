// mcuServer.h

// SysBios UG P 3-52 for max stack size method


#ifndef TCPECHO_H_
#define TCPECHO_H_

#ifdef __cplusplus
extern "C" {							// prevents name mangling of functions defined within
#endif

#include <xdc/std.h>										// XDCtools Header files
#include <xdc/cfg/global.h>
#include <xdc/runtime/Error.h>
#include <xdc/runtime/Memory.h>
#include <xdc/runtime/System.h>

#include <ti/sysbios/BIOS.h>					// BIOS Header files
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Task.h>

#include <ti/ndk/inc/netmain.h>		 	// NDK Header files
#include <ti/ndk/inc/_stack.h>

#include <ti/drivers/GPIO.h>					// TI-RTOS Header files
#include <ti/drivers/SPI.h>

// IP Address defined in .cfg to 10.1.1.99
#define SERVERPORT 1000

#define TCPPACKETSIZE 1024
#define NUMTCPWORKERS 1

// GPIO pins handle +/-4mA @ 0 - 3.3V
// defs for GPIOPadConfigSet()
// 	GPIO_PIN_TYPE_STD_WPU     Push-pull with weak pull-up
//	GPIO_PIN_TYPE_OD           			Open-drain
//	GPIO_PIN_TYPE_OD_WPU      Open-drain with weak pull-up
//	GPIO_PIN_TYPE_ANALOG

// for SSDD:
// SPI is implemented on the SSI0 peripheral. SSI1 is setup but not used
// SPI signals mapped to GPIO pins and interrupt set in Board_initSPI()
// SPI is setup in Board.c: setupSPI() as master, bitrate=100kHz, frameFormat =
// refer TI-RTOS UG: SPI Driver for API
// refer F28M35 Tech Ref Manual @ SPI for HW and frame formatting.
//								GPIO		   Signal			 DIMM         Daughter Board     ===== LogicPort ====
//                             Port/Bit                          Pin No.      Header PinName	Pin	No. 	 Colour
// =========  SPI Pinout ===============================		=================
//PA5_GPIO5    		A/5		SSI0Tx	    				75						PC4				D0		black/white
//PA2_GPIO2    		A/2		SSI0Clk			 		24						PB0				D1		brown/white
//PA3_GPIO3    		A/3		SSI0Fss	(LDAC)	74						PC7				D2		red/white						LDAC
//PA4_GPIO4			A/4		SSI0Rx					not used		--

// ========== GPIO input pins  ==========================================
// PB0_GPIO08 	B/0 		SCANXFAULT			28	 					PA0							Scanner X Fault
// PB1_GPIO09 	B/1 		SCANYFAULT			78   						PA1							Scanner Y Fault
// PB2_GPIO10		B/2		LASERFAULT			29	 					--								Laser Fault
// PB3_GPIO11	 	B/3		SPARE_IN1			79	 					--
// PB4_GPIO12	 	B/4		SPARE_IN2			33						--
// PB5_GPIO13	 	B/5		SPARE_IN3			83						--

// ========== GPIO output pins  ========================================
//  PC6  					C/6		on-board LED2			-						-
//  PC7  					C/7		on-board LED3			-						-
// PA0_GPIO00		A/0		SPARE_OUT1			23					PB1
// PA1_ GPIO01	A/1		~DAC_RST 				73					PB6							DAC reset

// PD4_GPIO20		D/4		LASER_PWR 				40					PB7
// PD5_GPIO21 	D/5		SPARE_OUT4			90  					PB4
// PD6_GPIO22 	D/6		SPARE_OUT5			41   					PB5
// PD7_GPIO23 	D/7		SPARE_OUT3			91					--

//--------------------------------------------------------------
// Earth: (8, 10, 12, 14, 58, 60, 62, 64)			8		9		Ground		grey/black

// status word bits
//#define SW_POWER_ON		true	//~logic
//#define SW_POWER_OFF		false



//F28M35x Tech Ref Manual Table 4-2 GPIO Pins gives each GPIO pin a number starting GPIO0

// from GPIO.h: Value is a bit representation of the pin(s) to be changed. Only the pins specified by index will be written.
// If the index is for pin 2, it  can be written high by passing in 0x04 or ~1 as the value. In general ~1  and 0
// should be used for single pins or multiple pins with the same value.
 //  Only when writing multiple pins with different values should you use a bit  represented value.

// I think the indexes are as identified in the GPIO enumeration (below).  This seems consistent with ....

// -----  GPIO control bit values --------
// LEDs are active low, NEGATIVE LOGIC
#define BOARD_LED_ON  					(0)
#define BOARD_LED_OFF 					(~0)

// positive logic pins
#define  GPIO_PIN_ON							(~0)
#define GPIO_PIN_OFF							(0)

// negative logic pins
#define GPIO_DAC_RESET_TRUE		(0)  //reset  = true means reset
#define GPIO_DAC_RESET_FALSE		(~0)  //reset  = true means reset

//#define GPIO_ON  		 					false
// refer http://e2e.ti.com/support/embedded/tirtos/f/355/t/307885 for index explanation
// The index eg 0x40 is the pin index into GPIO configuration structure, GPIO_HWAttrs gpioHWAttrs[]
//#define GPIO_SPARE_OUT1 		(0x04)		// GPIO_PORTA_PIN_0
//#define GPIO_DAC_RST 				(0x08)		// GPIO_PORTA_PIN_1
//#define GPIO_LASER_OFF 		(0x10)		// GPIO_PORTD_PIN_4
//#define GPIO_X_OFF						(0x20)		// GPIO_PORTD_PIN_5
////#define GPIO_Y_OFF						(0x40)		// GPIO_PORTD_PIN_6
#define GPIO_SPARE_OUT3					(0x80)		//GPIO_PORTD_PIN_7 is spare

#define FAULT					true
#define NO_FAULT  		false
#define ACK_OK				true
#define ACK_NOTOK	false


struct statusWord {
	UShort	serverFault		:	1;	// server has fault
	UShort 	laserFault		:	1;	// server fault
	UShort	scanXFault		:	1;	// server fault
	UShort	scanYFault		:	1;	// server fault
	UShort	unknownScannerFault	:	1;	// server fault
	UShort	hwStartFail		:	1;	// server hardware didn't start
	UShort 	laserPower		: 	1;	// client command
	UShort	dacReset			:	1;	// client command
	UShort	spiError				:	1;	// server sets on excessive spi errors
	UShort	verbose				:	1;	// client command
} ;

struct clientMssg {						// message recvd. from client
	UShort scanX;								//scanner X value
	UShort scanY;
	UShort info;									// send data client to server & vice versa
} ;

struct serverMssg {						// message sent to client
	UShort scanX;								//scanner X value
	UShort scanY;
	UShort info;
	struct statusWord status;
};

enum cmdMssg {CMD_LASER_OFF, CMD_LASER_ON,
			CMD_DAC_RESET_OFF, CMD_DAC_RESET_ON,
            CMD_SLEW, CMD_VERBOSE_ON, CMD_VERBOSE_OFF,
            CMD_NULL_MSSG, CMD_RESET } ;

int main(void);
void setupSPI(void);
int checkHardware(void);
extern void tcpWorker(UArg arg0, UArg arg1);
extern void tcpHandlerFxn(UArg arg0, UArg arg1);
extern void idleFxn(UArg arg0, UArg arg1);
extern int resetServer(void) ;


//===============  derived from TIRTOS  TMDXDOCKH52C1.h ===========
//

//Enum of EMAC names on the board
//
typedef enum EMACName {
   EMAC0 = 0,

   EMACCOUNT
}  EMACName;

//
// enumerate GPIO pins
// same order as in GPIO_PinConfig structure
typedef enum GPIOName {
    //input pins
	SCANXFAULT = 0,
    SCANYFAULT,
    LASERFAULT,
    SPARE_IN1,
    SPARE_IN2,
    SPARE_IN3,

	// output pins
	LED2,
    LED3,
    SPARE_OUT1	,
    DAC_RST,
    LASER_PWR,
    SPARE_OUT4,
    SPARE_OUT5,
    SPARE_OUT3,

    GPIOCOUNT
} GPIOName;

//
// enumerate SPI names
//
typedef enum SPIName {
	SPI0 = 0,
	SPI1,
//	SPI2,
//	SPI3,

    SPICOUNT
} SPIName;


// ************ APPEARS NOT NEEDED WITH NEW GPIO DRIVER ***************
//
// GPIO_Callbacks structure for GPIO interrupts
//
//extern const GPIO_Callbacks gpioPortBCallbacks;


extern void Board_initEMAC(void);
/*!
 *  @brief  Initialize the general board specific settings
 *
 *  This function initializes the general board specific settings. This include
 *     - Enable clock sources for peripherals
 *     - Disable clock source to watchdog module
 */
extern void Board_initGeneral(void);
/*!
 *  @brief  Initialize board specific GPIO settings
 *
 *  This function initializes the board specific GPIO settings and
 *  then calls the GPIO_init API to initialize the GPIO module.
 *
 *  The GPIOs controlled by the GPIO module are determined by the GPIO_config
 *  variable.
 */
extern void Board_initSPI(void);
/*!
 *  @brief  Initialize board specific UART settings
 *
 *  This function initializes the board specific UART settings and then calls
 *  the UART_init API to initialize the UART module.
 *
 *  The UART peripherals controlled by the UART module are determined by the
 *  UART_config variable.
 */
extern void Board_initGPIO(void);
/*!
 *  @brief  Initialize board specific I2C settings
 *
 *  This function initializes the board specific I2C settings and then calls
 *  the I2C_init API to initialize the I2C module.
 *
 *  The I2C peripherals controlled by the I2C module are determined by the
 *  I2C_config variable.
 */


#ifdef __cplusplus
}
#endif

#endif /* TCPECHO_H_ */
