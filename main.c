
//-------------------------------------------------------------------------
// main.c
// This program call the interrupt driven RPI serial receiver and
// normal transmitter.
// 07/07/2013 - Initial version
// 10/10/2013 - Fix head/tail
// 09/09/2014 - remove rxbuffer[]
//-------------------------------------------------------------------------

#include <stdint.h>
#include <cmpe240.h>
/*---------------------------------------------------------------------------
  Interrupt handler variables 
---------------------------------------------------------------------------*/
#define  RXBUFMASK 0xFFF
extern volatile uint32_t rxhead;
extern volatile uint32_t rxtail;





extern void enable_irq (void);
extern void uartPutStr(const char *str);
extern void uartHexStrings(uint32_t data);
extern void uartPutC(const char character);


void test_encode() { 
	INT_FRACT nums[] = { {0, 0}, {1, 0}, {0, 0x80000000}, {0, 0xC0000000}, {0xD, 0x20000000}, \
	{0, 0xD5400000}, {0xfffffff3, 0x20000000}, {0x00800000, 0} };
	IEEE_FLT flt = 0;
	for (int temp = 0; temp < 1000; temp++) dummy();

	uartPutStr("Encoding:\n\0");
	
	uartPutC('\r');
	uartPutStr("Input                Results\n\0");
	uartPutC('\r');
	
	for(int i = 0; i < 8; i++) { 
		flt = IeeeEncode(nums[i]);
		for (int temp = 0; temp < 1000; temp++) dummy();
		uartHexStrings(nums[i].real);
		uartHexStrings(nums[i].fraction);
		uartHexStrings(flt);
		uartPutC('\n');
		uartPutC('\r');

	}

}

void test_mul() { 
	INT_FRACT nums[] = { {0, 0}, {0x3f800000, 0x3f800000}, {0x3f000000, 0x3f000000}, {0x41520000, 0x41520000}, {0xC1520000, 0xC1520000}, \
	{0x4B000000, 0x3f000000} }; //using int-fract struct for holding a and b
	IEEE_FLT flt = 0;
	for (int temp = 0; temp < 1000; temp++) dummy();

	uartPutStr("Multiply:\n\0");
	uartPutC('\r');
	uartPutStr("Input                Results\n\0");
	uartPutC('\r');
	
	for(int i = 0; i < 6; i++) { 
		flt = IeeeMult(nums[i].real, nums[i].fraction);
		for (int temp = 0; temp < 1000; temp++) dummy();
		uartHexStrings(nums[i].real);
		uartHexStrings(nums[i].fraction);
		uartHexStrings(flt);
		uartPutC('\n');
		uartPutC('\r');

	}

}

void test_add() { 
	INT_FRACT nums[] = { {0, 0}, {0x3f800000, 0x3f800000}, {0x3f000000, 0x3f000000}, {0x41520000, 0x41520000}, {0xC1520000, 0xC1520000}, \
	{0x4B000000, 0x3f000000} }; //using int-fract struct for holding a and b
	IEEE_FLT flt = 0;
	for (int temp = 0; temp < 1000; temp++) dummy();

	uartPutStr("Add:\n\0");
	uartPutC('\r');
	uartPutStr("Input                Results\n\0");
	uartPutC('\r');
	
	for(int i = 0; i < 6; i++) { 
		flt = IeeeAdd(nums[i].real, nums[i].fraction);
		for (int temp = 0; temp < 1000; temp++) dummy();
		uartHexStrings(nums[i].real);
		uartHexStrings(nums[i].fraction);
		uartHexStrings(flt);
		uartPutC('\n');
		uartPutC('\r');
	}

}

/*---------------------------------------------------------------------------
  The main entry point
---------------------------------------------------------------------------*/
int main(void )
{
	
 
    // Initialize the UART with
    uart_init();
	while(1) {
    // Zero out the circular buffer
	rxtail = rxhead = 0;

    // Enable interrupts
	enable_irq();

    // echo forever 
	echoBuffer();
	}
	
	
	
		



    return(0);
}