#include "main.h"
#include "XBee.h"
#include "Timer.h"
#include "Protocol.h"
#include "Battery.h"


static RxStateCallback rxStateCB;


void pcInit(RxStateCallback rxSCB)
{
	rxStateCB = rxSCB;
}

void pcFrameReceived(uint8_t *frame, uint32_t size)
{
	static const int OS_BODY = 5;  // 1 API ID byte, 8 source address bytes, 1 RSSI byte, 1 options byte

	switch (frame[0])	// Switch by frame type
	{
	case 0x81:	// RX with 16-bit address
	{
		//uint64_t source  = (frame[1] << 56) | (frame[2] << 48) | (frame[3] << 40) | (frame[4] << 32) |
		//				   (frame[5] << 24) | (frame[6] << 16) | (frame[7] <<  8) |  frame[8];
		//uint8_t  rssi	= frame[9];
		//uint8_t  options = frame[10];
		const uint8_t *body = (void*)&frame[OS_BODY];

		switch (body[0])
		{
		case 's':  // Master state message
			if (size >= sizeof(stateMsg) + OS_BODY)
			{
				stateMsg* msg = (stateMsg*)body;
				rxStateCB(msg);
			}
		default:	// Unknown incoming packet
			break;
		}

		break;
	}
	default:
		break;
	}
}

void pcSendState(uint64_t addr, bool isRunning, bool isButtonDown, uint32_t rtcTime, uint32_t timerValue)
{
	// This message is time-sensitive, so don't allow retransmissions.
	// Lost packets are better than late packets here.
	stateMsg msg;
	msg.type	   = 's';
	msg.running    = !!isRunning;
	msg.btnDown    = !!isButtonDown;
	msg._unused    = 0;
	msg.rtcTime	   = rtcTime;
	msg.timerValue = timerValue;
	xbSendFrameTx16(addr, XB_OPT_NONE, (uint8_t*)&msg, sizeof(msg));
}
