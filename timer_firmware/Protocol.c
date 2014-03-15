#include "main.h"
#include "XBee.h"
#include "Timer.h"
#include "Protocol.h"

uint32_t lastHeartbeat = 0, lastTimeError = INT32_MAX;

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
		case 'S':	// Heartbeat
		{
			if (size - 5 < sizeof(heartbeatMsg)) break;
			heartbeatMsg *msg = (heartbeatMsg*)body;
			uint32_t now = timerNow(rtc);
			int64_t delta = msg->time - now;
			//if (!(rtc->count & 0x0F))
				timerAdjust(rtc, delta);
			lastTimeError = delta;
			lastHeartbeat = timerNow(rtc);
			break;
		}
		default:	// Unknown incoming packet
			break;
		}

		GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, GPIO_PIN_2);
		SysCtlDelay(SysCtlClockGet() / (1000 * 3));
		GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, 0);

		break;
	}
	default:
		break;
	}
}

void pcSendUpdate(uint16_t addr, uint32_t timeDown, uint32_t timeUp)
{
	updateMsg msg;
	msg.type     = 'u';
	msg.timeErr  = lastTimeError;
	msg.timeDown = timeDown;
	msg.timeUp   = timeUp;
	xbSendFrameTx16(addr, XB_OPT_NONE, (uint8_t*)&msg, sizeof(msg));
}

