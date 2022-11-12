# Introduction:
Firmware to support gs_usb on most STM32 devices.  The board specific code can be autogenerated by CubeMX and copied over to your own directory in the ./Portable directory.

The goal of this project was to create a stable foundation for gs_usb while allowing developers to create custom software to support their boards.

The core functionality is in the ./Core directory and all the custom code for each board is in the ./Portable directory.

Using this driver you can create your own custom boards while utilizing the gs_usb core.  

Core functionality includes:
- USB connectivity to the gs_usb driver as implemented in the Linux kernel (support is currently matching kernel 6.1rc1 but is backwards compatible with all pervious kernels that support gs_usb).  This functionality is maintained as stable even as the developer creates custom code.
- Support for bxCAN (STMF0, STMF4, etc.)
- Support for FDCAN (STMG0, STMG4, STMH7, STML5, STMU5)
- Support for LIN (using any UART available on the STM32)
- Support for the custom code to take action on messages received on the bus or transmitted by the host.  The developer can also inject messages onto the bus in an adhoc manner based on their own custom code (e.g. a GPIO triggers the transmission of a message on the CAN bus or triggers a message back to the host).

Since the core codebase uses FreeRTOS you can create your own tasks/timers/queues/etc as needed to develop custom features for your board.

I have included a few examples to be used as templates.

<br>
<br>

# How to create your own board:
1. Pick one of the examples that best matches your desired board.  The only files that need to be hand modified are the board.c and board.h.  Everything else will be copied from the CubeMX generated files.
2. Create a project in CubeMX based on the chipset you are using and the GPIO and/or additional peripherals you plan on using.
3. **This is very important:**  You must ensure you select the following to ensure the files are generated correctly by CubeMX.
    - You select CAN and USB under the "Connectivity" heading on the left.  You must also enable the NVIC for all CAN channels and the USB.
    - Enable FreeRTOS under the "Middlewares" heading and select any interface.  This ensures that the correct NVIC priorities are set and code is not generated for the IRQs that FreeRTOS uses.
    - Enable TIM2 by changing Clock Source -> Internal Clock.
    - Under the "SYS" menu change the Timebase Source to TIM1.
    - Any other configuration (adding UARTs, I2C, etc.) is up to you.  You will need to determine where to place the autogenerated code for those peripherals.
4. In the project make sure you select Makefile in your toolchain selection then Generate Code.
5. The following files need to be copied over to the Portable/Inc and Portable/Src directories:
    - To Inc (XX matches your chip family)
        - stm32XXxx_hal_conf.h
        - stm32XXxx_it.h
    - To Src (XX matches your chip family)
        - stm32XXxx_hal_msp.c
        - stm32XXxx_hal_timebase_tim.c
        - stm32XXxx_it.c
        - system_stm32XXxx.c
