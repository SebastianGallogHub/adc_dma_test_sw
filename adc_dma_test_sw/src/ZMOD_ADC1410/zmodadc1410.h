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

#define IDEAL_RANGE_ADC_HIGH 1.0 ///< Ideal range ADC LOW
#define IDEAL_RANGE_ADC_LOW 25.0 ///< Ideal range ADC HIGH
#define REAL_RANGE_ADC_HIGH 1.086 ///< Real range ADC HIGH
#define REAL_RANGE_ADC_LOW 26.25 ///< Real range ADC LOW

/****************************************************************************/
/**
 * ZMODADC1410 calibration registers
 */
#define ZMODADC1410_REG_ADDR_SC1LGMULTCOEF	0x24	///< SC1LGMULTCOEF 	register address
#define ZMODADC1410_REG_ADDR_SC1LGADDCOEF	0x28	///< SC1LGADDCOEF		register address
#define ZMODADC1410_REG_ADDR_SC1HGMULTCOEF	0x2C	///< SC1HGMULTCOEF	register address
#define ZMODADC1410_REG_ADDR_SC1HGADDCOEF	0x30	///< SC1HGADDCOEF 	register address
#define ZMODADC1410_REG_ADDR_SC2LGMULTCOEF	0x34	///< SC2LGMULTCOEF 	register address
#define ZMODADC1410_REG_ADDR_SC2LGADDCOEF	0x38	///< SC2LGADDCOEF 	register address
#define ZMODADC1410_REG_ADDR_SC2HGMULTCOEF	0x3C	///< SC2HGMULTCOEF 	register address
#define ZMODADC1410_REG_ADDR_SC2HGADDCOEF	0x40	///< SC2HGADDCOEF 	register address

#define ZMODADC1410_REGFLD_SC1LGMULTCOEF_VAL	ZMODADC1410_REG_ADDR_SC1LGMULTCOEF, 0, 18	///< VAL field of SC1LGMULTCOEF register
#define ZMODADC1410_REGFLD_SC1HGMULTCOEF_VAL	ZMODADC1410_REG_ADDR_SC1HGMULTCOEF, 0, 18	///< VAL field of SC1HGMULTCOEF register
#define ZMODADC1410_REGFLD_SC1LGADDCOEF_VAL		ZMODADC1410_REG_ADDR_SC1LGADDCOEF, 0, 18	///< VAL field of SC1LGADDCOEF register
#define ZMODADC1410_REGFLD_SC1HGADDCOEF_VAL		ZMODADC1410_REG_ADDR_SC1HGADDCOEF, 0, 18	///< VAL field of SC1HGADDCOEF register
#define ZMODADC1410_REGFLD_SC2LGMULTCOEF_VAL	ZMODADC1410_REG_ADDR_SC2LGMULTCOEF, 0, 18	///< VAL field of SC2LGMULTCOEF register
#define ZMODADC1410_REGFLD_SC2HGMULTCOEF_VAL	ZMODADC1410_REG_ADDR_SC2HGMULTCOEF, 0, 18	///< VAL field of SC2HGMULTCOEF register
#define ZMODADC1410_REGFLD_SC2LGADDCOEF_VAL		ZMODADC1410_REG_ADDR_SC2LGADDCOEF, 0, 18	///< VAL field of SC2LGADDCOEF register
#define ZMODADC1410_REGFLD_SC2HGADDCOEF_VAL		ZMODADC1410_REG_ADDR_SC2HGADDCOEF, 0, 18	///< VAL field of SC2HGADDCOEF register
/**
 * ZMODADC1410 calibration conf datas
 */
#define ZMODADC1410_CALIB_USER_ADDR 0x7000 ///< Address in flash for user calibration area
#define ZMODADC1410_CALIB_FACT_ADDR 0x8100 ///< Address in flash for factory calibration area
#define ZMODADC1410_CALIB_ID	0xAD ///< Calibration ID

/**
 * Struct that maps the calibration data stored in the ZMODDAC1411 flash.
 * 128 bytes in size.
 */
typedef struct _CALIBECLYPSEADC {
    unsigned char   id; ///< 0xAD
    int             date; ///< unix time
    float           cal[2][2][2]; ///< [channel 0:1][low/high gain 0:1][0 multiplicative : 1 additive]
    unsigned char   nop[68]; ///< reserved
    char            lodg[22]; ///< BT log: Reference Board SN
    unsigned char   crc; ///< to generate: init 0 and -= 127 bytes; the checksum of the structure should be 0
} __attribute__((__packed__)) CALIBECLYPSEADC;

/****************************************************************************/

typedef enum ZMODADC1410_SC{
	SC1, SC2,
} ZmodADC1410_SC;

typedef enum ZOMODADC1410_COUPLING {
	DC_COUPLING,AC_COUPLING,
} ZmodADC1410_COUPLING;

typedef enum ZMODADC1410_GAIN {
	LOW_GAIN, HIGH_GAIN,
} ZmodADC1410_GAIN;

void ZMODADC1410_Init(uintptr_t iicAddress, uintptr_t flashAddress);
void ZMODADC1410_PrintConfigLog(int);
void ZMODADC1410_SetGain(ZmodADC1410_SC channel, ZmodADC1410_GAIN gain);
void ZMODADC1410_SetCoupling(ZmodADC1410_SC channel, ZmodADC1410_COUPLING coupling);
uint16_t ZMODADC1410_ChannelData(ZmodADC1410_SC channel, uint32_t data);
int16_t ZMODADC1410_SignedChannelData(uint8_t channel, uint32_t data);
void ZMODADC1410_SetCalibValues(uint8_t channel, uint8_t gain, float valG, float valA);


#endif /* SRC_ZMOD_ADC1410_ZMODADC1410_H_ */
