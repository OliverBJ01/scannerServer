// board.c
// this file was derived from TIRTOS/.../TMDXDOCKH52C1.c

// note board MAC addess is coded into EMAC routine

#include <inc/hw_memmap.h>
#include <inc/hw_types.h>
#include <inc/hw_ints.h>
#include <inc/hw_gpio.h>
#include <inc/hw_sysctl.h>

#include <driverlib/gpio.h>
#include <driverlib/sysctl.h>
#include <driverlib/ssi.h>
#include <driverlib/udma.h>

#include <xdc/std.h>
#include <xdc/cfg/global.h>
#include <xdc/runtime/Error.h>
#include <xdc/runtime/System.h>
#include <ti/sysbios/family/arm/m3/Hwi.h>

// TI-RTOS Header files
#include <ti/drivers/EMAC.h>
#include <ti/drivers/emac/EMACTiva.h>
#include <ti/drivers/GPIO.h>
#include <ti/drivers/SPI.h>
#include <ti/drivers/spi/SPITivaDMA.h>

#include "mcuServer.h"

// locate in dma able areaa for SPI / MessageQ msgs
#pragma DATA_SECTION(TxBuffer, ".dma");
 UShort    TxBuffer[2];			// Tx  buffer,   SPI expects UShort frame sizes 9 - 16 bits
 UShort    RxBuffer[2];			// not used

 // Global SPI structures, handle
SPI_Transaction SpiTransaction;	// SPI transaction structure
SPI_Params SpiParams;						// SPI parameter structure
SPI_Handle spi;



//============================setupSPI()=====================
// setupSPI() code does not come from TMDXDOCKH52C1.c
// called from main()
void setupSPI(void)  {

    // ===================setup SPI
	// refer F28M35 Tech Ref Manual @ SPI for frame format diagram
    SPI_Params_init(&SpiParams);		    // init SPI parameter structure
    SpiParams. bitRate   = 100000;  			// Hz
    SpiParams. dataSize  = 16;						// SPI driver expects UShort data for 9-16 bits
    SpiParams. frameFormat  = SPI_POL1_PHA1;      // refer F28M35 Tech Ref Manual @ SPI for frame formats
    //  remaining defaults  are: mode = SPI_MASTER, transferMode = SPI_MODE_BLOCKING
    // transferCallbackFxn = NULL.

    spi = SPI_open(SPI0,  &SpiParams);	    // Initialize SPI handle as default master
    //spi = SPI_open(SPI0,  NULL);
    if (spi == NULL)
        System_abort("Error initializing SPI\n"); System_flush();

    // Initialize  SPI transaction structure
    SpiTransaction.count = 2;								// 2 frames of 16 bits
    SpiTransaction.txBuf = (Ptr)TxBuffer;
    SpiTransaction.rxBuf = NULL;						// SPI discards all frames received
}
//========================== end  setupSPI()=====================


//  ========================  EMAC ) ============================
 //
//  Place into subsections to allow the TI linker to remove items properly
#pragma DATA_SECTION(EMAC_config, ".const:EMAC_config")
#pragma DATA_SECTION(emacHWAttrs, ".const:emacHWAttrs")
#pragma DATA_SECTION(NIMUDeviceTable, ".data:NIMUDeviceTable")

#include <ti/drivers/EMAC.h>
#include <ti/drivers/emac/EMACTiva.h>

// EMAC objects
EMACTiva_Object emacObjects[EMACCOUNT];

// B Oliver's Board MAC Address
unsigned char macAddress[6] = {0xA8, 0x63, 0xf2, 0x00, 0x11, 0x95};

//  EMAC configuration structure
const EMACTiva_HWAttrs emacHWAttrs[EMACCOUNT] = {
    {INT_ETH, ~0 /* Interrupt priority */, macAddress}
};

const EMAC_Config EMAC_config[] = {
    {&EMACTiva_fxnTable, emacObjects, emacHWAttrs},   // pointer into prev structure
    {NULL, NULL, NULL}
};

//  Required by the Networking Stack (NDK). This array must be NULL terminated.
// This can be removed if NDK is not used.
// Double curly braces are needed to avoid GCC bug #944572
// https://bugs.launchpad.net/gcc-linaro/+bug/944572
NIMU_DEVICE_TABLE_ENTRY  NIMUDeviceTable[2] = {{EMACTiva_NIMUInit},  {NULL}};

