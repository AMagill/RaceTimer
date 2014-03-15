#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "main.h"
#include "Timer.h"
#include "XBee.h"
#include "Protocol.h"
#include "BufferedUART.h"

Timer *rtc, *periodic;
uint32_t timeDown, timeUp;

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

void tick(Timer *timer)
{
	//gpioBlinkLEDs((timer->count&0x07)<<1);
	pcSendUpdate(0xFFFF, timeDown, timeUp);
}

int main(void)
{
    // Set the system clock to run at 50MHz from the PLL.
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);
    ROM_FPULazyStackingEnable();

    // Configure devices
    gpioInit();							// GPIO (buttons & LEDs)
	rtc = timerInit(0, 1.0, NULL);		// RTC timer
	periodic = timerInit(1, 0.5, tick);	// Periodic timer
    xbInit(pcFrameReceived);			// XBee connection (and UART)
    uartInit();							// General purpose UART

    // Enable processor interrupts.
    ROM_IntMasterEnable();

    while(1)
    {
    	uint8_t btns = gpioGetButtons();
    	if (!(btns & BTN_1))
    	{
    		timeDown = timerNow(rtc);
    		pcSendUpdate(0xFFFF, timeDown, timeUp);
    		while (!(gpioGetButtons() & BTN_1));
    		timeUp = timerNow(rtc);
    		pcSendUpdate(0xFFFF, timeDown, timeUp);
    	}
    }
}
