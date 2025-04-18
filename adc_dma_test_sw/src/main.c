/*
 * main.c
 *
 *  Created on: Sep 5, 2024
 *      Author: sebas
 */

/***************************** Include Files *******************************/
#include <unistd.h>

#include "xparameters.h"
#include "xil_printf.h"

#include "UART/uart.h"
#include "includes/log.h"
#include "includes/assert.h"
#include "mefs/mefSendDataAsync.h"
#include "mefs/mefCaptureCommand.h"
#include "InterruptSystem/interruptSystem.h"

/************************** Constant Definitions **************************/

/**************************** Type Definitions ******************************/

/************************** Function Prototypes *****************************/

/************************** Variable Definitions ***************************/

/****************************************************************************/

int main(){
	// Como manipula el puerto serie hay que inicializarlo
	// antes de empezar a mandar al terminal
	UART_Init();

	LOG(0, "------------------------------------------ INICIO MAIN --------------------------------------------");
	usleep(2000);

	mefCaptureCommand_Init();

	// Se configura el sistema de interrupciones con los handlers
	// registrados hasta este punto
	ASSERT_SUCCESS(
			IntrSystem_Setup(),
			"Fallo al inicializar el sistema de interrupciones");

	// Se inicializa la recepción siempre después de configurar
	// el sistema de interrupciones
	UART_SetupRx();

	while(1){
		mefSendDataAsync();

		mefCaptureCommand();
	}

	return 0;
}




