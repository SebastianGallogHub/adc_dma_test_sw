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
//	LOG(1, "AXITAR_Init");

	AXITAR_StopAll_();

	//Espero que se registre el valor de stop
	while(AXI_TAR_mReadReg(AXITAR_BASE, AXITAR_CONFIG_OFF));

	AXIDMA_Init();
	ZMODADC1410_Init();

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

void AXITAR_Start(){
	AXIDMA_SetupRx(
		SD_WORDS_PER_SECTOR(AXITAR_AXIDMA_TRANSFER_LEN) * 2,  	// Cantidad de pulsos a recibir en el buffer
		AXITAR_AXIDMA_TRANSFER_LEN,								// Longitud de 1 pulso en bytes
		SD_WORDS_PER_SECTOR(AXITAR_AXIDMA_TRANSFER_LEN),		// Coalescencia
		(AXIDMA_ProcessBufferDelegate)SD_WriteNextSector);		// Handler para procesar el buffer

//	LOG(1, "----- Inicio adquisición -----");

	AXITAR_Start_();
}

void AXITAR_Stop(){
	AXITAR_StopAll_();
	AXIDMA_Reset();
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
	}else{
		ch1_hist = hist;
		AXI_TAR_mWriteReg(AXITAR_BASE, AXITAR_CH1_HIST_OFF, hist & AXITAR_DISABLE_CH_MASK);
	}
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

