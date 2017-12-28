#include "main.h"
#include "Timer.h"
#include "BufferedUART.h"
#include <math.h>

void rtcInit()
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_WTIMER0);
    TimerConfigure(WTIMER0_BASE, TIMER_CFG_SPLIT_PAIR | TIMER_CFG_A_PERIODIC);
    TimerPrescaleSet(WTIMER0_BASE, TIMER_A, SysCtlClockGet()/1000);
    //TimerLoadSet(TIMER0_BASE, TIMER_A, 0xFFFFFFFF);
    TimerEnable(WTIMER0_BASE, TIMER_A);
}

uint32_t rtcMillis()
{
	return 0xFFFFFFFF - TimerValueGet(WTIMER0_BASE, TIMER_A);
}

int32_t rtcSub(uint32_t t1, uint32_t t2)
{
	// Yes, this is stupidly simple, but there's too much risk for bugs if the
	// typecast isn't done properly.
	return (int32_t)(t1-t2);
}


Timer* timerInit(uint32_t base, float period, void (*callback)(void))
{
	Timer *this = malloc(sizeof(Timer));
	this->base 			= base;
	this->timer 		= TIMER_BOTH;
	this->periodTicks 	= period * SysCtlClockGet();

    //SysCtlPeripheralEnable(allPeriphs[iBase]);
    TimerConfigure(this->base, TIMER_CFG_PERIODIC);
    TimerLoadSet(this->base, this->timer, this->periodTicks);
    TimerIntRegister(this->base, this->timer, callback);
    TimerIntEnable(this->base, (this->base==TIMER_B)?TIMER_TIMB_TIMEOUT:TIMER_TIMA_TIMEOUT);
    TimerEnable(this->base, this->timer);

    return this;
}

void timerStart(Timer *this)
{
    TimerEnable(this->base, this->timer);
}

void timerStop(Timer *this)
{
	TimerDisable(this->base, this->timer);
}

void timerRestart(Timer *this)
{
	TimerEnable(this->base, this->timer);
	TimerLoadSet(this->base, this->timer, this->periodTicks);
}
