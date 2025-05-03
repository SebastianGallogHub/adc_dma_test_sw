/*
 * zmodadc1410.h
 *
 *  Created on: Mar 14, 2025
 *      Author: sebas
 */

#ifndef SRC_ZMOD_ADC1410_ZMODADC1410_H_
#define SRC_ZMOD_ADC1410_ZMODADC1410_H_

#include "xparameters.h"
#include "xil_types.h"

#define ZMODADC1410_REG_ADDR_TRIG			0x1C	///< TRIG				register address
#define ZMODADC1410_REGFLD_TRIG_SC1_AC_DC	ZMODADC1410_REG_ADDR_TRIG, 19, 1	///< SC1_AC_DC	field of TRIG register
#define ZMODADC1410_REGFLD_TRIG_SC2_AC_DC	ZMODADC1410_REG_ADDR_TRIG, 20, 1	///< SC2_AC_DC 	field of TRIG register
#define ZMODADC1410_REGFLD_TRIG_SC1_HG_LG	ZMODADC1410_REG_ADDR_TRIG, 21, 1	///< SC1_HG_LG 	field of TRIG register
#define ZMODADC1410_REGFLD_TRIG_SC2_HG_LG	ZMODADC1410_REG_ADDR_TRIG, 22, 1	///< SC2_HG_LG 	field of TRIG register

typedef enum ZMODADC1410_SC{
	SC1, SC2,
} ZmodADC1410_SC;

typedef enum ZOMODADC1410_COUPLING {
	DC_COUPLING,AC_COUPLING,
} ZmodADC1410_COUPLING;

typedef enum ZMODADC1410_GAIN {
	LOW_GAIN, HIGH_GAIN,
} ZmodADC1410_GAIN;

void ZMODADC1410_Init();
void ZMODADC1410_PrintConfigLog(int);
void ZMODADC1410_SetGain(ZmodADC1410_SC channel, ZmodADC1410_GAIN gain);
void ZMODADC1410_SetCoupling(ZmodADC1410_SC channel, ZmodADC1410_COUPLING coupling);
uint16_t ZMODADC1410_ChannelData(ZmodADC1410_SC channel, uint32_t data);
int16_t ZMODADC1410_SignedChannelData(uint8_t channel, uint32_t data);


#endif /* SRC_ZMOD_ADC1410_ZMODADC1410_H_ */
