#pragma once

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "inc/hw_ints.h"
#include "inc/hw_types.h"
#include "inc/hw_timer.h"
#include "inc/hw_memmap.h"
#include "driverlib/debug.h"
#include "driverlib/fpu.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/adc.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "driverlib/timer.h"
#include "driverlib/i2c.h"
#include "Timer.h"

extern Timer* rtc;

#define LED_R	GPIO_PIN_1
#define LED_G	GPIO_PIN_3
#define LED_B	GPIO_PIN_2

#define BTN_1	GPIO_PIN_4
#define BTN_0	GPIO_PIN_2

void gpioBlinkLEDs(uint8_t which);
uint8_t gpioGetButtons();

void delayMs(uint32_t ms);
