#include "main.h"
#include "BigButton.h"

static ButtonCallback buttonCB  = NULL;
static uint32_t btnLastInt      = 0;
static bool     btnLastState 	= false;
static bool     btnDebouncing 	= false;
static const uint32_t dbTimerBase = TIMER0_BASE;
static uint32_t dbTimerTicks;

static const uint32_t blTimerBase = TIMER1_BASE;
static uint32_t blTimerTicks;
static uint32_t blinkPattern;
static uint32_t blinkStep;
static uint32_t const *blinkData;

void debounceCB();
void blinkCB();
void buttonIntHandler();

void buttonInit(ButtonCallback btnCB)
{
	buttonCB = btnCB;

	// Enable peripherals
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER1);

	// LED output
	ROM_GPIOPinTypeGPIOOutput(GPIO_PORTA_BASE, GPIO_PIN_2);
	GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_2, 0);

	// Button input
	ROM_GPIOPinTypeGPIOInput(GPIO_PORTA_BASE, GPIO_PIN_3);
	ROM_GPIOPadConfigSet(GPIO_PORTA_BASE, GPIO_PIN_3, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPD);
	ROM_GPIOIntTypeSet(GPIO_PORTA_BASE, GPIO_PIN_3, GPIO_BOTH_EDGES);
	GPIOIntRegister(GPIO_PORTA_BASE, buttonIntHandler);
	GPIOIntClear(GPIO_PORTA_BASE, GPIO_PIN_3);
	GPIOIntEnable(GPIO_PORTA_BASE, GPIO_PIN_3);

	// Debounce timer
	dbTimerTicks = ROM_SysCtlClockGet() / 256;
    ROM_TimerConfigure(dbTimerBase, TIMER_CFG_ONE_SHOT);
    ROM_TimerLoadSet(dbTimerBase, TIMER_A, dbTimerTicks);
    TimerIntRegister(dbTimerBase, TIMER_A, debounceCB);
    ROM_TimerIntEnable(dbTimerBase, TIMER_TIMA_TIMEOUT);

    // Blink timer
    buttonSetBlink(BLINK_SOS);
    blTimerTicks = ROM_SysCtlClockGet() / 1000;
    ROM_TimerConfigure(blTimerBase, TIMER_CFG_PERIODIC);
    ROM_TimerLoadSet(blTimerBase, TIMER_A, blTimerTicks*1000);
    TimerIntRegister(blTimerBase, TIMER_A, blinkCB);
    ROM_TimerIntEnable(blTimerBase, TIMER_TIMA_TIMEOUT);
    ROM_TimerEnable(blTimerBase, TIMER_A);
}

void buttonSetBlink(uint32_t pattern)
{
	//static const uint32_t blinkFast[]   = {2, 10, 10};
	//static const uint32_t blinkSlow[]   = {2, 1000, 1000};
	static const uint32_t blinkStrobe[] = {2, 10, 1000};
	//static const uint32_t blinkdDd[]    = {6, 10, 200, 100, 200, 10, 1000};
	static const uint32_t blinkSOS[]    = {18, 100, 200, 100, 200, 100, 200,
			                                   500, 200, 500, 200, 500, 200,
			                                   100, 200, 100, 200, 100, 1000};

	if (pattern == blinkPattern) return;
	blinkPattern = pattern;

	switch (pattern)
	{
	case BLINK_OFF:
		ROM_TimerDisable(blTimerBase, TIMER_A);
		ROM_GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_2, 0);
		break;
	case BLINK_ON:
		ROM_TimerDisable(blTimerBase, TIMER_A);
		ROM_GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_2, GPIO_PIN_2);
		break;
	case BLINK_SOS:
		ROM_TimerEnable(blTimerBase, TIMER_A);
		ROM_GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_2, 0);
		blinkData = blinkSOS;
		blinkStep = 0;
		break;
	case BLINK_STROBE:
		ROM_TimerEnable(blTimerBase, TIMER_A);
		ROM_GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_2, 0);
		blinkData = blinkStrobe;
		blinkStep = 0;
		break;
	}
}




static void buttonIntHandler()
{
	GPIOIntClear(GPIO_PORTA_BASE, GPIO_PIN_3);

	if (!btnDebouncing)
	{
		// Accept the transition immediately
		btnLastState  = !btnLastState;
		btnDebouncing = true;
		buttonCB(btnLastState, rtcMillis());
	}

	// Otherwise the transition will be accepted retroactively when
	// transitions stop long enough for the debounce timer to expire.
	btnLastInt = rtcMillis();
	ROM_TimerEnable(dbTimerBase, TIMER_A);
	ROM_TimerLoadSet(dbTimerBase, TIMER_A, dbTimerTicks);
}

static void debounceCB()
{
	ROM_TimerIntClear(dbTimerBase, TIMER_TIMA_TIMEOUT);

	bool btnState = !!GPIOPinRead(GPIO_PORTA_BASE, GPIO_PIN_3);
	if (btnState != btnLastState)
	{
		// Accept the transition
		btnLastState = btnState;
		buttonCB(btnLastState, btnLastInt);
	}
	btnDebouncing = false;
}

static void blinkCB()
{
	ROM_TimerIntClear(blTimerBase, TIMER_TIMA_TIMEOUT);

	uint32_t state = ROM_GPIOPinRead(GPIO_PORTA_BASE, GPIO_PIN_2);
	ROM_GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_2, state ^ GPIO_PIN_2);

	blinkStep++;
	if (blinkStep > blinkData[0]) blinkStep = 1;
    ROM_TimerLoadSet(blTimerBase, TIMER_A, blTimerTicks * blinkData[blinkStep]);
}

