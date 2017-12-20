#include "main.h"
#include "Display.h"

static uint16_t frameBuf[8] = {0, 0, 0, 0, 0, 0, 0, 0};

static void displaySendArray(uint8_t addr, uint8_t cmd, const uint8_t* msg, uint16_t length);
static void displaySendSingle(uint8_t addr, uint8_t msg);

static const uint8_t DISP_L_ADDR = 0x70;
static const uint8_t DISP_R_ADDR = 0x71;


//  #########=0001=########
//
//  ##  ##     ##     ##  ##
//  0020 0100 0200 0400 0002
//  ##    ##   ##   ##    ##
//  ##     ##  ##  ##     ##
//
//  ###=0040=##  ##=0080=###
//
//  ##     ##  ##  ##     ##
//  ##    ##   ##   ##    ##
//  0010 0800 1000 2000 0004
//  ##  ##     ##     ##  ##   /    \
//                             (4000)
//  #########=0008=#########   \    /
//

static const uint16_t fontTable[] =
{
    0b0000000000000001,
    0b0000000000000010,
    0b0000000000000100,
    0b0000000000001000,
    0b0000000000010000,
    0b0000000000100000,
    0b0000000001000000,
    0b0000000010000000,
    0b0000000100000000,
    0b0000001000000000,
    0b0000010000000000,
    0b0000100000000000,
    0b0001000000000000,
    0b0010000000000000,
    0b0100000000000000,
    0b1000000000000000,
    0b0000000000000000,
    0b0000000000000000,
    0b0000000000000000,
    0b0000000000000000,
    0b0000000000000000,
    0b0000000000000000,
    0b0000000000000000,
    0b0000000000000000,
    0b0001001011001001,
    0b0001010111000000,
    0b0001001011111001,
    0b0000000011100011,
    0b0000010100110000,
    0b0001001011001000,
    0b0011101000000000,
    0b0001011100000000,
    0b0000000000000000, //
    0b0110000100000000, // !
    0b0000001000100000, // "
    0b0001001011001110, // #
    0b0001001011101101, // $
    0b0000110000100100, // %
    0b0010001101011101, // &
    0b0000010000000000, // '
    0b0010010000000000, // (
    0b0000100100000000, // )
    0b0011111111000000, // *
    0b0001001011000000, // +
    0b0000100000000000, // ,
    0b0000000011000000, // -
    0b0000000000000000, // .
    0b0000110000000000, // /
    0b0000110000111111, // 0
    0b0000000000000110, // 1
    0b0000000011011011, // 2
    0b0000000010001111, // 3
    0b0000000011100110, // 4
    0b0010000001101001, // 5
    0b0000000011111101, // 6
    0b0000000000000111, // 7
    0b0000000011111111, // 8
    0b0000000011101111, // 9
    0b0001001000000000, // :
    0b0000101000000000, // ;
    0b0010010000000000, // <
    0b0000000011001000, // =
    0b0000100100000000, // >
    0b0001000010000011, // ?
    0b0000001010111011, // @
    0b0000000011110111, // A
    0b0001001010001111, // B
    0b0000000000111001, // C
    0b0001001000001111, // D
    0b0000000011111001, // E
    0b0000000001110001, // F
    0b0000000010111101, // G
    0b0000000011110110, // H
    0b0001001000000000, // I
    0b0000000000011110, // J
    0b0010010001110000, // K
    0b0000000000111000, // L
    0b0000010100110110, // M
    0b0010000100110110, // N
    0b0000000000111111, // O
    0b0000000011110011, // P
    0b0010000000111111, // Q
    0b0010000011110011, // R
    0b0000000011101101, // S
    0b0001001000000001, // T
    0b0000000000111110, // U
    0b0000110000110000, // V
    0b0010100000110110, // W
    0b0010110100000000, // X
    0b0001010100000000, // Y
    0b0000110000001001, // Z
    0b0000000000111001, // [
    0b0010000100000000, //
    0b0000000000001111, // ]
    0b0000110000000011, // ^
    0b0000000000001000, // _
    0b0000000100000000, // `
    0b0001000001011000, // a
    0b0010000001111000, // b
    0b0000000011011000, // c
    0b0000100010001110, // d
    0b0000100001011000, // e
    0b0000000001110001, // f
    0b0000010010001110, // g
    0b0001000001110000, // h
    0b0001000000000000, // i
    0b0000000000001110, // j
    0b0011011000000000, // k
    0b0000000000110000, // l
    0b0001000011010100, // m
    0b0001000001010000, // n
    0b0000000011011100, // o
    0b0000000101110000, // p
    0b0000010010000110, // q
    0b0000000001010000, // r
    0b0010000010001000, // s
    0b0000000001111000, // t
    0b0000000000011100, // u
    0b0010000000000100, // v
    0b0010100000010100, // w
    0b0010100011000000, // x
    0b0010000000001100, // y
    0b0000100001001000, // z
    0b0000100101001001, // {
    0b0001001000000000, // |
    0b0010010010001001, // }
    0b0000010100100000, // ~
    0b0011111111111111,
};


