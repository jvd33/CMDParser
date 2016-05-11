//-------------------------------------------------------------------------
// uart.c
// This program implements an interrupt driven RPI serial receiver and
// normal transmitter.
// 07/07/2013 - Initial version
// 10/10/2013 - updated comments, double echo the output
// 11/07/2013 - update BAUD rate
// 11/25/2013 - Added decStr()
// 03/08/2014 - Use Transmitter idle & empty + delay
// 05/14/2014 - Added dummy() 
// 05/15/2014 - Removed mmio_write
// 09/07/2014 - Add delay loop in putStr() to prevent Putty crashes
// 12/15/2014 - Added a command line buffer and lab 13
//-------------------------------------------------------------------------

// #define LAB_13 1

#include <stdint.h>
#include "cmpe240.h"

#define XMIT_SLOWDOWN   3000

/*---------------------------------------------------------------------------
  Interrupt handler variables 
---------------------------------------------------------------------------*/
#define  RXBUFMASK 0xFFF
volatile uint32_t rxhead;
volatile uint32_t rxtail;
volatile unsigned char rxbuffer[RXBUFMASK+1];
char cmdLine [100];




/*---------------------------------------------------------------------------
  Initialize the UART
---------------------------------------------------------------------------*/
void uart_init(void)
   {
    uint32_t temp;
    uint32_t *ptr;

    // Disable interrupts mmio_write(IRQ_DISABLE1,1<<29);
    ptr = (uint32_t *)IRQ_DISABLE1;
    *ptr = 1<<29;

    // Enable the Auxiliary peripherals mio_write(AUX_ENABLES,1);
    ptr = (uint32_t *)AUX_ENABLES;
    *ptr = 1;

    // Mini UART Interrupt Enable
    // 31:8 Reserved, write zero, 
    // 7:6  FIFO      enables Both bits 
    // 5:3          Always zero
    // 2:1  READ:  Interrupt ID bits
    //             00 : No interrupts, 01 : Transmit holding register empty
    //             10 : Receiver holds valid byte, 11 : <Not possible>
    //      WRITE: FIFO clear bits
    //      write: Writing with bit 1 set will clear the receive FIFO
    //             Writing with bit 2 set will clear the transmit FIFO
    // mmio_write(AUX_MU_IER_REG,0);
    ptr = (uint32_t *)AUX_MU_IER_REG;
    *ptr = 0;

    // Mini UART Extra Control
    // 31:8 Reserved, write zero
    // 7    CTS assert,   Allows one to invert the CTS auto flow polarity.
    // 6    RTS assert,   Allows one to invert the RTS auto flow polarity.
    // 5:4  RTS AUTO flow, 00 : De-assert RTS when the receive FIFO has 3 spaces left.
    //                     01 : De-assert RTS when the receive FIFO has 2 spaces left.
    //                     10 : De-assert RTS when the receive FIFO has 1 spaces left.
    //                     11 : De-assert RTS when the receive FIFO has 4 spaces left.
    // 3    Enable transmit Auto flow control using CTS
    // 2    Enable receive Auto flow control using RTS
    // 1    Transmitter enable
    // 0    Receiver enable
    // mmio_write(AUX_MU_CNTL_REG,0);
    ptr = (uint32_t *)AUX_MU_CNTL_REG;
    *ptr = 0;

    // Mini UART Line Control (start/stop bits)  
    // 31:8 Reserved, write zero, read as don’t care
    // 7    DLAB access 
    // 6    Break  
    // 5:1  reserved     bit 1 must be 1  
    // 0    data size   0= 7-bit, 1= 8-bit mode
    // mmio_write(AUX_MU_LCR_REG,3);   // 8 bit mode
    ptr = (uint32_t *)AUX_MU_LCR_REG;
    *ptr = 3;

    // Mini UART Modem Control
    // 31:8 Reserved, write zero, read as don’t care
    // 7:2  Reserved, write zero, Some of these bits have functions in a 16550
    //                but are ignored here
    // 1    RTS       If clear the UART1_RTS line is high
    //                If set   the UART1_RTS line is low
    //                Ignored if the RTS is used for auto-flow
    // 0    Reserved, write zero,  function in a 16550 UART but ignored
    // mmio_write(AUX_MU_MCR_REG,0);
    ptr = (uint32_t *)AUX_MU_MCR_REG;
    *ptr = 0;

    // Mini UART Interrupt Enable
    // 31:8 Reserved, write zero, r
    // 7:6  FIFO      enables Both bits 
    // 5:3          Always zero
    // 2:1  READ:  Interrupt ID bits
    //             00 : No interrupts, 01 : Transmit holding register empty
    //             10 : Receiver holds valid byte, 11 : <Not possible>
    //      WRITE: FIFO clear bits
    //      write: Writing with bit 1 set will clear the receive FIFO
    //             Writing with bit 2 set will clear the transmit FIFO
    // 0    Interrupt pending  - Cleared when pending
    // 5 = 0000 0000 0000 0000 0000 0000 0000 0101
    // mmio_write(AUX_MU_IER_REG,0x5);    //enable rx interrupts
    ptr = (uint32_t *)AUX_MU_IER_REG;
    *ptr = 0x5;

    // Mini UART Interrupt Identify
    //31:8 Reserved, write zero, 
    //7:0 MS 8 bits Baudrate  if DLAB=1, then access to the MS 8 bits of the 16-bit 
    //                                    baud rate register.
    //7:2 reserved            if DLAB=2, write zero,
    //1 Enable receive interrupt, (DLAB=0)
    //                 If this bit is clear no receive interrupts are generated.
    //0 Enable transmit interrupt, (DLAB=0)
    //                 If this bit is set the interrupt line is asserted whenever
    //
    //   0xc6 = 0000 0000 0000 0000 0000 0000 0110 1100
    //  
    // mmio_write(AUX_MU_IIR_REG,0xC6);
    ptr = (uint32_t *)AUX_MU_IIR_REG;
    *ptr = 0xC6;

    //Mini UART Baud rate   ((250,000,000/115200)/8)-1 = 270
    // mmio_write(AUX_MU_BAUD_REG,RPI_BAUD_57600);
    ptr = (uint32_t *)AUX_MU_BAUD_REG;
    *ptr = RPI_BAUD_57600;

    // GPIO Function Select 1
    // Each of the GPFSELn registers controls the functions of ten GPIO pins. 
    // GPFSEL0 controls GPIO00 through GPIO09. 
    // GPFSEL1 controls GPIO10 through GPIO19
    // The GPIO has 6 modes per pin, hence 3 bits are needed to control
    //                  each pin
    // Serial transmit:receive (TXD0:RXD0) are on GPIO14 and GPIO15
    // We only want to set our pin mode not affect other, so use and/or
    // to mask out the bits we want to change 
    // temp=mmio_read(GPFSEL1);
    ptr = (uint32_t *)GPFSEL1;
    temp = *ptr;

    temp &= ~(7<<12);    //gpio14
    temp |= 2<<12;    //alt5
    temp &= ~(7<<15);    //gpio15
    temp |= 2<<15;    //alt5
    //mmio_write(GPFSEL1,temp);
    ptr = (uint32_t *)GPFSEL1;
    *ptr = temp;


    /*-----------------------------------------------------------------------
      The GPIO Pull-up/down Clock Registers control the actuation of internal 
      pull-downs on the respective GPIO pins. These registers must be used in 
      conjunction with the GPPUD register to effect GPIO Pull-up/down changes. 
      The following sequence of events is required:
        1. Write to GPPUD to set the required control signal 
        2. Wait 150 cycles 
        3. Write to GPPUDCLK0/1 to clock the control signal into the GPIO pads 
      you wish to modify 
        4. Wait 150 cycles
        5. Write to GPPUD to remove the control signal
        6. Write to GPPUDCLK0/1 to remove the clock
   -----------------------------------------------------------------------*/
    // GPIO Pin Pull-up/down Enable
    // 1 .Disable pull up/down for all GPIO pins & delay for 150 cycles.

    // mmio_write(GPPUD,0);
    ptr = (uint32_t *)GPPUD;
    *ptr = 0;
        for(temp = 0; temp < DELAY_CYCLES; temp++) dummy();

    // GPIO Pin Pull-up/down Enable Clock 0
    // 3. Disable pull up/down for pin 14,15 & delay for 150 cycles.
    // mmio_write(GPPUDCLK0,(1<<14)|(1<<15));
    ptr = (uint32_t *)GPPUDCLK0;
    *ptr = (1<<14)|(1<<15);
        for(temp = 0; temp < DELAY_CYCLES; temp++) dummy();

    //  5. Write to GPPUD to remove the control signal
    // mmio_write(GPPUD,0);
    ptr = (uint32_t *)GPPUD;
    *ptr = 0;

    // GPIO Pin Pull-up/down Enable Clock 0
    // 6. Write 0 to GPPUDCLK0 to make it take effect.
    // mmio_write(GPPUDCLK0,0);
    ptr = (uint32_t *)GPPUDCLK0;
    *ptr = 0;

    // Mini UART Extra Control
    // 31:8 Reserved, write zero
    // 7    CTS assert,   Allows one to invert the CTS auto flow polarity.
    // 6    RTS assert,   Allows one to invert the RTS auto flow polarity.
    // 5:4  RTS AUTO flow, 00 : De-assert RTS when the receive FIFO has 3 spaces left.
    //                     01 : De-assert RTS when the receive FIFO has 2 spaces left.
    //                     10 : De-assert RTS when the receive FIFO has 1 spaces left.
    //                     11 : De-assert RTS when the receive FIFO has 4 spaces left.
    // 3    Enable transmit Auto flow-control using CTS
    // 2    Enable receive Auto flow-control using RTS
    // 1    Transmitter enable
    // 0    Receiver enable
    // 3 = 0000 0000 0000 0000 0000 0000 0000 0011
    // mmio_write(AUX_MU_CNTL_REG,3);
    ptr = (uint32_t *)AUX_MU_CNTL_REG;
    *ptr = 3;

    // Enable interrupts
    // mmio_write(IRQ_ENABLE1,1<<29);
    ptr = (uint32_t *)IRQ_ENABLE1;
    *ptr = 1<<29;

    return;
   }  // End uart_init()


