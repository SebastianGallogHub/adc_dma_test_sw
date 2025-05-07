/*
 * main.c
 *
 *  Created on: Sep 5, 2024
 *      Author: sebas
 */

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
		if (mefTAR(0))
			return 1;
	}
}




