#include "main.h"
#include "Time.h"
#include "BufferedUART.h"
#include <math.h>

static uint32_t seconds = 0;
static float fracLen;
static uint32_t clockSpeed;
static bool synced = false;
static TickCallback tickCB = NULL;

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
	return (seconds << 16) + (uint32_t)(frac / fracLen);	// 16.16
}

void timeSync(double offset)
{
	double offset_i, offset_f;

	if (synced)
	{
		uartPrint("E %Lf\r\n", offset);
	}
	else
	{
		offset_f = modf(offset, &offset_i);	// Split into integer and fractional parts
		int32_t tav = HWREG(TIMER0_BASE + TIMER_O_TAV) + (offset_f * clockSpeed);
		if (tav > clockSpeed)
		{
			offset_i++;
			tav -= clockSpeed;
		} else if (tav < 0)
		{
			offset_i--;
			tav += clockSpeed;
		}
		seconds += offset_i;
		HWREG(TIMER0_BASE + TIMER_O_TAV) = tav;
		uartPrint("S %Lf\r\n", offset);
		synced = true;
	}
}

void timeUnsync()
{
	synced = false;
}

void timeIntHandler()
{
	ROM_TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
	seconds++;
	if (tickCB) tickCB(seconds);
}

void timeSetTickCB(TickCallback cb)
{
	tickCB = cb;
}