/*---------------------------------------------------------------------------
  Writes a byte to the serial port by waiting 
---------------------------------------------------------------------------*/
void uartPutC(const char character)
   {
   uint32_t *ptr;
   // Wait for the port to be ready
   while(1)
      {
      // Mini UART Line Status
      // 31:7 Reserved, write zero
      // 6    Transmitter idle  
      // 5    Transmitter empty
      // 4:2  Reserved, write zero
      // 1    Receiver Overrun
      // 0    Data ready 
      // 0x60 = 0000 0000 0000 0000 0000 0000 0110 0000
      ptr = (uint32_t *)AUX_MU_LSR_REG;
      if ((*ptr & 0x60) == 0x60) break;   // xmiterr idle & empty
      }  // End while


   // Some RPI's report idle AND empty when they are not 100% idle and empty
   // Delay 150 cycles 
   for(int temp = 0; temp < DELAY_CYCLES; temp++) dummy();

 
   //Mini UART I/O Data
   // 31:8 Reserved, write zero
   // 7:0 LS 8 bits Baud rate read/write, DLAB=1
   //        Access to the LS 8 bits of the 16-bit baud rate register.
   // 7:0 Transmit data, DLAB=0
   //        Data written is put in the transmit FIFO 
   // 7:0 Receive data read, DLAB=0
   //        Data read is taken from the receive FIFO 
   // mmio_write(AUX_MU_IO_REG, character);
    ptr = (uint32_t *)AUX_MU_IO_REG;
    *ptr = character;
   return;
   }  // End uartPutC()


