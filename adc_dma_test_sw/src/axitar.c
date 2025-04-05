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

void AXI_TAR_master_test_IntrHandler(void * Callback);

/************************** Variable Definitions ***************************/

u32 axiTarTransferCount = 0;
Intr_Config tarMasterTestIntrConfig =	{
		AXI_TAR_DR_INTR_ID,
		(Xil_ExceptionHandler)AXI_TAR_master_test_IntrHandler,
		(void *)AXI_TAR_BASE,
		0xA0
};

/****************************************************************************/

void AXI_TAR_master_test_Init(u32 cuenta)
{
	LOG(1, "TAR_Init");

	AXI_TAR_StopAll();

	//Espero que se registre el valor de stop
	while(AXI_TAR_mReadReg(AXI_TAR_BASE,AXI_TAR_CONFIG_OFF));

	AXI_TAR_mWriteReg(AXI_TAR_BASE, master_COUNT_CFG_OFF, cuenta);

	if (cuenta/100 > 1000) {
		LOG(2, "TAR configurado para interrumpir cada %d ciclos (%d ms)", cuenta, cuenta/100000);
	} else {
		LOG(2, "TAR configurado para interrumpir cada %d ciclos (%d us)", cuenta, cuenta/100);
	}

	AddIntrHandler(&tarMasterTestIntrConfig);
	return;
}
void AXI_TAR_master_test_IntrHandler(void * Callback)
{
	if(axiTarTransferCount <= UINT32_MAX)
		axiTarTransferCount ++;
	return;
}