//  =========================== initEMAC
void Board_initEMAC(void)
{
    /*
     *  Set up the pins that are used for Ethernet
     *  MII_TXD3
     */
    GPIODirModeSet(GPIO_PORTC_BASE, GPIO_PIN_4, GPIO_DIR_MODE_HW);
    GPIOPadConfigSet(GPIO_PORTC_BASE, GPIO_PIN_4, GPIO_PIN_TYPE_STD);
    GPIOPinConfigure(GPIO_PC4_MIITXD3);

    /* MII_MDIO */
    GPIODirModeSet(GPIO_PORTE_BASE, GPIO_PIN_6, GPIO_DIR_MODE_HW);
    GPIOPadConfigSet(GPIO_PORTE_BASE, GPIO_PIN_6, GPIO_PIN_TYPE_STD);
    GPIOPinConfigure(GPIO_PE6_MIIMDIO);

    /* MII_RXD3 */
    GPIODirModeSet(GPIO_PORTF_BASE, GPIO_PIN_5, GPIO_DIR_MODE_HW);
    GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_5, GPIO_PIN_TYPE_STD);
    GPIOPinConfigure(GPIO_PF5_MIIRXD3);

    /* MII_TXER , MII_RXDV , MII_RXD1 , MII_RXD2 */
    GPIODirModeSet(GPIO_PORTG_BASE, GPIO_PIN_7|GPIO_PIN_3|GPIO_PIN_1|GPIO_PIN_0,
                   GPIO_DIR_MODE_HW);
    GPIOPadConfigSet(GPIO_PORTG_BASE, GPIO_PIN_7|GPIO_PIN_3|GPIO_PIN_1|
                     GPIO_PIN_0,
                     GPIO_PIN_TYPE_STD);
    GPIOPinConfigure(GPIO_PG0_MIIRXD2);
    GPIOPinConfigure(GPIO_PG1_MIIRXD1);
    GPIOPinConfigure(GPIO_PG3_MIIRXDV);
    GPIOPinConfigure(GPIO_PG7_MIITXER);

    /* MII_TXCK , MII_TXEN , MII_TXD0 , MII_TXD1 , MII_TXD2 , MII_RXD0 */
    GPIODirModeSet(
        GPIO_PORTH_BASE, GPIO_PIN_7|GPIO_PIN_6|GPIO_PIN_5|GPIO_PIN_4|
        GPIO_PIN_3|
        GPIO_PIN_1, GPIO_DIR_MODE_HW);
    GPIOPadConfigSet(
        GPIO_PORTH_BASE, GPIO_PIN_7|GPIO_PIN_6|GPIO_PIN_5|GPIO_PIN_4|
        GPIO_PIN_3|
        GPIO_PIN_1, GPIO_PIN_TYPE_STD);
    GPIOPinConfigure(GPIO_PH1_MIIRXD0);
    GPIOPinConfigure(GPIO_PH3_MIITXD2);
    GPIOPinConfigure(GPIO_PH4_MIITXD1);
    GPIOPinConfigure(GPIO_PH5_MIITXD0);
    GPIOPinConfigure(GPIO_PH6_MIITXEN);
    GPIOPinConfigure(GPIO_PH7_MIITXCK);

    /*
     *  MII_PHYRSTn , MII_PHYINTRn , MII_CRS , MII_COL , MII_MDC , MII_RXCK ,
     *  MII_RXER
     */
    GPIODirModeSet(
        GPIO_PORTJ_BASE, GPIO_PIN_7|GPIO_PIN_6|GPIO_PIN_5|GPIO_PIN_4|
        GPIO_PIN_3|
        GPIO_PIN_2|GPIO_PIN_0, GPIO_DIR_MODE_HW);
    GPIOPadConfigSet(
        GPIO_PORTJ_BASE, GPIO_PIN_7|GPIO_PIN_6|GPIO_PIN_5|GPIO_PIN_4|
        GPIO_PIN_3|
        GPIO_PIN_2|GPIO_PIN_0, GPIO_PIN_TYPE_STD);
    GPIOPinConfigure(GPIO_PJ0_MIIRXER);
    GPIOPinConfigure(GPIO_PJ2_MIIRXCK);
    GPIOPinConfigure(GPIO_PJ3_MIIMDC);
    GPIOPinConfigure(GPIO_PJ4_MIICOL);
    GPIOPinConfigure(GPIO_PJ5_MIICRS);
    GPIOPinConfigure(GPIO_PJ6_MIIPHYINTRn);
    GPIOPinConfigure(GPIO_PJ7_MIIPHYRSTn);

    /* Enable and Reset the Ethernet Controller. */
    SysCtlPeripheralEnable(SYSCTL_PERIPH_ETH);
    SysCtlPeripheralReset(SYSCTL_PERIPH_ETH);

    if (macAddress[0] == 0xff && macAddress[1] == 0xff &&
        macAddress[2] == 0xff && macAddress[3] == 0xff &&
        macAddress[4] == 0xff && macAddress[5] == 0xff) {
        System_abort("Change the macAddress variable to match your board's MAC sticker");
    }

    /* Once EMAC_init is called, EMAC_config cannot be changed */
    EMAC_init();
}
//  =========================end initEMAC =======================



