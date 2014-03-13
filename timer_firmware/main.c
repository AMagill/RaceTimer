#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "main.h"
#include "Time.h"
#include "XBee.h"
#include "Protocol.h"
#include "BufferedUART.h"

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

void tick(uint32_t seconds)
{
	gpioBlinkLEDs((seconds&0x07)<<1);
	if (!(seconds & 0x0F))
		timeUnsync();
	pcSendSyncRequest(0xFFFF);
}

int main(void)
{
    // Set the system clock to run at 50MHz from the PLL.
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);
    ROM_FPULazyStackingEnable();

    // Configure devices
    gpioInit();		// GPIO (buttons & LEDs)
	timeInit();		// Timer
	timeSetTickCB(tick);
    xbInit();		// XBee connection (and UART)
    xbSetFrameCB(pcFrameReceived);
    uartInit();

    // Enable processor interrupts.
    ROM_IntMasterEnable();

    while(1)
    {
    	uint8_t btns = gpioGetButtons();
    	if (!(btns & BTN_1))
    	{
    		ROM_TimerDisable(TIMER0_BASE, TIMER_A);
    		SysCtlDelay(SysCtlClockGet() / (10 * 3));
    		GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3, LED_R);
    		while (!(gpioGetButtons() & BTN_1));
    		ROM_TimerEnable(TIMER0_BASE, TIMER_A);
    		GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3, 0);
    		timeUnsync();
    		//gpioBlinkLEDs(LED_B);
    		//SysCtlDelay(SysCtlClockGet() / (10 * 3));
    	}
    }
}
