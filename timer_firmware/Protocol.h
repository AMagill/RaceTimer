#pragma once

typedef struct __attribute__((packed)) {
    char      type;
    uint8_t   running : 1;
    uint8_t   btnDown : 1;
    uint8_t   _unused : 6;
    uint32_t  rtcTime;     // milliseconds
    uint32_t  timerValue;  // milliseconds
} stateMsg;

typedef void (*RxStateCallback)(const stateMsg* msg);

void pcInit(RxStateCallback rxStateCB);
void pcFrameReceived(uint8_t *frame, uint32_t size);
void pcSendState(uint64_t addr, bool isRunning, bool isButtonDown, uint32_t rtcTime, uint32_t timerValue);
