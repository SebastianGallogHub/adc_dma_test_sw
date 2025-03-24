/*
 * tar_config.c
 *
 *  Created on: Mar 14, 2025
 *      Author: sebas
 */

#include "axitar.h"

#include "xil_io.h"
#include "AXI_TAR.h"

#include "interruptSystem.h"
#include "log.h"

void TAR_IntrHandler(void * Callback);

u32 tarTransferCount = 0;
Intr_Config tarIntrConfig =
	{TAR_DR_INTR_ID, (Xil_ExceptionHandler)TAR_IntrHandler, (void *)TAR_BASE};

void TAR_Init(u32 cuenta)
{
	LOG(1, "TAR_Init");

	TAR_StopAll();
	do{/*Espero a que se registre el valor de stop*/}
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
void TAR_IntrHandler(void * Callback)
{
	if(tarTransferCount <= UINT32_MAX)
	{
		tarTransferCount ++;
//		XTime_GetTime(&tFinish);
//		LOG(2, "%d) %d ms ", tarTransferCount, GetElapsed_ms);
	}

	return;
}

