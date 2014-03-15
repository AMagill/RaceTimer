#ifndef TIMER_H_
#define TIMER_H_

struct _Timer;
typedef void (*TimerCallback)(struct _Timer*);

typedef struct _Timer {
	uint8_t  		timer;
	uint32_t 		base;
	float    		period;
	uint32_t 		periodTicks;
	float    		fracLen;
	uint32_t 		count;
	TimerCallback 	callback;
	bool			adjusted;
} Timer;

Timer* 		timerInit(uint8_t timer, float period, TimerCallback callback);
uint32_t 	timerNow(Timer *this);
void 		timerAdjust(Timer *this, uint32_t offset);

#endif /* TIMER_H_ */
