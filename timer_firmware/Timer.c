#include "main.h"
#include "Timer.h"
#include "BufferedUART.h"
#include <math.h>

void rtcInit()
{
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_WTIMER0);
    ROM_TimerConfigure(WTIMER0_BASE, TIMER_CFG_SPLIT_PAIR | TIMER_CFG_A_PERIODIC);
    ROM_TimerPrescaleSet(WTIMER0_BASE, TIMER_A, ROM_SysCtlClockGet()/1000);
    //ROM_TimerLoadSet(TIMER0_BASE, TIMER_A, 0xFFFFFFFF);
    ROM_TimerEnable(WTIMER0_BASE, TIMER_A);
}

uint32_t rtcMillis()
{
	return 0xFFFFFFFF - ROM_TimerValueGet(WTIMER0_BASE, TIMER_A);
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
	this->periodTicks 	= period * ROM_SysCtlClockGet();

    //ROM_SysCtlPeripheralEnable(allPeriphs[iBase]);
    ROM_TimerConfigure(this->base, TIMER_CFG_PERIODIC);
    ROM_TimerLoadSet(this->base, this->timer, this->periodTicks);
    TimerIntRegister(this->base, this->timer, callback);
    ROM_TimerIntEnable(this->base, (this->base==TIMER_B)?TIMER_TIMB_TIMEOUT:TIMER_TIMA_TIMEOUT);
    ROM_TimerEnable(this->base, this->timer);

    return this;
}

void timerStart(Timer *this)
{
    ROM_TimerEnable(this->base, this->timer);
}

void timerStop(Timer *this)
{
	ROM_TimerDisable(this->base, this->timer);
}

void timerRestart(Timer *this)
{
	ROM_TimerEnable(this->base, this->timer);
	ROM_TimerLoadSet(this->base, this->timer, this->periodTicks);
}
