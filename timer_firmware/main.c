#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "inc/hw_nvic.h"
#include "main.h"
#include "Timer.h"
#include "XBee.h"
#include "Protocol.h"
#include "BufferedUART.h"
#include "Battery.h"
#include "BigButton.h"

Timer *periodic;
uint32_t timeDown, timeUp;

void buttonIntHandler();
void gpioInit()
{
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);

	// Configure LEDs
	// On-board RGB LED
	ROM_GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);
	GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3, 0);

	// Configure buttons
	// On board buttons
	ROM_GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_4);
	ROM_GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_4, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);

}


void tick()
{
	ROM_TimerIntClear(TIMER2_BASE, TIMER_A);
	pcSendHeartbeat();
	batterySampleTrigger();

	if (pcLastHeard() > 1000)	// Haven't heard an ACK for over a second
		buttonSetBlink(BLINK_SOS);
	else
		buttonSetBlink(BLINK_STROBE);
}


void buttonCB(bool pressed, uint32_t time)
{
	pcSendEvent(time, pressed ? EVT_BTN_DOWN : EVT_BTN_UP);
}



void moveVtableToRAM()
{
	uint32_t ui32Idx, ui32Value;
	extern void (*g_pfnRAMVectors[NUM_INTERRUPTS])(void);

	ui32Value = HWREG(NVIC_VTABLE);
	for(ui32Idx = 0; ui32Idx < NUM_INTERRUPTS; ui32Idx++)
		g_pfnRAMVectors[ui32Idx] = (void (*)(void))HWREG((ui32Idx * 4) + ui32Value);
	HWREG(NVIC_VTABLE) = (uint32_t)g_pfnRAMVectors;
}


int main(void)
{
    // Set the system clock to run at 50MHz from the PLL.
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);
    ROM_FPULazyStackingEnable();

    ROM_IntMasterDisable();
    moveVtableToRAM();

    // Configure devices
    gpioInit();							// GPIO (buttons & LEDs)
    rtcInit();
    buttonInit(buttonCB);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER2);
	periodic = timerInit(TIMER2_BASE, 0.5, tick);	// Periodic timer
    xbInit(pcFrameReceived);			// XBee connection (and UART)
    uartInit();							// General purpose UART
    batteryInit();

    // Enable processor interrupts.
    ROM_IntMasterEnable();


    while (1)
    {
    	ROM_SysCtlSleep();
    }
}