6. Open the main.c in the autogenerated code:
    - Cut and paste the MX_GPIO_Init and SystemClock_Config functions into your board.c file (overwrite what's already in the template).
7. Open the main.h in the autogenerated code:
    - Cut and paste the "#include "stm32XXxx_hal.h"" line and paste it into the board_hal_config.h in you board's "Inc" directory replacing what may already be in the template.
    - Cut and paste any of the GPIO defines and place them into your board.h.
8. Copy the startup_stm32xxxxxxx.s file from the autogenerated ./Core/Startup directory into the root of your board directory.
9. Copy the STM32xxxxxxx_FLASH.ld file from the root of the autogenerated code into the root of your board directory.
10. Edit anything in the board.c, board.h that needs to be changed to match your board's configuration.
11. Edit anything in the FreeRTOSConfig.h that you may need to tailor for your board.
11. Edit the Makefile in your board's root directory to update the names of the startup and linker files you copied over.  If your chipset doesn't match the template then you will need to update the chipset family ID on source and include files.
12. Compile and fix stuff you didn't update correctly.


<br>
<br>

# How to use the LIN driver:
https://www.csselectronics.com/pages/lin-bus-protocol-intro-basics

The LIN driver is configured using "pseudo" CAN messages sent from the host.  Since LIN data can be up to 8-bytes long we need to split the configuration into two CAN messages to properly configure.
As currently designed the LIN driver can operate in either Slave or Monitor mode depending on the configuration.  With monitor mode the developer has the ability to assign a "gateway" CAN message that is sent from the device to the host which contains data received on LIN (up to a maximum of 6 data bytes with the current non-FD design).

By default there are 10 slots per channel in the schedule table for use in slave or monitor mode.  A matching PID in slave mode will trigger the transmission of the data contained in that slot.  A PID match in 
The configuration method uses CANIDs that are configurable by the developer to best fit their design and use cases.  The example below uses the default CANDIDs used by the budgetcan_g0 firmware.


Slot table design:

|CH #|Index|PID|Len|Data0|Data1|...|DataN|
|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|
| 0x00 | 0x00 | 0x00 | 0x02 | 0xDE | 0xAD | ... | 0x00 |
| 0x00 | 0x00 | 0x01 | 0x01 | 0xBE | 0x00 | ... | 0x00 |
| xxx | xxx | xxx | xxx | xxx | xxx | ... | xxx |


Loading the slot is a two message process.  The first message sent is the data bytes you wish to be loaded into the slot.  This data will be held in the buffer until a load command is issued.  Therefore there are two messages: Command message and Data message.

The data message is simply a CAN frame with a DLC of 8 that contains the 8 bytes of data you wish to load into the table

The command message has the following structure:
Byte 0: Command type - 0 = Load slot, 1 = Reserved, 2 = Enable slot, 3 = Disable slot, 4 = Disable all slots, 5 = Erase all slots 
Byte 1: Channel - 0-255 : Developer may chose to have multiple LIN channels.  This value defines which channel the command will be carried out on.
Byte 2: Slot index - 0-255: By default there are 10 slots but a developer may choose to offer as many as 255.  This value defines which slot the command will be carried out on.
Byte 3: Slot flags - Bit 0 - 1 = Slot is active, 0 = Slot is inactive : Bits 1-7 = Reserved
Byte 4: Slot action - Bits 0:1 - 00 = Slave mode, 01 = Monitor mode, 10 & 11 = Reserved, Bits 2-7 = Reserved
Byte 5: PID : The PID that triggers a match on this slot
Byte 6: Length : The amount of data that will be returned in slave mode (0-8)

The following is an example of setting up slot 0 on channel 0 to trigger a slave response on PID $02 with 4 bytes of data (0x00, 0x00, 0x60, 0x40)

Setting up the data portion of the slot:

|CANID|DLC|B1|B2|B3|B4|B5|B6|B7|B8|
|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|
|0x1FFFFE81|0x08|0x00|0x00|0x60|0x40|0x00|0x00|0x00|0x00|0x00|

Loading the slot table with the data provided above and immediately start responding:

|CANID|DLC|Channel|Index|Flags|ActionID|PID|Len|xx|xx|
|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|
|0x1FFFFE80|0x08|0x00|0x00|0x01|0x00|0x02|0x04|0x00|0x00|

When the table is already loaded you can activate and inactivate that individual slot

The following is an example of setting a speicifc slot inactive (Channel 0, Index 2)
|CANID|DLC|Channel|Index|Flags|ActionID|PID|Len|xx|xx|
|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|
|0x1FFFFE80|0x08|0x00|0x02|0x00|0x00|0x00|0x00|0x00|0x00|

The following is an example of setting a speicifc slot active (Channel 0, Index 2)
|CANID|DLC|Channel|Index|Flags|ActionID|PID|Len|xx|xx|
|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|
|0x1FFFFE80|0x08|0x00|0x02|0x01|0x00|0x00|0x00|0x00|0x00|


Using these slot tables there are a variety of ways to carry out tasks on the LIN bus.  If you wish to perform a slave action (e.g. simulate a switch press) you could quickly load new data into the table with the steps above and the next slave request will respond with the newly loaded data.  Or you may chose to load all possible slave response scenarios into different slots and enable and disable those slots with a single command message.  The driver is designed to be as flexible as possible to preform many different LIN tasks.

Currently the driver does not support master frames.  I may be added at a later date depending on interest.

I may update the configuration system at a later date to use longer CANFD frame for loading the tables and sending monitor data.


