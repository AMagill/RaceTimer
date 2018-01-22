#ifndef BIGBUTTON_H_
#define BIGBUTTON_H_

#define BLINK_OFF       0
#define BLINK_ON        1
#define BLINK_SOS       2
#define BLINK_STROBE    3

typedef void (*ButtonCallback)(bool pressed, uint32_t time);

void buttonInit(ButtonCallback btnCB);
void buttonSetLed(bool ledOn);

#endif /* BIGBUTTON_H_ */
