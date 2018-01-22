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
#include "Display.h"

static const bool     RED_BUTTON    = true;
static const uint16_t RED_ADDRESS   = 0x0101;
static const uint16_t GREEN_ADDRESS = 0x0100;

static bool     timerRunning   = false;
static bool     buttonDown     = false;
static uint32_t buttonTime     = 0;
static int32_t  timerValue     = 0;     // Start time if running, total ms if stopped
static uint32_t lastRx         = 0;
static uint32_t lastTx         = 0;


static void gpioInit()
{
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);

	// Configure LEDs
	// On-board RGB LED
	GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);
	GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3, 0);

	// Configure buttons
	// On board buttons
	GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_4);
	GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_4, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
}

static void gpioSetLed(bool red, bool green, bool blue)
{
	uint8_t val =
		(red   ? GPIO_PIN_1 : 0) |
		(green ? GPIO_PIN_3 : 0) |
		(blue  ? GPIO_PIN_2 : 0);
	GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3, val);
}


static void sendState()
{
	const uint32_t now = rtcMillis();
	lastTx = now;
	gpioSetLed(now-lastRx < 50, now-lastTx < 50, buttonDown);

	if (RED_BUTTON)
		pcSendState(GREEN_ADDRESS, timerRunning, buttonDown, now, timerValue);
	else
		pcSendState(RED_ADDRESS,   timerRunning, buttonDown, now, buttonTime);
}

static void tick()
{
	TimerIntClear(TIMER2_BASE, TIMER_A);

	uint32_t now = rtcMillis();

	if (RED_BUTTON)
	{
		// Make sure updates go out periodically
		if (now - lastTx > 1000)
			sendState();
	}

	if (now - lastRx > 2100)  // Haven't heard from the other box in a while
	{
		displaySetText("NOT CONN");
		displayUpdate();
		buttonSetLed(false);
	}
	else
	{
		if (timerRunning)
			displaySetTime(now - timerValue);
		else
			displaySetTime(timerValue);
		displayUpdate();

		if (RED_BUTTON)
		{
			buttonSetLed(timerRunning);
		}
		else
		{
			if (timerRunning)
				buttonSetLed(false);
			else if (buttonDown)
				buttonSetLed(now%100<50);
			else
				buttonSetLed(true);
		}
	}

	gpioSetLed(now-lastRx < 50, now-lastTx < 50, buttonDown);
}


static void buttonCB(bool pressed, uint32_t time)
{
	const uint32_t now = rtcMillis();
	buttonDown = pressed;
	buttonTime = time;  // Not 'now', because the debounce algorithm can send events from the past

	if (RED_BUTTON)
	{
		if (pressed && timerRunning)  // Press down stops
		{
			timerRunning = false;
			timerValue   = buttonTime - timerValue;
			sendState();
		}
	}
	else // green button
	{
		// Just let red know about the button.
		sendState();
	}
}

static void rxStateCB(const stateMsg* msg)
{
	static uint32_t lastButtonTime = 0;
	const uint32_t now = rtcMillis();
	lastRx = now;
	gpioSetLed(now-lastRx < 50, now-lastTx < 50, buttonDown);

	if (RED_BUTTON)
	{
		if (msg->timerValue > lastButtonTime)
		{ // The green button state changed
			if (lastButtonTime != 0)
			{
				if (msg->btnDown)
				{ // The green button just went down
					timerRunning = false;
					timerValue   = 0;
				}
				else
				{ // The green button just went up
					timerRunning = true;
					timerValue   = now;
				}
				sendState();
			}
			lastButtonTime = msg->timerValue;
		}
	}
	else  // green button
	{
		timerRunning = msg->running;
		timerValue   = msg->timerValue;
		if (timerRunning)
			timerValue += (now - msg->rtcTime);  // Compensate for clock skew
		sendState();
	}
}




static void moveVtableToRAM()
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
    Timer *periodic;

    // Set the system clock to run at 50MHz from the PLL.
    SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);
    FPULazyStackingEnable();

    IntMasterDisable();
    moveVtableToRAM();

    // Configure devices
    gpioInit();							// GPIO (buttons & LEDs)
    rtcInit();
    buttonInit(buttonCB);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER2);
	periodic = timerInit(TIMER2_BASE, 0.01, tick);	// Periodic timer
    xbInit(pcFrameReceived);			// XBee connection (and UART)
    uartInit();							// General purpose UART
    batteryInit();
    displayInit();
    pcInit(rxStateCB);

    // Enable processor interrupts.
    IntMasterEnable();

    delayMs(100);  // Let battery voltage settle
    batterySampleTrigger();
    delayMs(10);

    displaySetText("  HELLO");
    displayUpdate();
    delayMs(500);

    displaySetText("BATT");
    displaySetNumber(5, batteryGetPercent());
    displayUpdate();
    delayMs(1000);

    timerStart(periodic);

    while (1)
    {
    	SysCtlSleep();
    }
}


void delayMs(uint32_t ms)
{
    SysCtlDelay(SysCtlClockGet() / 3000 * ms);
}
