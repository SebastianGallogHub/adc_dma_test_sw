/***************************************************************
 * Nombre del Proyecto : Registrador de Amplitud y Tiempo (TAR)
 * Archivo             : interruptSystem.h
 * Descripción         : Definiciones de constantes y funciones para el
 * 						 el registro y setups de handlers de interrupciones.
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
