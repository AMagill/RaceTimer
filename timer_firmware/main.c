#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "main.h"
#include "Time.h"
#include "XBee.h"
#include "Protocol.h"

void gpioInit()
{
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);

	// Configure LEDs
	ROM_GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);
	GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3, 0);

	// Configure buttons
	ROM_GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_4);
	ROM_GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_4, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
}

void gpioBlinkLEDs(uint8_t which)
{
	GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3, which & (LED_R|LED_G|LED_B));
	SysCtlDelay(SysCtlClockGet() / (1000 * 3));
	GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3, 0);
}

uint8_t gpioGetButtons()
{
	return ROM_GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_4);
}

int main(void)
{
    // Set the clocking to run directly from the crystal.
    //ROM_SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);

    // Set the system clock to run at 50MHz from the PLL.
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);
    ROM_FPULazyStackingEnable();

    // Configure devices
    gpioInit();		// GPIO (buttons & LEDs)
	timeInit();		// Timer
    xbInit();		// XBee connection (and UART)
    xbSetFrameCB(pcFrameReceived);

    // Enable processor interrupts.
    ROM_IntMasterEnable();

    while(1)
    {
    	uint8_t btns = gpioGetButtons();
    	if (!(btns & BTN_1))
    	{
    		syncRequestMsg msg;
    		msg.type = 's';
    		msg.time0 = timeNow();
    		xbSendFrameTx16(0xFFFF, false, (uint8_t*)&msg, sizeof(msg));
    		SysCtlDelay(SysCtlClockGet() / (10 * 3));
    	}
    }
}
