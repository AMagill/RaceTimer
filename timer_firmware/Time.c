#include "main.h"
#include "inc/hw_memmap.h"
#include "driverlib/rom.h"
#include "driverlib/timer.h"
#include <math.h>
#include "Time.h"

uint32_t seconds = 0;
float fracLen;
uint32_t clockSpeed;

void timeInit()
{
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    ROM_TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC_UP);
    ROM_TimerLoadSet(TIMER0_BASE, TIMER_A, ROM_SysCtlClockGet());
    ROM_IntEnable(INT_TIMER0A);
    ROM_TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    ROM_TimerEnable(TIMER0_BASE, TIMER_A);

    clockSpeed = ROM_SysCtlClockGet();
    fracLen = (float)clockSpeed / (1<<16);
}

uint32_t timeNow()
{
	uint32_t frac = ROM_TimerValueGet(TIMER0_BASE, TIMER_A);
	return (seconds << 16) + (frac / fracLen);	// 16.16
}

void timeSync(double offset)
{
	double offset_i, offset_f;
	offset_f = modf(offset, &offset_i);	// Split into integer and fractional parts
	seconds += offset_i;
	uint32_t tav = HWREG(TIMER0_BASE + TIMER_O_TAV) + (offset_f * clockSpeed);
	if (tav > clockSpeed) tav -= clockSpeed;
	HWREG(TIMER0_BASE + TIMER_O_TAV) = tav;
}

void timeIntHandler()
{
	ROM_TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

	seconds++;
	gpioBlinkLEDs((seconds&0x07)<<1);
}
