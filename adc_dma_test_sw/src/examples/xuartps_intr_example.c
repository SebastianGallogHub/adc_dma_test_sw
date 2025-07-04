/***************************** Include Files *******************************/
#ifdef MAIN_xuartps_intr_example
#include "xparameters.h"

#include "xil_printf.h"

#include "xuartps_0.h"
#include "interruptSystem.h"

/************************** Constant Definitions **************************/

/**************************** Type Definitions ******************************/

/************************** Function Prototypes *****************************/

/************************** Variable Definitions ***************************/

/****************************************************************************/


int main (void){

	UARTPS_0_Init();

	SetupIntrSystem();

	UARTPS_0_Test();

	return 0;
}
#endif // MAIN_xuartps_intr_example
