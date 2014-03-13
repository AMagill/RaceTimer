#include "main.h"
#include "BufferedUART.h"
#include "RingBuffer.h"
#include <stdio.h>

#define BUF_SIZE 64
static RingBuffer *rxBuf, *txBuf;

// Uses UART0: Rx PA0 + Tx PA1
void uartInit()
{
	// Init variables
	rxBuf = rbInit(BUF_SIZE);
	txBuf = rbInit(BUF_SIZE);

	// Enable hardware
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

	// Configure UART
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    ROM_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    ROM_UARTConfigSetExpClk(UART0_BASE, ROM_SysCtlClockGet(), 115200,
                            UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE);
    ROM_UARTFIFOEnable(UART0_BASE);
    ROM_IntEnable(INT_UART0);
    ROM_UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT | UART_INT_TX);
}

void uartIntHandler()
{
    // Get and clear the interrupt status.
	uint32_t status = ROM_UARTIntStatus(UART0_BASE, true);
    ROM_UARTIntClear(UART0_BASE, status);

    // Loop while there are characters in the receive FIFO.
    if (status & (UART_INT_RX | UART_INT_RT))
    {
		while(!rbIsFull(rxBuf) && ROM_UARTCharsAvail(UART0_BASE))
			rbWrite(rxBuf, ROM_UARTCharGetNonBlocking(UART0_BASE));
    }

    if (status & UART_INT_TX)
    {
    	while (!rbIsEmpty(txBuf) && ROM_UARTSpaceAvail(UART0_BASE))
    		ROM_UARTCharPut(UART0_BASE, rbRead(txBuf));
    }
}

void uartSend(const char *buffer, uint32_t count)
{
    // Loop while there are more characters to send.
    while(count--)
    {
    	// Send now, if nothing else is waiting.  Otherwise buffer it.
    	if (rbIsEmpty(txBuf) && ROM_UARTSpaceAvail(UART0_BASE))
    		ROM_UARTCharPut(UART0_BASE, *buffer++);
    	else
    		rbWrite(txBuf, *buffer++);
    }
}

void uartPrint(const char *format, ...)
{
	int len;
	char buffer[128];
	va_list args;
	va_start(args, format);
	len = vsnprintf(buffer, sizeof(buffer), format, args);
	uartSend(buffer, len);
	va_end(args);
}
