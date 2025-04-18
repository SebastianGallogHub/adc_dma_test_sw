/*
 * interrupt_config.h
 *
 *  Created on: Mar 14, 2025
 *      Author: sebas
 */

#ifndef SRC_INTERRUPTSYSTEM_INTERRUPTSYSTEM_H_
#define SRC_INTERRUPTSYSTEM_INTERRUPTSYSTEM_H_

#include "xparameters_ps.h"
#include "xscugic.h"
#include "xil_types.h"

#define INTC			XScuGic
#define INTC_HANDLER	XScuGic_InterruptHandler

#define INTC_DEVICE_ID      XPAR_SCUGIC_SINGLE_DEVICE_ID

typedef struct InterruptionHandlerConfig
{
	u16 IntrId;
	Xil_ExceptionHandler Handler;
	void* CallBackRef;
	u8 Priority;
}Intr_Config;

void IntrSystem_AddHandler(Intr_Config* handlerConfig);
int IntrSystem_Setup();
void IntrSystem_DisableIntr(u16 intrId);
void IntrSystem_Disable();

#endif /* SRC_INTERRUPTSYSTEM_INTERRUPTSYSTEM_H_ */
