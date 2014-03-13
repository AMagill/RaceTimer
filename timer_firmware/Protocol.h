#ifndef PROTOCOL_H_
#define PROTOCOL_H_

typedef struct __attribute__((packed)) {
	char type;
	uint32_t time0;				// 16.16 fixed-point
} syncRequestMsg;

typedef struct __attribute__((packed)) {
	char type;
	uint32_t time0, time1;		// 16.16 fixed-point
} syncReplyMsg;


void pcFrameReceived(uint8_t *frame, uint32_t size);
void pcSendSyncRequest(uint16_t addr);


#endif /* PROTOCOL_H_ */
