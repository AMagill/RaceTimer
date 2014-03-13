#ifndef XBEE_H_
#define XBEE_H_

#define XB_OPT_NONE   0
#define XB_OPT_NO_ACK 1

typedef void (*FrameCallback)(uint8_t*,uint32_t);

void xbInit();
void xbUARTIntHandler();
void xbSendFrameTx16(uint16_t address, uint8_t opts, const uint8_t *msg, uint16_t length);
void xbSetFrameCB(FrameCallback cb);

#endif /* XBEE_H_ */
