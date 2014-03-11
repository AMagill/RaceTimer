#include "main.h"
#include "XBee.h"
#include "Time.h"
#include "Protocol.h"

void pcFrameReceived(uint8_t *frame, uint32_t size)
{
	switch (frame[0])	// Switch by frame type
	{
	case 0x81:	// RX with 16-bit address
	{
		uint16_t source  = (frame[1] << 8) | frame[2];
		//uint8_t  rssi    = frame[3];
		//uint8_t  options = frame[4];

		switch (frame[5])
		{
		case 's':	// Sync request
		{
			if (size - 5 < sizeof(syncRequestMsg)) break;
			syncRequestMsg *msg = (syncRequestMsg*)(&frame[5]);
			syncReplyMsg reply;
			reply.type = 'S';
			reply.time0 = msg->time0;
			reply.time1 = timeNow();
			xbSendFrameTx16(source, false, (uint8_t*)&reply, sizeof(reply));
			break;
		}
		case 'S':	// Sync reply
		{
			if (size - 5 < sizeof(syncReplyMsg)) break;
			syncReplyMsg *msg = (syncReplyMsg*)(&frame[5]);
			uint32_t time2 = timeNow();
			int64_t theta_fp = msg->time1 - ((int64_t)msg->time0 + time2)/2;
			double theta = (double)theta_fp / (1<<16);
			timeSync(theta);
			break;
		}
		default:	// Unknown incoming packet
			break;
		}
		break;
	}
	default:
		break;
	}

	GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, GPIO_PIN_2);
	SysCtlDelay(SysCtlClockGet() / (1000 * 3));
	GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, 0);
}