/*---------------------------------------------------------------------------
  This routine writes the string to the serial port, no CR or LF added.
---------------------------------------------------------------------------*/
void uartPutStr(const char *str)
   {
   while (*str != 0x00)
      {
      uartPutC(*str);
      str++;
      
      // Slow uartPutStr() to prevent PuTTy crashes on the PC
      for (int temp = 0; temp < XMIT_SLOWDOWN; temp++) dummy();

      }
   } // end uartPutStr()


/*---------------------------------------------------------------------------
  The routine converts an unsigned number to character decimal.  The buffer
  must be a minimum of 11 bytes long.
   void decStr(uint32_t num, uint8_t *buff)
  Where: 
   uint32_t num   - number to be converted to character
   uint8_t  buff   - pointer to a buffer to hold the answer, must be 11 bytes
                 leading zeros will be stripped.
---------------------------------------------------------------------------*/
void decStr(uint32_t num, uint8_t *buff)
{
   // An 32 bit in can only be 2**32-1 or 4,294,967,295
   uint32_t nums [9] = {100000000, 10000000, 1000000, 100000, 10000, 1000, 100, 10, 1};
   uint32_t i, digit;
   uint32_t first = 0;                  // The first non-zero value
   uint32_t indx = 0;                  // Output buffer index

   for(i =0; i < 9; i++)
   {
      digit = 0;
      // Needs conversion
      while (num >= nums [i])
      {
         digit++;
         num -= nums[i];
      }
      if (digit) { first = 1;}         // skip all leading zeroes
      if (first) 
      {
         buff [indx] = digit + 0x30;      // Convert to character
         indx++;
      }
   }
   // always null terminate
   buff [indx] = 0x00;
}



/*---------------------------------------------------------------------------
  This converts an integer binary string to printable hex using a masking
  technique and then write a space character
---------------------------------------------------------------------------*/
void uartHexStrings(uint32_t data)
   {
    //uint32_t ra;
    uint32_t readBits;
    uint32_t rdChar;

    readBits=32;
    while(1)
       {
       // Mask out a nibble
       readBits -= 4;
       rdChar = (data >> readBits) &0xF;

       //Convert the nibble to character hex
       if (rdChar > 9) rdChar += 0x37; else rdChar += 0x30;
 
       // Send the byte out
       uartPutC(rdChar);
       if (readBits ==0) break;
       }

   // display a space character
   uartPutC(0x20);
   } // end uartHexStrings()



