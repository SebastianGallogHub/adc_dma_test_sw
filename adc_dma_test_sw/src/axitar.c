/*
 * tar_config.c
 *
 *  Created on: Mar 14, 2025
 *      Author: sebas
 */

/***************************** Include Files *******************************/

#include "axitar.h"

#include "xil_io.h"
#include "AXI_TAR.h"

#include "interruptSystem.h"
#include "log.h"

/************************** Constant Definitions **************************/

/**************************** Type Definitions ******************************/

/************************** Function Prototypes *****************************/

void TAR_master_IntrHandler(void * Callback);

/************************** Variable Definitions ***************************/

u32 axiTarTransferCount = 0;
Intr_Config tarIntrConfig =	{
		TAR_DR_INTR_ID,
		(Xil_ExceptionHandler)TAR_master_IntrHandler,
		(void *)TAR_BASE,
		0xA0
};
/****************************************************************************/

void AXI_TAR_SetHysteresis(int channel, u16 low, u16 high) {
	u32 histeresis = 0;
	histeresis = (high << 16) & low;
	if (channel == 0){
		AXI_TAR_mWriteReg(TAR_BASE, TAR_CH1_HIST_OFF, histeresis);
	}else{
		AXI_TAR_mWriteReg(TAR_BASE, TAR_CH2_HIST_OFF, histeresis);
	}
}

void AXI_TAR_Init() {
	LOG(1, "TAR_Init");

	TAR_StopAll();

	//Espero que se registre el valor de stop
	while(AXI_TAR_mReadReg(TAR_BASE,TAR_CONFIG_OFF));
}

/****************************************************************************/

void AXI_TAR_master_Init(u32 cuenta) {
	LOG(1, "TAR_Init");

	TAR_StopAll();

	//Espero que se registre el valor de stop
	while(AXI_TAR_mReadReg(TAR_BASE,TAR_CONFIG_OFF));

	AXI_TAR_mWriteReg(TAR_BASE, master_COUNT_CFG_OFF, cuenta);

	if (cuenta/100 > 1000) {
		LOG(2, "TAR configurado para interrumpir cada %d ciclos (%d ms)", cuenta, cuenta/100000);
	} else {
		LOG(2, "TAR configurado para interrumpir cada %d ciclos (%d us)", cuenta, cuenta/100);
	}

	AddIntrHandler(&tarIntrConfig);
	return;
}

void TAR_master_IntrHandler(void * Callback) {
	if(axiTarTransferCount <= UINT32_MAX)
	{
		axiTarTransferCount ++;
//		XTime_GetTime(&tFinish);
//		LOG(2, "%d) %d ms ", tarTransferCount, GetElapsed_ms);
	}

	return;
}

