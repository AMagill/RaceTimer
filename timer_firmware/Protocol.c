#include "main.h"
#include "XBee.h"
#include "Timer.h"
#include "Protocol.h"
#include "Battery.h"

#define MASTER_ADDR 0x0000

uint32_t lastHeardMaster = 0;

void pcFrameReceived(uint8_t *frame, uint32_t size)
{
	switch (frame[0])	// Switch by frame type
	{
	case 0x80:	// RX with 64-bit address
	{
		//uint64_t source  = (frame[1] << 56) | (frame[2] << 48) | (frame[3] << 40) | (frame[4] << 32) |
		//		           (frame[5] << 24) | (frame[6] << 16) | (frame[7] <<  8) |  frame[8];
		//uint8_t  rssi    = frame[9];
		//uint8_t  options = frame[10];
		const uint8_t *body = (void*)&frame[11];

		switch (body[0])
		{
		default:	// Unknown incoming packet
			break;
		}

		break;
	}
	case 0x89:	// TX Status
		if (frame[2] == 0)		// Success
			lastHeardMaster = rtcMillis();
		break;
	default:
		break;
	}
}

void pcSendHeartbeat()
{
	// This is timing-critical, so ensure that the XBee is awake and ready to go first.
	xbSleep(false);

	// Heartbeat message is time-sensitive, so don't allow retransmissions.
	// Lost packets are better than late packets here.
	heartbeatMsg msg;
	msg.type     = 'h';
	msg.time     = rtcMillis();
	msg.battery  = batteryGetLevel();
	xbSendFrameTx16(MASTER_ADDR, XB_OPT_NO_ACK, (uint8_t*)&msg, sizeof(msg));

	// Still, it's nice to know if someone is listening, so send a ping
	// just so we can see if it gets ACKed.
	pingMsg msg2;
	msg.type     = 'p';
	xbSendFrameTx16(MASTER_ADDR, XB_OPT_NONE, (uint8_t*)&msg2, sizeof(msg2));
}


void pcSendEvent(uint32_t eventTime, char eventType)
{
	eventMsg msg;
	msg.type      = 'e';
	msg.eventTime = eventTime;
	msg.eventType = eventType;
	xbSendFrameTx16(MASTER_ADDR, XB_OPT_NONE, (uint8_t*)&msg, sizeof(msg));
}

uint32_t pcLastHeard()
{
	return rtcMillis() - lastHeardMaster;
}
