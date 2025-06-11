/***************************************************************
 * Nombre del Proyecto : Registrador de Amplitud y Tiempo (TAR)
 * Archivo             : axitar.c
 * Descripción         : Archivo de implementación para la inicialización y
 * 						 control del IP AXI_TAR
 * Autor               : Sebastián Nahuel Gallo
 * Fecha de creación   : 14/03/2025
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

/***************************** Include Files *******************************/
#include "../AXITAR/axitar.h"

#include <sleep.h>
#include "xil_io.h"
#include "xparameters.h"
#include "AXI_TAR.h" // Librería generada por Vivado

#include "../includes/log.h"
#include "../includes/assert.h"
#include "../SD_CARD/sd_card.h"
#include "../AXITAR/axitar_axidma.h"
#include "../ZMOD_ADC1410/zmodadc1410.h"
#include "../InterruptSystem/interruptSystem.h"

/************************** Constant Definitions **************************/
// Para configurar el ZMOD y su calibración
#define IIC_BASE_ADDR   	XPAR_PS7_I2C_1_BASEADDR
#define FLASH_ADDR_ADC   	0x30

// Para almacenar los datos en la SD hasta su envío a PC
#define SECTORS_TO_WRITE 	16

/**************************** Type Definitions ******************************/

/************************** Function Prototypes *****************************/
void AXITAR_master_test_IntrHandler(void * Callback);

/************************** Variable Definitions ***************************/
u32 ch0_hist = 0;
u32 ch1_hist = 0;
u32 axiTarTransferCount = 0;
Intr_Config tarMasterTestIntrConfig ={
		AXITAR_DR_INTR_ID,
		(Xil_ExceptionHandler)AXITAR_master_test_IntrHandler,
		(void *)AXITAR_BASE,
		0xA0
};

/****************************************************************************/
void AXITAR_Init() {
	AXITAR_StopAll_();

	//Espero que se registre el valor de stop
	while(AXI_TAR_mReadReg(AXITAR_BASE, AXITAR_CONFIG_OFF));

	AXIDMA_Init();
	ZMODADC1410_Init(IIC_BASE_ADDR, FLASH_ADDR_ADC);

	AXITAR_DisableChannel(0);
	AXITAR_DisableChannel(1);
}

void AXITAR_PrintConfigLog(int l){
	ZMODADC1410_PrintConfigLog(l);

	LOG(l, "AXI_TAR config:");

	if(ch0_hist == AXITAR_DISABLE_CH_MASK)
		LOG(l+1, "CHA: DESHABILITADO");
	else
		LOG(l+1, "CHA: histéresis (%u ; %u)", AXITAR_LowHist(ch0_hist), AXITAR_HighHist(ch0_hist));

	if(ch1_hist == AXITAR_DISABLE_CH_MASK)
		LOG(l+1, "CHB: DESHABILITADO");
	else
		LOG(l+1, "CHB: histéresis (%u ; %u)", AXITAR_LowHist(ch1_hist), AXITAR_HighHist(ch1_hist));
}

int AXITAR_Start(){
	// (SD_WORDS_PER_SECTOR(AXITAR_AXIDMA_TRANSFER_LEN)) * SECTORS_TO_WRITE * n
	// Se configura que haya un equivalente a SECTORS_TO_WRITE segmentos de SD
	// Se configuran bd's de datos de que apuntan a un buffer de longitud de 1 registro (8 bytes)

	ASSERT_SUCCESS(AXIDMA_SetupRx(
		(SD_WORDS_PER_SECTOR(AXITAR_AXIDMA_TRANSFER_LEN)) * SECTORS_TO_WRITE * 48, 	// Tamaño total del buffer en cantidad de bd's (en palabras de 8 bytes)
		(SD_WORDS_PER_SECTOR(AXITAR_AXIDMA_TRANSFER_LEN)) * SECTORS_TO_WRITE,		// Tamaño de la porción del buffer que se procesa (en palabras de 8 bytes)
		AXITAR_AXIDMA_TRANSFER_LEN,													// Longitud de un registro de pulso en bytes
		(SD_WORDS_PER_SECTOR(AXITAR_AXIDMA_TRANSFER_LEN)) * 2), 					// Coalescencia, cada cuántos sectores recibidos
																					// va a hacer una interrupción
		"Fallo al inicializar AXI_DMA Rx");

	AXITAR_Start_();

	return 0;
}

void AXITAR_SaveDataAsync(){
	u32 buffer;

	if (AXIDMA_BufferSectorComplete(&buffer))
		SD_WriteSectors((unsigned char *)buffer, SECTORS_TO_WRITE);
}

void AXITAR_Stop(){
	AXITAR_StopAll_();
	AXIDMA_StopRxAsync();
	AXITAR_DisableChannel(0);
	AXITAR_DisableChannel(1);
}

void AXITAR_DisableChannel(int channel){
	AXITAR_SetHysteresis(channel, AXITAR_DISABLE_CH_MASK);
}

void AXITAR_SetHysteresis(int channel, u32 hist) {
	if (channel == 0){
		ch0_hist = hist;
		AXI_TAR_mWriteReg(AXITAR_BASE, AXITAR_CH0_HIST_OFF, hist & AXITAR_DISABLE_CH_MASK);
		while ((AXI_TAR_mReadReg(AXITAR_BASE, AXITAR_CH0_HIST_OFF) != (hist & AXITAR_DISABLE_CH_MASK)));
	}else{
		ch1_hist = hist;
		AXI_TAR_mWriteReg(AXITAR_BASE, AXITAR_CH1_HIST_OFF, hist & AXITAR_DISABLE_CH_MASK);
		while ((AXI_TAR_mReadReg(AXITAR_BASE, AXITAR_CH1_HIST_OFF) != (hist & AXITAR_DISABLE_CH_MASK)));
	}
}

/****************************************************************************/

void AXITAR_master_test_Init(u32 cuenta){
	LOG(1, "TAR_Init");

	AXITAR_StopAll_();

	//Espero que se registre el valor de stop
	while(AXI_TAR_mReadReg(AXITAR_BASE, AXITAR_CONFIG_OFF));

	AXI_TAR_mWriteReg(AXITAR_BASE, master_COUNT_CFG_OFF, cuenta);

	if (cuenta/100 > 1000) {
		LOG(2, "TAR configurado para interrumpir cada %d ciclos (%d ms)", cuenta, cuenta/100000);
	} else {
		LOG(2, "TAR configurado para interrumpir cada %d ciclos (%d us)", cuenta, cuenta/100);
	}

	IntrSystem_AddHandler(&tarMasterTestIntrConfig);
	return;
}

void AXITAR_master_test_IntrHandler(void * Callback){
	if(axiTarTransferCount <= UINT32_MAX)
		axiTarTransferCount ++;
	return;
}

