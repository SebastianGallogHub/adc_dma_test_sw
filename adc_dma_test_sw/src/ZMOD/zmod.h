/*
 * zmod.h
 *
 *  Created on: Mar 14, 2025
 *      Author: sebas
 */

#ifndef SRC_ZMOD_ZMOD_H_
#define SRC_ZMOD_ZMOD_H_

#include "xparameters.h"
#include "xil_types.h"

#define ZMOD_BASE_ADDR  	XPAR_AXI_ZMODADC1410_0_S00_AXI_BASEADDR
#define ZMOD_REG_ADDR_CR 	0x00	///< CR					register address
#define ZMOD_REGFLD_CR_RST 	ZMOD_REG_ADDR_CR, 31, 1	///< RST 			field of CR register

void ZMOD_WriteRegFld(uint8_t regAddr, uint8_t lsbBit, uint8_t noBits, uint32_t value);
int32_t ZMOD_ToSigned(uint32_t value, uint8_t noBits);

#endif /* SRC_ZMOD_ZMOD_H_ */
