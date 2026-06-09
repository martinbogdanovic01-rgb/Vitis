/* Exercise to practice Software Layered Architecture diagrams. See instructions.txt
*
* Author: Brice Guayrin (b.guayrin@fontys.nl)
*
* Date: Marrch 2025
*/

#include "voltage.h"

#include "xadcps.h"

static XAdcPs xadc;

static float xyz_raw_to_voltage(u16 raw_data)
{
	float voltage = (((float)(raw_data)) * 3.3f)/65536.0f;
	return voltage;
}

void xyz_init()
{
	int Status;
	XAdcPs_Config *ConfigPtr;
	XAdcPs *XAdcInstPtr = &xadc;

	ConfigPtr = XAdcPs_LookupConfig(XADC_DEVICE_ID);
	if (ConfigPtr == NULL) { return; }

	Status = XAdcPs_CfgInitialize(XAdcInstPtr, ConfigPtr, ConfigPtr->BaseAddress);
	if (Status != XST_SUCCESS) { return; }

	Status = XAdcPs_SelfTest(XAdcInstPtr);
	if (Status != XST_SUCCESS) { return; }

	XAdcPs_SetSequencerMode(XAdcInstPtr, XADCPS_SEQ_MODE_CONTINPASS);
}

float xyz_get_voltage(u8 channel)
{
	u16 raw_data = XAdcPs_GetAdcData(&xadc,   XADCPS_CH_AUX_MIN + channel);
	float voltage = xyz_raw_to_voltage(raw_data);
	return voltage;
}

