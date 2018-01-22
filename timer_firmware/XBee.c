#include "main.h"
#include "XBee.h"
#include "RingBuffer.h"

#define BUF_SIZE 256
static uint8_t rxBuf[BUF_SIZE];
static RingBuffer *txBuf;
static FrameCallback frameCB = NULL;

// Uses UART1: Rx PB0 + Tx PB1
void xbInit(FrameCallback callback)
{
	// Init variables
	txBuf = rbInit(BUF_SIZE);
	frameCB = callback;

	// Enable hardware
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART1);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);

	// Configure UART
    GPIOPinConfigure(GPIO_PB0_U1RX);
    GPIOPinConfigure(GPIO_PB1_U1TX);
    GPIOPinTypeUART(GPIO_PORTB_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    UARTConfigSetExpClk(UART1_BASE, SysCtlClockGet(), 57600,
                            UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE);
    UARTFIFOEnable(UART1_BASE);
    IntRegister(INT_UART1, xbUARTIntHandler);
    IntEnable(INT_UART1);
    UARTIntEnable(UART1_BASE, UART_INT_RX | UART_INT_RT | UART_INT_TX);
}

void xbUARTIntHandler()
{
    static uint16_t rxBytes = 0, rxLength = 0;
    static uint8_t  rxChecksum = 0;

    // Get and clear the interrupt status.
	uint32_t status = UARTIntStatus(UART1_BASE, true);
    UARTIntClear(UART1_BASE, status);

    // Loop while there are characters in the receive FIFO.
    if (status & (UART_INT_RX | UART_INT_RT))
    {
		while(UARTCharsAvail(UART1_BASE))
		{
			uint8_t inCh = UARTCharGetNonBlocking(UART1_BASE);
			if (rxBytes == 0)					// Start delimiter
			{
				if (inCh == 0x7E)
					rxBuf[rxBytes++] = inCh;
			}
			else if (rxBytes == 1)				// Length MSB
			{
				rxLength = inCh << 8;
				rxBuf[rxBytes++] = inCh;
			}
			else if (rxBytes == 2)				// Length LSB
			{
				rxLength = (rxLength | inCh) + 3;
				if (rxLength < 256)	// Prevent unreasonably long reads
				{
					rxBuf[rxBytes++] = inCh;
					rxChecksum = 0;
				}
				else
					rxBytes = 0;
			}
			else if (rxBytes < rxLength)		// Frame body
			{
				rxBuf[rxBytes++] = inCh;
				rxChecksum += inCh;
			}
			else								// Checksum
			{
				rxBuf[rxBytes++] = inCh;
				rxChecksum += inCh;

				if (rxChecksum == 0xFF)			// Accepted frame!
					if (frameCB) frameCB(&rxBuf[3], rxLength-3);

				rxBytes = 0;
			}
		}
    }

    if (status & UART_INT_TX)
    {
    	while (!rbIsEmpty(txBuf) && UARTSpaceAvail(UART1_BASE))
    		UARTCharPut(UART1_BASE, rbRead(txBuf));
    }
}

void xbUARTSend(const uint8_t *buffer, uint32_t count)
{
    // Loop while there are more characters to send.
    while(count--)
    {
    	// Send now, if nothing else is waiting.  Otherwise buffer it.
    	if (rbIsEmpty(txBuf) && UARTSpaceAvail(UART1_BASE))
    		UARTCharPut(UART1_BASE, *buffer++);
    	else
    		rbWrite(txBuf, *buffer++);
    }
}

void xbSendFrameTx16(uint16_t address, uint8_t opts, const uint8_t *msg, uint16_t length)
{
	uint8_t frame[8];
	frame[0] = 0x7E;				// Start delimiter
	frame[1] = (length + 5) >> 8;	// Length MSB
	frame[2] = (length + 5) & 0xFF;	// Length LSB
	frame[3] = 0x01;				// Frame type: TX w/ 16-bit address
	frame[4] = !opts;				// Frame ID	- don't request response if NO_ACK
	frame[5] = address >> 8;		// Address MSB
	frame[6] = address & 0xFF;		// Address LSB
	frame[7] = opts;				// Options

	// Checksum
	int i;
	uint8_t checksum = 0;
	for (i = 3; i < sizeof(frame); i++)
		checksum += frame[i];
	for (i = 0; i < length; i++)
		checksum += msg[i];
	checksum = 0xFF - checksum;

	xbUARTSend(frame, sizeof(frame));
	xbUARTSend(msg, length);
	xbUARTSend(&checksum, 1);
}

void xbSendFrameTx64(uint64_t address, uint8_t opts, const uint8_t *msg, uint16_t length)
{
    uint8_t frame[14];
    frame[ 0] = 0x7E;                    // Start delimiter
    frame[ 1] = (length + 11) >> 8;      // Length MSB
    frame[ 2] = (length + 11) & 0xFF;    // Length LSB
    frame[ 3] = 0x00;                    // Frame type: TX w/ 64-bit address
    frame[ 4] = !opts;                   // Frame ID - don't request response if NO_ACK
    frame[ 5] = (address >> 56);         // Address MSB
    frame[ 6] = (address >> 48) & 0xFF;  // Address
    frame[ 7] = (address >> 40) & 0xFF;  // Address
    frame[ 8] = (address >> 32) & 0xFF;  // Address
    frame[ 9] = (address >> 24) & 0xFF;  // Address
    frame[10] = (address >> 16) & 0xFF;  // Address
    frame[11] = (address >> 8 ) & 0xFF;  // Address
    frame[12] = (address      ) & 0xFF;  // Address LSB
    frame[13] = opts;                    // Options

    // Checksum
    int i;
    uint8_t checksum = 0;
    for (i = 3; i < sizeof(frame); i++)
        checksum += frame[i];
    for (i = 0; i < length; i++)
        checksum += msg[i];
    checksum = 0xFF - checksum;

    xbUARTSend(frame, sizeof(frame));
    xbUARTSend(msg, length);
    xbUARTSend(&checksum, 1);
}
