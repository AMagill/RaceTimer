#ifndef TIMER_H_
#define TIMER_H_

struct _Timer;
typedef void (*TimerCallback)(struct _Timer*);

typedef struct _Timer {
	uint32_t 		base;
	uint32_t		timer;
	uint32_t 		periodTicks;
} Timer;

void 		rtcInit();
uint32_t 	rtcMillis();
Timer* timerInit(uint32_t base, float period, void (*callback)(void));
void timerStart(Timer *this);
void timerStop(Timer *this);
void timerRestart(Timer *this);

#endif /* TIMER_H_ */
