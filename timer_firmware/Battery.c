#include "main.h"
#include "Battery.h"

static uint16_t batteryLevel;

static const uint32_t SEQ = 0;
static const uint16_t BATT_MAX = 2750;  // ~5V
static const uint16_t BATT_MIN = 1770;  // ~3.3V

void batteryADCIntHandler()
{
	uint32_t buf;

	ADCIntClear(ADC0_BASE, SEQ);
	ADCSequenceDataGet(ADC0_BASE, SEQ, &buf);

	batteryLevel = (uint16_t)buf;
}

void batteryInit()
{
	// HACK: Looks like there's a bug, so this has to go before periph enable to be safe.
	// Stellaris -> Tiva changed which registers are used to enable devices, and this
	// function works on the legacy-support register, which can behave oddly.
	//SysCtlADCSpeedSet(SYSCTL_ADCSPEED_250KSPS);

	SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
	GPIOPinTypeADC(GPIO_PORTD_BASE, GPIO_PIN_0);

	ADCHardwareOversampleConfigure(ADC0_BASE, 64);
	ADCSequenceDisable(ADC0_BASE, SEQ);
	ADCSequenceConfigure(ADC0_BASE, SEQ, ADC_TRIGGER_PROCESSOR, 0);
	ADCSequenceStepConfigure(ADC0_BASE, SEQ, 0, ADC_CTL_CH7 | ADC_CTL_IE | ADC_CTL_END);
	ADCSequenceEnable(ADC0_BASE, SEQ);
	ADCIntEnable(ADC0_BASE, SEQ);

	ADCIntRegister(ADC0_BASE, SEQ, batteryADCIntHandler);
}

void batterySampleTrigger()
{
	ADCProcessorTrigger(ADC0_BASE, SEQ);
}

uint16_t batteryGetLevel()
{
	return batteryLevel;
}

uint8_t batteryGetPercent()
{
    if (batteryLevel < BATT_MIN)
        return 0;
    if (batteryLevel > BATT_MAX)
        return 100;
    return (int32_t)(batteryLevel - BATT_MIN) * 100 / (BATT_MAX - BATT_MIN);
}