// Uses I2C2: clock PE4 + data PE5
void displayInit()
{
    // Enable hardware
    SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C2);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);

    GPIOPinConfigure(GPIO_PE4_I2C2SCL);
    GPIOPinConfigure(GPIO_PE5_I2C2SDA);

    GPIOPinTypeI2CSCL(GPIO_PORTE_BASE, GPIO_PIN_4);
    GPIOPinTypeI2C(GPIO_PORTE_BASE,    GPIO_PIN_5);

    I2CMasterInitExpClk(I2C2_BASE, SysCtlClockGet(), false);

    displaySendSingle(DISP_L_ADDR, 0x21);  // Enable oscillators
    displaySendSingle(DISP_R_ADDR, 0x21);

    displayUpdate();                       // Initialize frame

    displaySendSingle(DISP_L_ADDR, 0x81);  // Displays on, no blink
    displaySendSingle(DISP_R_ADDR, 0x81);
}

void displaySetText(const char* text)
{
    int i;
    for (i = 0; i < 8; i++)
    {
        if (text[i] == 0)
            break;
        displaySetChar(i, text[i]);
    }

    // If we ran into a null terminator, blank the rest
    for (; i < 8; i++)
        displaySetChar(i, ' ');
}

void displaySetChar(uint8_t index, uint8_t ch)
{
    if (index < 8)
        frameBuf[index] = fontTable[ch];
}

void displayUpdate()
{
    displaySendArray(DISP_L_ADDR, 0x00, (uint8_t*)frameBuf,   8);
    displaySendArray(DISP_R_ADDR, 0x00, (uint8_t*)frameBuf+8, 8);
}

void displayScrollText(const char* text, uint32_t delayMs)
{
    bool done = false;
    int i = 0;

    // Scan for nulls to see if we have at least a screenfull
    for (i = 0; i < 7; i++)
        done |= (text[i] == 0);

    // Draw at least once, and then until we're out of screenfulls
    i = 0;
    do
    {
        displaySetText(text+i);
        displayUpdate();
        SysCtlDelay(SysCtlClockGet() / 3000 * delayMs);
        done |= (text[i+8] == 0);
        i++;
    } while (!done);
}



static void displaySendSingle(uint8_t addr, uint8_t cmd)
{
    I2CMasterSlaveAddrSet(I2C2_BASE, addr, false);
    I2CMasterDataPut(I2C2_BASE, cmd);
    I2CMasterControl(I2C2_BASE, I2C_MASTER_CMD_SINGLE_SEND);
    while(I2CMasterBusy(I2C2_BASE));
}

static void displaySendArray(uint8_t addr, uint8_t cmd, const uint8_t* data, uint16_t length)
{
    if (length == 0)
        return displaySendSingle(addr, cmd);

    I2CMasterSlaveAddrSet(I2C2_BASE, addr, false);
    I2CMasterDataPut(I2C2_BASE, cmd);
    I2CMasterControl(I2C2_BASE, I2C_MASTER_CMD_BURST_SEND_START);
    while(I2CMasterBusy(I2C2_BASE));

    int i;
    for (i = 0; i < length-1; i++)
    {
        I2CMasterDataPut(I2C2_BASE, data[i]);
        I2CMasterControl(I2C2_BASE, I2C_MASTER_CMD_BURST_SEND_CONT);
        while(I2CMasterBusy(I2C2_BASE));
    }

    I2CMasterDataPut(I2C2_BASE, data[i]);
    I2CMasterControl(I2C2_BASE, I2C_MASTER_CMD_BURST_SEND_FINISH);
    while(I2CMasterBusy(I2C2_BASE));
}


