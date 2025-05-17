/*
 * zmod.c
 *
 *  Created on: Mar 14, 2025
 *      Author: sebas
 */

/***************************** Include Files *******************************/
#include "../ZMOD/zmod.h"

#include <stdlib.h>
#include "xil_io.h"
#include "flash.h"

/************************** Constant Definitions **************************/

/**************************** Type Definitions ******************************/

/************************** Function Prototypes *****************************/

/************************** Variable Definitions ***************************/
uintptr_t 	flashAddr; ///< Flash  base address
uint8_t 	*ZMOD_Calib; ///< pointer to ZMOD_Calibration data
uint32_t 	calibSize; ///< calibration size
uint16_t 	userCalibAddr; ///< address of user calibration area
uint16_t 	factCalibAddr;///< address of factory calibration area
uint8_t 	calibID; ///< calibration ID

/****************************************************************************/

void ZMOD_InitFlash(uintptr_t iicAddress, uintptr_t flashAddress){
	flashAddr = fnInitFlash(iicAddress, flashAddress);
	ZMOD_Calib = 0;
}

void ZMOD_DeinitFlash(){
	fnDestroyFlash(flashAddr);
	if(ZMOD_Calib)
	{
		free(ZMOD_Calib);
	}
}

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

uint32_t ZMOD_ReadRegFld(uint8_t regAddr, uint8_t lsbBit, uint8_t noBits){
	uint32_t reg_mask = ((1 << noBits) - 1) << lsbBit;
	uint32_t value = Xil_In32(ZMOD_BASE_ADDR + regAddr);

	// mask out bits corresponding to the specified field
	value &= reg_mask;

	// align value to bit 0 (right justify)
	value >>= lsbBit;

	return value;
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

/****************************************************************************/

/**
 * Initialize the calibration related data.
 * It allocates the ZMOD_Calib area as an array of bytes having the desired length.
 * It also stores the calibration ID and the user and factory calibration flash addresses.
 * In the end it reads the user calibration values by calling the Zmod specific
 *  (virtual) readUserCalib function.
 * @param calibSize the size of calibration data to be retrieved from Flash.
 * @param calibID the ID of the calibration data.
 * @param userCalibAddr the flash address of user calibration area.
 * @param factCalibAddr the flash address of factory calibration area.
 * @return the status: ERR_SUCCESS for success, ERR_FAIL for allocation error.
 */
int ZMOD_InitCalib(uint32_t calibSize_, uint8_t calibID_, uint32_t userCalibAddr_, uint32_t factCalibAddr_)
{
	ZMOD_Calib = (uint8_t *)malloc(calibSize_);
	calibSize = calibSize_;
	calibID = calibID_;
	userCalibAddr = userCalibAddr_;
	factCalibAddr = factCalibAddr_;
	ZMOD_ReadUserCalib();
	return ((ZMOD_Calib != 0) ? ERR_SUCCESS: ERR_FAIL);
}

/**
 * Computes a one byte checksum of an array of bytes.
 * @param pData a pointer to an array of bytes
 * @param len the length of the array
 */
uint8_t ZMOD_ComputeCRC(uint8_t *pData, uint32_t len)
{
	uint8_t s = 0;
	uint32_t i;
	for(i = 0; i < len; i++)
	{
		s-= pData[i];
	}
	return s;
}

/**
 * Writes the calibration data from the area pointed by calib member into the user calibration area.
 * At the ZMOD class level, the calib area is regarded just as an array of bytes.
 * The area pointed by calib must be already allocated by calling initCalib
 * (normally called from the child class).
 * Before writing it fills the first byte of the calib area with the calibration ID and
 * the last byte of the calib area with the checksum of all but the last byte of the calib area.
 *
 */
void ZMOD_WriteUserCalib()
{
	uint8_t crc;
	// fill the calib ID byte on the first byte of calib
	ZMOD_Calib[0] = calibID;
	// compute the checksum on all but the last byte of the calib area
	crc = ZMOD_ComputeCRC(ZMOD_Calib, calibSize - 1);
	// fill the checksum byte on the last byte of calib area
	ZMOD_Calib[calibSize] = crc;

	fnWriteFlash(flashAddr, userCalibAddr, ZMOD_Calib, calibSize);
}


/**
 * Reads the calibration data into the bytes array pointed by ZMOD_Calib class member.
 * At the ZMOD class level, the ZMOD_Calib area is regarded just as an array of bytes.
 * The area pointed by ZMOD_Calib must be already allocated by calling initCalib
 *  (normally called from the child class).
 * This function expects and verifies that the first byte of the data retrieved
 * from flash is the ZMOD_Calib id, while the last byte is the checksum.
 * @return the status: ERR_SUCCESS for success, ERR_CALIB_ID for ZMOD_Calib ID error,
 *  ERR_CALIB_CRC for CRC error.
 */
int ZMOD_ReadUserCalib()
{
	int status;
	uint8_t crc;
	// read the user calibration data as an array of bytes
	status = fnReadFlash(flashAddr, userCalibAddr, ZMOD_Calib, calibSize);
	if(status == ERR_SUCCESS)
	{
		// check the expected ZMOD_Calib ID against the first byte of the ZMOD_Calib area
		if(ZMOD_Calib[0] != calibID)
		{
			// ZMOD_Calib ID error
			status = ERR_CALIB_ID;
		}
		else
		{
			// compute the CRC and check against the last byte of the ZMOD_Calib area
			crc = ZMOD_ComputeCRC(ZMOD_Calib, calibSize - 1);
			if(crc != ZMOD_Calib[calibSize-1])
			{
				// CRC error
				status = ERR_CALIB_CRC;
			}
		}
	}
	return status;
}

/**
 * Restores the factory calibration data by reading it from factory calibration area into the
 *  area pointed by calib and writing it into the user calibration area.
 *
 */
int ZMOD_RestoreFactoryCalib()
{
	// reads the factory calibration into calib as an array of bytes
	int status;
	// read the user calibration data as an array of bytes
	status = fnReadFlash(flashAddr, factCalibAddr, ZMOD_Calib, calibSize);
	if(status == ERR_SUCCESS)
	{
		// writes the calibration data from the area pointed by calib member into the user calibration area.
		ZMOD_WriteUserCalib();

		// this will generate calibration coefficients
		ZMOD_ReadUserCalib();
	}
	return status;
}
