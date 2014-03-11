#ifndef TIMER_H_
#define TIMER_H_

void timeInit();
uint32_t timeNow();
void timeSync(double offset);
void timeIntHandler();

#endif /* TIMER_H_ */
