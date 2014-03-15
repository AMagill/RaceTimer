#include "main.h"
#include "Timer.h"
#include "BufferedUART.h"
#include <math.h>

static Timer* these[6];

Timer* timerInit(uint8_t timer, float period, TimerCallback callback)
{
	const uint32_t allBases[]   = {TIMER0_BASE, TIMER1_BASE, TIMER2_BASE, TIMER3_BASE, TIMER4_BASE, TIMER5_BASE};
	const uint32_t allPeriphs[] = {SYSCTL_PERIPH_TIMER0, SYSCTL_PERIPH_TIMER1, SYSCTL_PERIPH_TIMER2,
								   SYSCTL_PERIPH_TIMER3, SYSCTL_PERIPH_TIMER4, SYSCTL_PERIPH_TIMER5};
	const uint32_t allInts[]    = {INT_TIMER0A, INT_TIMER1A, INT_TIMER2A, INT_TIMER3A, INT_TIMER4A, INT_TIMER5A};

	Timer *this 		= malloc(sizeof(Timer));
	this->timer 		= timer;
	this->base  		= allBases[timer];
	this->count 		= 0;
	this->period 		= period;
	this->periodTicks 	= period * ROM_SysCtlClockGet();
	this->fracLen       = (float)this->periodTicks / (1<<12);
	this->callback      = callback;
	this->adjusted		= 0;
	these[timer] 		= this;

    ROM_SysCtlPeripheralEnable(allPeriphs[timer]);
    ROM_TimerConfigure(this->base, TIMER_CFG_PERIODIC_UP);
    ROM_TimerLoadSet(this->base, TIMER_A, this->periodTicks);
    ROM_IntEnable(allInts[timer]);
    ROM_TimerIntEnable(this->base, TIMER_TIMA_TIMEOUT);
    ROM_TimerEnable(this->base, TIMER_A);

    return this;
}

uint32_t timerNow(Timer *this)
{
	uint32_t frac = ROM_TimerValueGet(this->base, TIMER_A) / this->fracLen;
	return (this->count << 12) + (frac & 0x0FFF);	// 20.12
}

void timerAdjust(Timer *this, uint32_t offset)
{
	double offset_i, offset_f;
	offset_f = modf((double)offset / (1<<12), &offset_i);	// Split into integer and fractional parts
	int32_t tav = HWREG(TIMER0_BASE + TIMER_O_TAV) + (offset_f * this->periodTicks);
	if (tav > this->periodTicks)
	{
		offset_i++;
		tav -= this->periodTicks;
	} else if (tav < 0)
	{
		offset_i--;
		tav += this->periodTicks;
	}
	this->count += offset_i;
	HWREG(TIMER0_BASE + TIMER_O_TAV) = tav;
	this->adjusted = true;
}

void timerIntHandler(Timer *this)
{
	ROM_TimerIntClear(this->base, TIMER_TIMA_TIMEOUT);
	this->count++;
	if (this->callback) this->callback(this);
}

void timer0IntHandler() { timerIntHandler(these[0]); }
void timer1IntHandler() { timerIntHandler(these[1]); }
void timer2IntHandler() { timerIntHandler(these[2]); }
void timer3IntHandler() { timerIntHandler(these[3]); }
void timer4IntHandler() { timerIntHandler(these[4]); }
void timer5IntHandler() { timerIntHandler(these[5]); }
