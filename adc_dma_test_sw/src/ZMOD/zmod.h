/***************************************************************
 * Nombre del Proyecto : Registrador de Amplitud y Tiempo (TAR)
 * Archivo             : zmod.h
 * Descripción         : Definiciones de constantes y funciones para comunicación y
 * 						 configuración de un módulo ZMOD genérico.
 * Autor               : Sebastián Nahuel Gallo
 * Fecha de creación   : 14/03/2024
 * Fecha de modificación: 11/06/2025
 * Versión             : v1.0
 *
 * Institución         : Universidad Nacional de Rosario (UNR)
 * Carrera             : Ingeniería Electrónica
 *
 * Derechos reservados:
 * Este código ha sido desarrollado en el marco del Proyecto Final de Ingeniería
 * por Sebastián Nahuel Gallo. Su uso está autorizado únicamente por la
 * Comisión Nacional de Energía Atómica (CNEA) con fines internos.
 * Queda prohibida su reproducción, modificación o distribución sin
 * autorización expresa por escrito del autor.
 ***************************************************************/

#ifndef SRC_ZMOD_ZMOD_H_
#define SRC_ZMOD_ZMOD_H_

#include "xparameters.h"
#include "xil_types.h"

#define ZMOD_BASE_ADDR  	XPAR_AXI_ZMODADC1410_0_S00_AXI_BASEADDR
#define ZMOD_REG_ADDR_CR 	0x00	///< CR					register address
#define ZMOD_REGFLD_CR_RST 	ZMOD_REG_ADDR_CR, 31, 1	///< RST 			field of CR register

/**
 * Error codes
 */
#define ERR_SUCCESS 	0 	///< Success
#define ERR_FAIL		-1	///< General fail
#define ERR_CALIB_CRC	-2	///< CRC check fail
#define ERR_CALIB_ID	-3	///< Calib ID check fail

extern uint8_t *ZMOD_Calib; ///< pointer to calibration data

void ZMOD_InitFlash(uintptr_t iicAddress, uintptr_t flashAddress);
void ZMOD_DeinitFlash();

void ZMOD_WriteRegFld(uint8_t regAddr, uint8_t lsbBit, uint8_t noBits, uint32_t value);
uint32_t ZMOD_ReadRegFld(uint8_t regAddr, uint8_t lsbBit, uint8_t noBits);
int32_t ZMOD_ToSigned(uint32_t value, uint8_t noBits);

int ZMOD_InitCalib(uint32_t calibSize, uint8_t calibID, uint32_t userCalibAddr, uint32_t factCalibAddr);
int ZMOD_ReadUserCalib();
void ZMOD_WriteUserCalib();
int ZMOD_RestoreFactoryCalib();

#endif /* SRC_ZMOD_ZMOD_H_ */