/*---------------------------------------------------------------------------
  This prints an integer binary string to printable hex with a CR/LF at the
  end 
---------------------------------------------------------------------------*/
void uartHexString ( uint32_t d )
   {
    uartHexStrings(d);
    uartPutC(0x0D);
    uartPutC(0x0A);
   } // end uartHexString()


/*---------------------------------------------------------------------------
  This subroutine loops forever, login for changes in the circular buffer
---------------------------------------------------------------------------*/
void echoBuffer(void)
   {
	uint32_t cmdIndex = 0;
   // Process forever, looking for data to change due to interrupt activity
    while(1)
       {
       // The only way for data to get into here is via the interrupt handler
        while(rxtail!=rxhead)
           {
           // Echo the character back  
           uartPutC(rxbuffer[rxtail]);
             
           // Add a LF if necessary and execut the command
           if (rxbuffer[rxtail] == '\r') 
           { 
              uartPutC('\n');
              cmdLine [cmdIndex] = '\0';
 //             uartPutStr("command line ");
             // uartPutStr(cmdLine);
 //             uartPutStr("\r\n");
			  int resp;
              resp = parseCmdLine(cmdLine);
			  switch(resp) { 
				default: uartPutStr("Unexpected behavior!\n\r\0");
				case 0: break;
				case 1: uartPutStr("Too Few Arguments.\n\r\0"); break;
				case 2: uartPutStr("Invalid Argument Size.\n\r\0"); break;
				case 3: uartPutStr("Invalid Hex Argument\n\r\0"); break;
				case 5: uartPutStr("File not found\n\r\0"); break;
				case 10: uartPutStr("Syntax Error\n\r\0"); break;
			  }
              cmdIndex = 0;
			  
           } // End if \r
           else
           {           
              // Copy the character 
              cmdLine [cmdIndex] = rxbuffer [rxtail];
 
              // Handle the back spaces
              if ((rxbuffer [rxtail] == ASCII_DELETE) || (rxbuffer [rxtail] == ASCII_BSPACE))
                {
                    // Don't back up too far
                    if (cmdIndex == 1)  { cmdIndex = 0; }
                    else {cmdIndex--;}
                }
                else
                {
                    cmdIndex++;
                    //  Don't over index
                    if (cmdIndex > CMD_LINE_LEN) { cmdIndex = CMD_LINE_LEN; }
                } // End if
                    
           } // End else \r
           
           // Increment our circular pointer
           rxtail = (rxtail+1) & RXBUFMASK;
         
           }  // End if data in the circular buffer
       }  // End while
   } // End echoBuffer()



/*---------------------------------------------------------------------------
  Called from an assembly language routine that saves the stack
---------------------------------------------------------------------------*/
void __attribute__((interrupt)) c_irq_handler( void )
   {
   uint32_t status, readChar;
   uint32_t *ptr;

   //an interrupt has occurred, find out why
   while(1) //resolve all interrupts to uart
        {
        // Mini UART Interrupt Identify
        // 31:8 Reserved, write zero
        // 7:6 FIFO enables, always read as 1 as the FIFOs are always enabled
        // 5:4 - Always read as zero
        //   3 - Always read as zero
        // 2:1 READ: Interrupt ID: 00 : No interrupts
        //            01 : Transmit holding register empty
        //            10 : Receiver holds valid byte
        //             11 : <Not possible>
        // 2:1 WRITE: FIFO clear: bit 1 set will clear the receive FIFO
        //                bit 2 set will clear the transmit FIFO
        //   0 - Interrupt pending - This bit is 0 whenever an interrupt is pending
        // eg:
        ptr = (uint32_t *)AUX_MU_IIR_REG;
        status = *ptr;

         // If bit 0 is 1, then no more interrupts pending
        if ((status & 0x01) == 1) break; //no more interrupts

         // If bit 2:1 is 10 then the receiver holds a byte
        if ((status & 0x06) == 4)
           {
           //receiver holds a valid byte
           // Mini UART I/O Data
           // 31:8 Reserved, write zero
           // 7:0 LS 8 bits Baud rate read/write, DLAB=1
           //        Access to the LS 8 bits of the 16-bit baud rate register.
           // 7:0 Transmit data, DLAB=0
           //        Data written is put in the transmit FIFO 
           // 7:0 Receive data read, DLAB=0
           //        Data read is taken from the receive FIFO 
           ptr = (uint32_t *)AUX_MU_IO_REG; //read byte from rx FIFO
           readChar = *ptr;

           // Put the data int our circular buffer
           rxbuffer[rxhead] = readChar & 0xFF;
           rxhead           = (rxhead+1) & RXBUFMASK;
           } // End if
         } // End while 
} // End c_irq_handler()


//-------------------------------------------------------------------------
//-------------------------------------------------------------------------


//-------------------------------------------------------------------------
//
// Copyright (c) 2012 David Welch dwelch@dwelch.com
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
//------------------------------------------------------------------------- 
