/*
 * interrupt_config.c
 *
 *  Created on: Mar 14, 2025
 *      Author: sebas
 */

/***************************** Include Files *******************************/
#include "../InterruptSystem/interruptSystem.h"

#include "xil_exception.h"
#include "xscugic.h"

#include "../includes/assert.h"
#include "../includes/log.h"

/************************** Constant Definitions **************************/
#define MAX_HANDLERS_CNT 10

/**************************** Type Definitions ******************************/

/************************** Function Prototypes *****************************/

/************************** Variable Definitions ***************************/
static INTC Intc;
int handlerIdx = 0;
Intr_Config* handlers[MAX_HANDLERS_CNT];

/****************************************************************************/

void IntrSystem_AddHandler(Intr_Config* handlerConfigPtr){
	if(handlerConfigPtr->IntrId != 0 && handlerIdx < MAX_HANDLERS_CNT)
		handlers[handlerIdx++] = handlerConfigPtr;
}

void IntrSystem_DisableIntr(u16 intrId){
	for(int i = 0;i < handlerIdx; i++)
	{
		if(handlers[i]->IntrId == intrId)
			XScuGic_Disconnect(&Intc, handlers[i]->IntrId);
	}
}

void IntrSystem_Disable(){
	for (int i = 0; i < handlerIdx; i++)
	{
		XScuGic_Disconnect(&Intc, handlers[i]->IntrId);
	}
}

int IntrSystem_Setup(){
	XScuGic_Config *IntcConfig;

	IntcConfig = XScuGic_LookupConfig(INTC_DEVICE_ID);
	if (NULL == IntcConfig) {
		return XST_FAILURE;
	}

	//Inicializar el controlador de interrupciones
	ASSERT_SUCCESS(
			XScuGic_CfgInitialize(&Intc, IntcConfig, IntcConfig->CpuBaseAddress),
			"Falla al inicializar las configuraciones de INTC");

	for (int i = 0; i < handlerIdx; i++) {
		XScuGic_SetPriorityTriggerType(&Intc, handlers[i]->IntrId, handlers[i]->Priority, 0x3);
		ASSERT_SUCCESS(
				XScuGic_Connect(&Intc, handlers[i]->IntrId, (Xil_InterruptHandler)handlers[i]->Handler, handlers[i]->CallBackRef),
				"Failed to connect interruption handler TAR_IntrHandler");
		XScuGic_Enable(&Intc, handlers[i]->IntrId);
	}

	//Interrupción de hardware
	Xil_ExceptionRegisterHandler(
			XIL_EXCEPTION_ID_INT,
			(Xil_ExceptionHandler)INTC_HANDLER,
			(void *)&Intc);

	Xil_ExceptionEnable();

	return XST_SUCCESS;
}


