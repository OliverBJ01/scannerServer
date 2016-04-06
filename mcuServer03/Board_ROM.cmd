/* ======== Board.cmd ========
  *  Modification of TMDXDOCKH52C1.cmd to support  NOLOAD
  Flash is partitioned to load stable code into Hi flash, and developing
  code into Lo flash.  Once stable code is loaded, adjust flash utility to
  not erase/load except into Lo flash, and using NOLOAD setting, just
  update developing code in Lo flash.
  (requires use of  #pragma CODE_SECTION ( ".devcode"); prior to
  functions to go into Lo flash.

  Step 1.  Place "#pragma CODE_SECTION ( <fn name>, ".devcode")"
  						in front of developmental fns
  Step 2
   -ar
   - set flash util to erase and load all flash
   - uncomment  ".text  : > FLASH_HIGH" // loads entire application
	- run debug
  Step 3.
   - set flash util to erase and load Lo flash
   - uncomment  ".text : NOLOAD	> FLASH_HIGH" // ignores text in Hi flash
	- run debug
*/

// compiles EABI


// Load to FLASH_HI (ROM)
//-a     			// guarantee no refs to syms outside ROM
//-r    				 // partial link - will link again
//Board.obj tcpFunctions.obj //mcuServer.obj
//-o rom.out					// to load into FLASH_HIGH (ROM)

MEMORY
{
    BOOTROM (RX)    : origin = 0x0,        length = 0x10000
    FLASH_BOOT (RWX): origin = 0x200030,   length = 0x4
 //   FLASH (RWX)     : origin = 0x200034,   length = 0x7FF9C
 
//  partition flash into high 'ROM' area and low 'develop' area
    FLASH (RWX)  					 : origin = 0x200034,   length = 0xffcb		//Sectors J-A
    FLASH_HIGH (RWX)     : origin = 0x210000,   length = 0x6ffff		//Sectors N-K

    C03SRAM (RWX)   			: origin = 0x20000000, length = 0x8000
    S07SHRAM (RWX) 		 : origin = 0x20008000, length = 0x10000
    CTOMMSGRAM (R) 		 : origin = 0x2007F000, length = 0x800
    MTOCMSGRAM (RW) 	: origin = 0x2007F800, length = 0x800
}

SECTIONS
{
// Allocate initalized data sections:
//  .text (compiler default) loads into FLASH_HIGH, my devt code into low flash
 // use #pragma CODE_SECTION (<fn name>, ".devcode")  before a function to locate it in (low) FLASH

//	.text     : type=NOLOAD	> FLASH_HIGH	// only loads devt code
//	.const     : type=NOLOAD	> FLASH_HIGH	// only loads devt code

	rom_sect		 : 		{
								 *  (  .text  )
								 *  (  .const )
								 		}   > FLASH_HIGH

	 .devcode	 : 		> FLASH		// my development code into low flash
  //   .text     : 			> FLASH
    .binit      :   			> FLASH		//special C/C++ copy table
    .cinit      : 			> FLASH		// data to init variables and constants
    .pinit      : 			> FLASH		// global constructor tables

      //  Initialized sections go in Flash
    .const      : 			> FLASH		// strings and data qualified with 'const'

    // Allocate uninitalized data sections:
    // UG says .data for EABI this is initialised data
    .data       : 			> S07SHRAM									//??????? UG says initialised global and staic variables (EABI only)

    .bss        : 			>> C03SRAM | S07SHRAM	// space for uninit. global and static variables
    .dma        : 			> S07SHRAM
    .sysmem     : 	> C03SRAM									//space for dynamic memory
    .stack      : 			> S07SHRAM
    .cio        : 				> C03SRAM
    .neardata   : 	> C03SRAM
    .rodata     : 		> C03SRAM
    .args       : 			> C03SRAM
}

__STACK_TOP = __stack + 256;
