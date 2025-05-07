/*
 * zmod.c
 *
 *  Created on: Mar 14, 2025
 *      Author: sebas
 */

/***************************** Include Files *******************************/
#include "../ZMOD/zmod.h"

#include "xil_io.h"

/************************** Constant Definitions **************************/

/**************************** Type Definitions ******************************/

/************************** Function Prototypes *****************************/

/************************** Variable Definitions ***************************/

/****************************************************************************/

void ZMOD_WriteRegFld(uint8_t regAddr, uint8_t lsbBit, uint8_t noBits, uint32_t value)
{
	uint32_t regMask = ((1 << noBits) - 1) << lsbBit;
	uint32_t regValue = Xil_In32(ZMOD_BASE_ADDR + regAddr);

	// align value to bit lsb_bit
	value <<= lsbBit;

	// mask out any bits outside specified field
	value &= regMask;

	// mask out bits corresponding to the specified field
	regValue &= ~regMask;

	// set the values for the field bits
	regValue |= value;

	Xil_Out32(ZMOD_BASE_ADDR + regAddr, regValue);
}

int32_t ZMOD_ToSigned(uint32_t value, uint8_t noBits)
{
	// align value to bit 31 (left justify), to preserve sign
	value <<= 32 - noBits;

	int32_t sValue = (int32_t)value;

	// align value to bit 0 (right justify)
	sValue >>= (32 - noBits);

	return sValue;
}
