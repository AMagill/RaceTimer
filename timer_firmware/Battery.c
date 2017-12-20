#include "main.h"
#include "Battery.h"

static uint16_t batteryLevel;

#define SEQ 0

void batteryADCIntHandler()
{
	uint32_t buf;

	ROM_ADCIntClear(ADC0_BASE, SEQ);
	ROM_ADCSequenceDataGet(ADC0_BASE, SEQ, &buf);

	batteryLevel = (uint16_t)buf;
}

void batteryInit()
{
	// HACK: Looks like there's a bug, so this has to go before periph enable to be safe.
	// Stellaris -> Tiva changed which registers are used to enable devices, and this
	// function works on the legacy-support register, which can behave oddly.
	//ROM_SysCtlADCSpeedSet(SYSCTL_ADCSPEED_250KSPS);

	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
	ROM_GPIOPinTypeADC(GPIO_PORTD_BASE, GPIO_PIN_0);

	ROM_ADCHardwareOversampleConfigure(ADC0_BASE, 64);

	ROM_ADCSequenceDisable(ADC0_BASE, SEQ);
	ROM_ADCSequenceConfigure(ADC0_BASE, SEQ, ADC_TRIGGER_PROCESSOR, 0);
	ROM_ADCSequenceStepConfigure(ADC0_BASE, SEQ, 0, ADC_CTL_CH7 | ADC_CTL_IE | ADC_CTL_END);
	ROM_ADCSequenceEnable(ADC0_BASE, SEQ);
	ROM_ADCIntEnable(ADC0_BASE, SEQ);

	ADCIntRegister(ADC0_BASE, SEQ, batteryADCIntHandler);
}

void batterySampleTrigger()
{
	ROM_ADCProcessorTrigger(ADC0_BASE, SEQ);
}

uint16_t batteryGetLevel()
{
	return batteryLevel;
}


