#ifndef TIMER_H_
#define TIMER_H_

typedef void (*TickCallback)(uint32_t);


void timeInit();
uint32_t timeNow();
void timeSync(double offset);
void timeUnsync();
void timeIntHandler();
void timeSetTickCB(TickCallback cb);

#endif /* TIMER_H_ */
