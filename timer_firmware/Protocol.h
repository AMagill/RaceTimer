#ifndef PROTOCOL_H_
#define PROTOCOL_H_

#define EVT_BTN_DOWN 'd'
#define EVT_BTN_UP   'u'

typedef struct __attribute__((packed)) {
	char      type;
	uint32_t  time;		// microseconds
	uint8_t   battery;
} heartbeatMsg;

typedef struct __attribute__((packed)) {
	char      type;
} pingMsg;

typedef struct __attribute__((packed)) {
	char      type;
	uint32_t  eventTime;
	char      eventType;
} eventMsg;


void pcFrameReceived(uint8_t *frame, uint32_t size);
void pcSendHeartbeat();
void pcSendEvent(uint32_t eventTime, char eventType);
uint32_t pcLastHeard();


#endif /* PROTOCOL_H_ */