//  ======================= DMA ================================
// SPI uses DMS
#pragma DATA_SECTION(dmaControlTable, ".dma");
#pragma DATA_ALIGN(dmaControlTable, 1024)
static tDMAControlTable dmaControlTable[32];
static bool DMA_initialized = false;

// Hwi_Struct used in the initDMA Hwi_construct call
static Hwi_Struct hwiStruct;

//  ======== dmaErrorHwi ========
//
static Void dmaErrorHwi(UArg arg)
{
    System_printf("DMA error code: %d\n", uDMAErrorStatusGet());
    uDMAErrorStatusClear();
    System_abort("DMA error!!");
}

//======= initDMA  ===========
void initDMA(void)
{
    Error_Block eb;
    Hwi_Params  hwiParams;

    if(!DMA_initialized){

        Error_init(&eb);

        Hwi_Params_init(&hwiParams);
        Hwi_construct(&(hwiStruct), INT_UDMAERR, dmaErrorHwi,
                      &hwiParams, &eb);
        if (Error_check(&eb)) {
            System_abort("Couldn't create DMA error hwi");
        }

        SysCtlPeripheralEnable(SYSCTL_PERIPH_UDMA);
        uDMAEnable();
        uDMAControlBaseSet(dmaControlTable);

        DMA_initialized = true;
    }
}
//  ========================== end DMA ============================


//  ========================= SPI ==================================
// derived from TIRTOS's TMDXDOCKH52C1.c
// Place into subsections to allow the TI linker to remove items properly
#pragma DATA_SECTION(SPI_config, ".const:SPI_config")
#pragma DATA_SECTION(spiTivaDMAHWAttrs, ".const:spiTivaDMAHWAttrs")

#include <ti/drivers/SPI.h>
#include <ti/drivers/spi/SPITivaDMA.h>

// SPI objects
SPITivaDMA_Object spiTivaDMAObjects[SPICOUNT];
#pragma DATA_SECTION(spiTivaDMAscratchBuf, ".dma");
#pragma DATA_ALIGN(spiTivaDMAscratchBuf, 32)
uint32_t spiTivaDMAscratchBuf[SPICOUNT];


