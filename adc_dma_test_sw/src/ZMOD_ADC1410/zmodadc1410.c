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
// Parámetros de calibración del canal
#define SC2_LOW_GAIN_CALIB_GAIN		1.0
#define SC2_LOW_GAIN_CALIB_ADITIVE	72.0

/**************************** Type Definitions ******************************/

/************************** Function Prototypes *****************************/
int32_t computeCoefMult(float cg, uint8_t gain);
int32_t computeCoefAdd(float ca, uint8_t gain);
int ZMODADC1410_readUserCalibFromFlashIntoIP();

/************************** Variable Definitions ***************************/

/****************************************************************************/

void ZMODADC1410_PrintConfigLog(int l){
	LOG(l, "ZMOD Scope 1410-105 config:");
	LOG(l+1, "CHA: LOW Gain - Res 3.21mV; DC Coupling");
	LOG(l+1, "CHB: LOW Gain - Res 3.21mV; DC Coupling");
}

void ZMODADC1410_Init(uintptr_t iicAddress, uintptr_t flashAddress)
{
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

//	// Inicializa la flash para la calibración
//	ZMOD_InitFlash(iicAddress, flashAddress);
//	// Configurar la calibración del CHB
//	ZMOD_InitCalib(sizeof(CALIBECLYPSEADC), ZMODADC1410_CALIB_ID, ZMODADC1410_CALIB_USER_ADDR, ZMODADC1410_CALIB_FACT_ADDR);
//	ZMOD_ReadUserCalib();

	// Calibración
	// Coeficientes de fábrica consultados de la flash del ADC, transformados a int32_t
	// por computeCoefMult y computeCoefAdd (18bit), alterados empíricamente para
	// medir correctamente la componente de contínua
	// CHA
	ZMOD_WriteRegFld(ZMODADC1410_REGFLD_SC1LGMULTCOEF_VAL, (uint32_t)(0x1087E));			// Fábrica = 0x10993
	ZMOD_WriteRegFld(ZMODADC1410_REGFLD_SC1LGADDCOEF_VAL,  (uint32_t)(0x002F9-0x0020F));	// Fábrica = 0x3FE42

	// CHB
	ZMOD_WriteRegFld(ZMODADC1410_REGFLD_SC2LGMULTCOEF_VAL, (uint32_t)(0x1087E));			// Fábrica = 0x1087E
	ZMOD_WriteRegFld(ZMODADC1410_REGFLD_SC2LGADDCOEF_VAL,  (uint32_t)(0x002F9+0x002CF));	// Fábrica = 0x002F9

	// Ganancia y acoplamiento por canal
	// CHA
	ZMODADC1410_SetGain(SC1, LOW_GAIN);
	ZMODADC1410_SetCoupling(SC1, DC_COUPLING);

	// CHB
	ZMODADC1410_SetGain(SC2, LOW_GAIN);
	ZMODADC1410_SetCoupling(SC2, DC_COUPLING);

	// Retardo para que las configuraciones surtan efecto
	usleep(900000U);

//	// Libero la memoria utilizada para configurar la flash
//	ZMOD_DeinitFlash();

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

/**
 * Set a pair of calibration values for a specific channel and gain into the ZMOD_Calib area (interpreted as CALIBECLYPSEADC).
 * In order for this change to be applied to user calibration area from flash, writeUserCalib function must be called.
 * @param channel the channel for which calibration values are set: 0 for channel 1, 1 for channel 2
 * @param gain the gain for which calibration values are set: 0 for LOW gain, 1 for HIGH gain
 * @param valG the gain calibration value to be set
 * @param valA the additive calibration value to be set
 */
void ZMODADC1410_SetCalibValues(uint8_t channel, uint8_t gain, float valG, float valA)
{
	CALIBECLYPSEADC *pCalib;
	if(ZMOD_Calib)
	{
		// interpret the ZMOD_Calib data as a CALIBECLYPSEADC data
		pCalib = (CALIBECLYPSEADC *)ZMOD_Calib;
		pCalib->cal[channel][gain][0] = valG;
		pCalib->cal[channel][gain][1] = valA;
	}
}

/**
 * Computes the Multiplicative calibration coefficient.
 * @param cg gain coefficient as it is stored in Flash
 * @param gain 0 LOW and 1 HIGH
 * @return a signed 32 value containing the multiplicative coefficient in the 18 lsb bits: bit 17 sign, bit 16-0 the value
 */
int32_t computeCoefMult(float cg, uint8_t gain)
{
	float fval = (gain ? (REAL_RANGE_ADC_HIGH/IDEAL_RANGE_ADC_HIGH):(REAL_RANGE_ADC_LOW/IDEAL_RANGE_ADC_LOW))*(1 + cg)*(float)(1<<16);
	int32_t ival = (int32_t) (fval + 0.5);	// round
	ival &= (1<<18) - 1; // keep only 18 bits
	return ival;
}

/**
 * Computes the Additive calibration coefficient.
 * @param ca - add coefficient as it is stored in Flash
 * @param gain 0 LOW and 1 HIGH
 * @return a signed 32 value containing the additive coefficient in the 18 lsb bits: bit 17 sign, bit 16-0 the value
 */
int32_t computeCoefAdd(float ca, uint8_t gain)
{
	float fval = ca / (gain ? IDEAL_RANGE_ADC_HIGH:IDEAL_RANGE_ADC_LOW)*(float)(1<<17);
	int32_t ival = (int32_t) (fval + 0.5);	// round
	ival &= (1<<18) - 1; // keep only 18 bits
	return ival;
}

/**
* Reads the calibration data into the calib class member.
* It calls the readUserCalib from the ZMOD class level to read the user calibration data
*  as an array of bytes into the area pointed by calib base class member.
* Then the calibration data is interpreted according to CALIBECLYPSEADC structure.
 * @return the status: ERR_SUCCESS for success, ERR_CALIB_ID for calib ID error,
 *  ERR_CALIB_CRC for CRC error, ERR_FAIL if calibration is not initialized.
*/
int ZMODADC1410_readUserCalibFromFlashIntoIP()
{
	int status;
	CALIBECLYPSEADC *pCalib;

	if(!ZMOD_Calib)
	{
		return ERR_FAIL;
	}
	// read the user calibration data as an array of bytes, into the area pointed by calib base class member
	status = ZMOD_ReadUserCalib();
	if (status == ERR_SUCCESS)
	{
		// interpret the calib data as a CALIBECLYPSEADC data
		pCalib = (CALIBECLYPSEADC *)ZMOD_Calib;

		// fill the calibration related registers
		// float           cal[2][2][2];   // [channel 0:1][low/high gain 0:1][0 multiplicative : 1 additive]
		ZMOD_WriteRegFld(ZMODADC1410_REGFLD_SC1HGMULTCOEF_VAL, computeCoefMult(pCalib->cal[0][1][0], 1));
		ZMOD_WriteRegFld(ZMODADC1410_REGFLD_SC1HGADDCOEF_VAL,   computeCoefAdd(pCalib->cal[0][1][1], 1));
		ZMOD_WriteRegFld(ZMODADC1410_REGFLD_SC1LGMULTCOEF_VAL, computeCoefMult(pCalib->cal[0][0][0], 0));
		ZMOD_WriteRegFld(ZMODADC1410_REGFLD_SC1LGADDCOEF_VAL,   computeCoefAdd(pCalib->cal[0][0][1], 0));

		ZMOD_WriteRegFld(ZMODADC1410_REGFLD_SC2HGMULTCOEF_VAL, computeCoefMult(pCalib->cal[1][1][0], 1));
		ZMOD_WriteRegFld(ZMODADC1410_REGFLD_SC2HGADDCOEF_VAL,   computeCoefAdd(pCalib->cal[1][1][1], 1));
		ZMOD_WriteRegFld(ZMODADC1410_REGFLD_SC2LGMULTCOEF_VAL, computeCoefMult(pCalib->cal[1][0][0], 0));
		ZMOD_WriteRegFld(ZMODADC1410_REGFLD_SC2LGADDCOEF_VAL,   computeCoefAdd(pCalib->cal[1][0][1], 0));

	}

	return ERR_SUCCESS;
}
