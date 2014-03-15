#ifndef PROTOCOL_H_
#define PROTOCOL_H_

typedef struct __attribute__((packed)) {
	char type;
	uint32_t time;		// 20.12 fixed-point
	uint8_t flags;
} heartbeatMsg;
#define HARTBEATMSG_FLAG_SYNC 1

typedef struct __attribute__((packed)) {
	char type;
	int32_t timeErr;
	uint32_t timeDown, timeUp;		// 20.12 fixed-point
} updateMsg;


void pcFrameReceived(uint8_t *frame, uint32_t size);
void pcSendUpdate(uint16_t addr, uint32_t timeDown, uint32_t timeUp);


#endif /* PROTOCOL_H_ */
