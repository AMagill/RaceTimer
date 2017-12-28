#pragma once

void displayInit();
void displaySetText(const char* text);
void displaySetChar(uint8_t index, uint8_t ch);
void displaySetNumber(uint8_t index, uint8_t num);
void displaySetTime(uint32_t timeMs);
void displayUpdate();
void displayScrollText(const char* text, uint32_t delayMs);
