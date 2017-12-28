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

static void debounceCB();
static void blinkCB();
static void buttonIntHandler();

void buttonInit(ButtonCallback btnCB)
{
	buttonCB = btnCB;

	// Enable peripherals
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER1);

	// LED output
	GPIOPinTypeGPIOOutput(GPIO_PORTA_BASE, GPIO_PIN_2);
	GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_2, 0);

	// Button input
	GPIOPinTypeGPIOInput(GPIO_PORTA_BASE, GPIO_PIN_3);
	GPIOPadConfigSet(GPIO_PORTA_BASE, GPIO_PIN_3, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPD);
	GPIOIntTypeSet(GPIO_PORTA_BASE, GPIO_PIN_3, GPIO_BOTH_EDGES);
	GPIOIntRegister(GPIO_PORTA_BASE, buttonIntHandler);
	GPIOIntClear(GPIO_PORTA_BASE, GPIO_PIN_3);
	GPIOIntEnable(GPIO_PORTA_BASE, GPIO_PIN_3);

	// Debounce timer
	dbTimerTicks = SysCtlClockGet() / 256;
    TimerConfigure(dbTimerBase, TIMER_CFG_ONE_SHOT);
    TimerLoadSet(dbTimerBase, TIMER_A, dbTimerTicks);
    TimerIntRegister(dbTimerBase, TIMER_A, debounceCB);
    TimerIntEnable(dbTimerBase, TIMER_TIMA_TIMEOUT);

    // Blink timer
    buttonSetBlink(BLINK_SOS);
    blTimerTicks = SysCtlClockGet() / 1000;
    TimerConfigure(blTimerBase, TIMER_CFG_PERIODIC);
    TimerLoadSet(blTimerBase, TIMER_A, blTimerTicks*1000);
    TimerIntRegister(blTimerBase, TIMER_A, blinkCB);
    TimerIntEnable(blTimerBase, TIMER_TIMA_TIMEOUT);
    TimerEnable(blTimerBase, TIMER_A);
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
		TimerDisable(blTimerBase, TIMER_A);
		GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_2, 0);
		break;
	case BLINK_ON:
		TimerDisable(blTimerBase, TIMER_A);
		GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_2, GPIO_PIN_2);
		break;
	case BLINK_SOS:
		TimerEnable(blTimerBase, TIMER_A);
		GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_2, 0);
		blinkData = blinkSOS;
		blinkStep = 0;
		break;
	case BLINK_STROBE:
		TimerEnable(blTimerBase, TIMER_A);
		GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_2, 0);
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
	TimerEnable(dbTimerBase, TIMER_A);
	TimerLoadSet(dbTimerBase, TIMER_A, dbTimerTicks);
}

static void debounceCB()
{
	TimerIntClear(dbTimerBase, TIMER_TIMA_TIMEOUT);

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
	TimerIntClear(blTimerBase, TIMER_TIMA_TIMEOUT);

	uint32_t state = GPIOPinRead(GPIO_PORTA_BASE, GPIO_PIN_2);
	GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_2, state ^ GPIO_PIN_2);

	blinkStep++;
	if (blinkStep > blinkData[0]) blinkStep = 1;
    TimerLoadSet(blTimerBase, TIMER_A, blTimerTicks * blinkData[blinkStep]);
}