// SPI configuration structure, describing which pins are to be used
const SPITivaDMA_HWAttrs spiTivaDMAHWAttrs[SPICOUNT] = {
    {
        SSI0_BASE,
        INT_SSI0,
		~0,			// Interrupt priority
        &spiTivaDMAscratchBuf[0],
        0,
        UDMA_CHANNEL_SSI0RX,
        UDMA_CHANNEL_SSI0TX,
        uDMAChannel8_15SelectDefault,
        UDMA_CHAN10_DEF_SSI0RX_M,
        UDMA_CHAN11_DEF_SSI0TX_M
    },
    {
        SSI1_BASE,
        INT_SSI1,
		~0,			// Interrupt priority
        &spiTivaDMAscratchBuf[1],
        0,
        UDMA_CHANNEL_SSI1RX,
        UDMA_CHANNEL_SSI1TX,
        uDMAChannel24_31SelectDefault,
        UDMA_CHAN24_DEF_SSI1RX_M,
        UDMA_CHAN25_DEF_SSI1TX_M
    },
   /* {
        SSI2_BASE,
        INT_SSI2,
        ~0,			// Interrupt priority
        &spiTivaDMAscratchBuf[2],
        0,
        UDMA_THRD_CHANNEL_SSI2RX,
        UDMA_THRD_CHANNEL_SSI2TX,
        uDMAChannel8_15SelectAltMapping,
        UDMA_CHAN12_THRD_SSI2RX,
        UDMA_CHAN13_THRD_SSI2TX
    },
    {
        SSI3_BASE,
        INT_SSI3,
        ~0,			// Interrupt priority
        &spiTivaDMAscratchBuf[3],
        0,
        UDMA_THRD_CHANNEL_SSI3RX,
        UDMA_THRD_CHANNEL_SSI3TX,
        uDMAChannel8_15SelectAltMapping,
        UDMA_CHAN14_THRD_SSI3RX,
        UDMA_CHAN15_THRD_SSI3TX
    }*/
};

const SPI_Config SPI_config[] = {
    {&SPITivaDMA_fxnTable, &spiTivaDMAObjects[0], &spiTivaDMAHWAttrs[0]},
    {&SPITivaDMA_fxnTable, &spiTivaDMAObjects[1], &spiTivaDMAHWAttrs[1]},
  /*  {&SPITivaDMA_fxnTable, &spiTivaDMAObjects[2], &spiTivaDMAHWAttrs[2]},
    {&SPITivaDMA_fxnTable, &spiTivaDMAObjects[3], &spiTivaDMAHWAttrs[3]}, */
    {NULL, NULL, NULL},
};

//  ======== initSPI ========
void Board_initSPI(void)
{
    /* SSI0 */
    SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI0);

    GPIOPinConfigure(GPIO_PA2_SSI0CLK);
    GPIOPinConfigure(GPIO_PA3_SSI0FSS);
    GPIOPinConfigure(GPIO_PA4_SSI0RX);
    GPIOPinConfigure(GPIO_PA5_SSI0TX);

    GPIOPinTypeSSI(GPIO_PORTA_BASE,  GPIO_PIN_2 | GPIO_PIN_3 |
                                    															GPIO_PIN_4 | GPIO_PIN_5);

    /* SSI1 */
    SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI1);

    GPIOPinConfigure(GPIO_PE0_SSI1CLK);
    GPIOPinConfigure(GPIO_PE1_SSI1FSS);
    GPIOPinConfigure(GPIO_PE2_SSI1RX);
    GPIOPinConfigure(GPIO_PE3_SSI1TX);

    GPIOPinTypeSSI(GPIO_PORTE_BASE, 	GPIO_PIN_0 | GPIO_PIN_1 |
                                    															GPIO_PIN_2 | GPIO_PIN_3);
    initDMA();
    SPI_init();
}
//  ========================= end SP I============================

//
//  ==========================  GPIO =============================
//
//   Place into subsections to allow the TI linker to remove items properly
#pragma DATA_SECTION(GPIOTiva_config, ".const:GPIOTiva_config")

#include <ti/drivers/GPIO.h>
#include <ti/drivers/gpio/GPIOTiva.h>

// Array of Pin configurations
// NOTE: The order of the pin configurations must coincide with what was
//       defined in TMDXDOCKH52C1.h
// NOTE: Pins not used for interrupts should be placed at the end of the
//       array.  Callback entries can be omitted from callbacks array to
//      reduce memory usage.

// many pin configs: Open drain, etc refer GPIO.h
// WPU (weak pull up resistor) sets Open Drain Configuration with WPU
// 	GPIO_CFG_IN_PU / PD  Input pin has Pullup / Pulldown

// Remote shutdown must be implemented as Open Collector circuit??

