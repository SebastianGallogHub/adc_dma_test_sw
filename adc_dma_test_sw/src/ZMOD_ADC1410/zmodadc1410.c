/*
 * zmodadc1410.c
 *
 *  Created on: Mar 14, 2025
 *      Author: sebas
 */

/***************************** Include Files *******************************/

#include "../ZMOD_ADC1410/zmodadc1410.h"

#include <sleep.h>

#include "../ZMOD/zmod.h"
#include "../includes/log.h"

/************************** Constant Definitions **************************/

/**************************** Type Definitions ******************************/

/************************** Function Prototypes *****************************/

/************************** Variable Definitions ***************************/

/****************************************************************************/

void ZMODADC1410_PrintConfigLog(int l){
	LOG(l, "ZMOD Scope 1410-105 config:");
	LOG(l+1, "CHA: LOW Gain - Res 3.21mV; DC Coupling");
	LOG(l+1, "CHB: LOW Gain - Res 3.21mV; DC Coupling");
}

void ZMODADC1410_Init()
{
//	LOG(1, "ZMODADC1410_Init");
	/*
	 * Esta función se comunica con ZMOD1410 AXI ADAPTER para
	 * configurar el ZMOD1410 a través del LLC. Este es su único propósito.
	 *
	 * El resto de las comunicaciones se realizarán mediante AXITAR_*() + AXIDMA_*()
	 * */

	// Reseteo el IP AXI_ZmodADC1410
	ZMOD_WriteRegFld(ZMOD_REGFLD_CR_RST, 1);
	usleep(1000U);

	ZMOD_WriteRegFld(ZMOD_REGFLD_CR_RST, 0);
	usleep(1000U);

	//Canal 1
	ZMODADC1410_SetGain(SC1, LOW_GAIN);
	ZMODADC1410_SetCoupling(SC1, DC_COUPLING);

	//Canal 2
	ZMODADC1410_SetGain(SC2, LOW_GAIN);
	ZMODADC1410_SetCoupling(SC2, DC_COUPLING);

	// Retardo para que las configuraciones surtan efecto
	usleep(900000U);

	return;
}
void ZMODADC1410_SetGain(ZmodADC1410_SC channel, ZmodADC1410_GAIN gain) {
	if(channel == SC1)
		ZMOD_WriteRegFld(ZMODADC1410_REGFLD_TRIG_SC1_HG_LG, gain);
	else
		ZMOD_WriteRegFld(ZMODADC1410_REGFLD_TRIG_SC2_HG_LG, gain);
}

void ZMODADC1410_SetCoupling(ZmodADC1410_SC channel, ZmodADC1410_COUPLING coupling) {
	if(channel == SC1)
		ZMOD_WriteRegFld(ZMODADC1410_REGFLD_TRIG_SC1_AC_DC, coupling);
	else
		ZMOD_WriteRegFld(ZMODADC1410_REGFLD_TRIG_SC2_AC_DC, coupling);
}

uint16_t ZMODADC1410_ChannelData(ZmodADC1410_SC channel, uint32_t data) {
	//					SC2(LSB)	  SC1(MSB)
	return (channel ? (data >> 2) : (data >> 18)) & 0x00003FFF;
}

int16_t ZMODADC1410_SignedChannelData(ZmodADC1410_SC channel, uint32_t data) {
	return ZMOD_ToSigned(ZMODADC1410_ChannelData(channel, data), 14);
}
