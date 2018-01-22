#include "main.h"
#include "BigButton.h"

static const uint32_t DB_TIMER_BASE = TIMER0_BASE;

static ButtonCallback buttonCB  = NULL;
static uint32_t btnLastInt      = 0;
static bool     btnLastState 	= false;
static bool     btnDebouncing 	= false;
static uint32_t dbTimerTicks;

static void debounceCB();
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
    TimerConfigure(DB_TIMER_BASE, TIMER_CFG_ONE_SHOT);
    TimerLoadSet(DB_TIMER_BASE, TIMER_A, dbTimerTicks);
    TimerIntRegister(DB_TIMER_BASE, TIMER_A, debounceCB);
    TimerIntEnable(DB_TIMER_BASE, TIMER_TIMA_TIMEOUT);
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
	TimerEnable(DB_TIMER_BASE, TIMER_A);
	TimerLoadSet(DB_TIMER_BASE, TIMER_A, dbTimerTicks);
}

static void debounceCB()
{
	TimerIntClear(DB_TIMER_BASE, TIMER_TIMA_TIMEOUT);

	bool btnState = !!GPIOPinRead(GPIO_PORTA_BASE, GPIO_PIN_3);
	if (btnState != btnLastState)
	{
		// Accept the transition
		btnLastState = btnState;
		buttonCB(btnLastState, btnLastInt);
	}
	btnDebouncing = false;
}


void buttonSetLed(bool ledOn)
{
    GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_2, ledOn ? GPIO_PIN_2 : 0);
}



