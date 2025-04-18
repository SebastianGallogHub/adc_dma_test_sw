/*
 * tar_config.c
 *
 *  Created on: Mar 14, 2025
 *      Author: sebas
 */

/***************************** Include Files *******************************/

#include "../AXITAR/axitar.h"

#include "xil_io.h"
#include "AXI_TAR.h"

#include "../includes/log.h"

#include "../InterruptSystem/interruptSystem.h"

/************************** Constant Definitions **************************/

/**************************** Type Definitions ******************************/

/************************** Function Prototypes *****************************/

void AXI_TAR_master_test_IntrHandler(void * Callback);

/************************** Variable Definitions ***************************/

u32 axiTarTransferCount = 0;
Intr_Config tarMasterTestIntrConfig ={
		AXITAR_DR_INTR_ID,
		(Xil_ExceptionHandler)AXI_TAR_master_test_IntrHandler,
		(void *)AXITAR_BASE,
		0xA0
};

/****************************************************************************/

void AXITAR_DisableChannel(int channel){
	AXITAR_SetHysteresis(channel, ((u32)0x3FFF << 16) | 0x3FFF);
}

void AXITAR_SetHysteresis(int channel, u32 hist) {
	if (channel == 0)
		AXI_TAR_mWriteReg(AXITAR_BASE, AXITAR_CH1_HIST_OFF, hist);
	else
		AXI_TAR_mWriteReg(AXITAR_BASE, AXITAR_CH2_HIST_OFF, hist);
}

void AXITAR_Init() {
	LOG(1, "TAR_Init");

	AXITAR_StopAll();

	//Espero que se registre el valor de stop
	while(AXI_TAR_mReadReg(AXITAR_BASE, AXITAR_CONFIG_OFF));
}

/****************************************************************************/

void AXITAR_master_test_Init(u32 cuenta){
	LOG(1, "TAR_Init");

	AXITAR_StopAll();

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

void AXI_TAR_master_test_IntrHandler(void * Callback){
	if(axiTarTransferCount <= UINT32_MAX)
		axiTarTransferCount ++;
	return;
}

