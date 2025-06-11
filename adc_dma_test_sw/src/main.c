/***************************************************************
 * Nombre del Proyecto : Registrador de Amplitud y Tiempo (TAR)
 * Archivo             : main.c
 * Descripción         : Archivo principal del sistema. Contiene la función
 *                       main y la inicialización de periféricos.
 * Autor               : Sebastián Nahuel Gallo
 * Fecha de creación   : 05/09/2024
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


/***************************** Include Files *******************************/
#include "mefs/mefTAR.h"
#include "includes/assert.h"

/************************** Constant Definitions **************************/

/**************************** Type Definitions ******************************/

/************************** Function Prototypes *****************************/

/************************** Variable Definitions ***************************/

/****************************************************************************/

int main(){
	ASSERT_SUCCESS(mefTAR_Init(), "Fallo al inicializar TAR");

	while(1){
		if (mefTAR())
			return 1;
	}
}




