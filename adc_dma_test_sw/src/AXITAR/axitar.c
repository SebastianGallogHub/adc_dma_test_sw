/*
 * tar_config.c
 *
 *  Created on: Mar 14, 2025
 *      Author: sebas
 */

/***************************** Include Files *******************************/
#include "../AXITAR/axitar.h"

#include <sleep.h>
#include "xil_io.h"
#include "AXI_TAR.h"

#include "../includes/log.h"
#include "../SD_CARD/sd_card.h"
#include "../AXITAR/axitar_axidma.h"
#include "../ZMOD_ADC1410/zmodadc1410.h"
#include "../InterruptSystem/interruptSystem.h"

/************************** Constant Definitions **************************/

/**************************** Type Definitions ******************************/

/************************** Function Prototypes *****************************/
void AXITAR_master_test_IntrHandler(void * Callback);

/************************** Variable Definitions ***************************/
u32 axiTarTransferCount = 0;
Intr_Config tarMasterTestIntrConfig ={
		AXITAR_DR_INTR_ID,
		(Xil_ExceptionHandler)AXITAR_master_test_IntrHandler,
		(void *)AXITAR_BASE,
		0xA0
};

/****************************************************************************/
void AXITAR_Init() {
	LOG(1, "AXITAR_Init");

	AXITAR_StopAll_();

	//Espero que se registre el valor de stop
	while(AXI_TAR_mReadReg(AXITAR_BASE, AXITAR_CONFIG_OFF));

	SD_Init();
	AXIDMA_Init();
	ZMODADC1410_Init();
}

void AXITAR_Start(){
	AXIDMA_SetupRx(
		SD_WORDS_PER_SECTOR(AXITAR_AXIDMA_TRANSFER_LEN) * 2,  	// Cantidad de pulsos a recibir en el buffer
		AXITAR_AXIDMA_TRANSFER_LEN,								// Longitud de 1 pulso en bytes
		SD_WORDS_PER_SECTOR(AXITAR_AXIDMA_TRANSFER_LEN),		// Coalescencia
		(AXIDMA_ProcessBufferDelegate)SD_WriteNextSector);		// Handler para procesar el buffer

	LOG(1, "----- Inicio adquisición -----");
	usleep(4000);

	SD_ResetRB();
	AXITAR_Start_();
}

void AXITAR_Stop(){
	AXITAR_StopAll_();
	AXIDMA_Reset();
	usleep(4000);
	SD_ResetRB();
}

void AXITAR_DisableChannel(int channel){
	AXITAR_SetHysteresis(channel, AXITAR_DISABLE_CH_MASK);
}

void AXITAR_SetHysteresis(int channel, u32 hist) {
	if (channel == 0)
		AXI_TAR_mWriteReg(AXITAR_BASE, AXITAR_CH0_HIST_OFF, hist & AXITAR_DISABLE_CH_MASK);
	else
		AXI_TAR_mWriteReg(AXITAR_BASE, AXITAR_CH1_HIST_OFF, hist & AXITAR_DISABLE_CH_MASK);
}

/****************************************************************************/

void AXITAR_master_test_Init(u32 cuenta){
	LOG(1, "TAR_Init");

	AXITAR_StopAll_();

	//Espero que se registre el valor de stop
	while(AXI_TAR_mReadReg(AXITAR_BASE,AXITAR_CONFIG_OFF));

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