GPIO_PinConfig gpioPinConfigs[] = {
		// == Input pins
		//  SCANXFAULT
		GPIOTiva_PB_0 | GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_RISING,
		//  SCANYFAULT
		GPIOTiva_PB_1 | GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_RISING,
		//  LASERFAULT
		GPIOTiva_PB_2 | GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_RISING,
		//  SPARE_IN1
		GPIOTiva_PB_3 | GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_NONE,
		//  SPARE_IN2
		GPIOTiva_PB_4 | GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_NONE,
		//  SPARE_IN3
		GPIOTiva_PB_5 | GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_NONE,

	    // == Output pins
		//  on-card LED2
		GPIOTiva_PC_6 | GPIO_CFG_OUT_STD | GPIO_CFG_OUT_STR_HIGH | GPIO_CFG_OUT_LOW,
	    //  on-card LED3
		GPIOTiva_PC_7 | GPIO_CFG_OUT_STD | GPIO_CFG_OUT_STR_HIGH | GPIO_CFG_OUT_LOW,
	    //  SPARE_OUT1
		GPIOTiva_PA_0 | GPIO_CFG_OUT_STD | GPIO_CFG_OUT_STR_HIGH | GPIO_CFG_OUT_LOW,
		//  DAC_RST
		GPIOTiva_PA_1 | GPIO_CFG_OUT_STD | GPIO_CFG_OUT_STR_HIGH | GPIO_CFG_OUT_LOW,
	    //  LASER_PWR
		GPIOTiva_PD_4 | GPIO_CFG_OUT_STD | GPIO_CFG_OUT_STR_HIGH | GPIO_CFG_OUT_LOW,
	    //  SPARE_OUT4
		GPIOTiva_PD_5 | GPIO_CFG_OUT_STD | GPIO_CFG_OUT_STR_HIGH | GPIO_CFG_OUT_LOW,
	    //  SPARE_OUT5
		GPIOTiva_PD_6 | GPIO_CFG_OUT_STD | GPIO_CFG_OUT_STR_HIGH | GPIO_CFG_OUT_LOW,
	    //  SPARE_OUT3
		GPIOTiva_PD_7 | GPIO_CFG_OUT_STD | GPIO_CFG_OUT_STR_HIGH | GPIO_CFG_OUT_LOW,
};

// SPI0 uses: PA2(02), PA3(03), PA4(04), PA5(05).
// SPI1 uses:  PE0(GPIO24), PE1(GPIO25), PE2(26), PE3(27), PE4(GPIO28),   could use these
// EMAC uses: PC4(, PC6, PF5, PG7, PH7, PJ7,

// Array of callback function pointers
// NOTE: The order of the pin configurations must coincide with what was
//       defined in TMDXDOCKH52C1.h
// NOTE: Pins not used for interrupts can be omitted from callbacks array to
//       reduce memory usage (if placed at end of gpioPinConfigs array).

GPIO_CallbackFxn gpioCallbackFunctions[] = {
    NULL,   	//SCANXFAULT input pin
	NULL		//SCANYFAULT input pin
};

// The device-specific GPIO_config structure
const GPIOTiva_Config GPIOTiva_config = {
    .pinConfigs = (GPIO_PinConfig *) gpioPinConfigs,
    .callbacks = (GPIO_CallbackFxn *) gpioCallbackFunctions,
    .numberOfPinConfigs = sizeof(gpioPinConfigs) / sizeof(GPIO_PinConfig),
    .numberOfCallbacks = sizeof(gpioCallbackFunctions)/sizeof(GPIO_CallbackFxn),
    .intPriority = (~0)
};


//
//======== Board_initGPIO ========
void Board_initGPIO(void)
{
	// Initialize peripheral and pins
    GPIO_init();

    GPIO_write(LED2, BOARD_LED_OFF);
    GPIO_write(LED3, BOARD_LED_OFF);
    //+++++++++++++++ and for other pins? ================
}
//  ==========================  end GPIO ============================



/*
 *  ======== TMDXDOCKH52C1_initGeneral ========
 */
void Board_initGeneral(void)
{
    /* Disable Protection */
    HWREG(SYSCTL_MWRALLOW) =  0xA5A5A5A5;

    /* Enable clock supply for the following peripherals */
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOG);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOH);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOJ);

    /* Disable clock supply for the watchdog modules */
    SysCtlPeripheralDisable(SYSCTL_PERIPH_WDOG1);
    SysCtlPeripheralDisable(SYSCTL_PERIPH_WDOG0);
}








