/***************************** Include Files *******************************/

#include "xparameters.h"

#include "xil_printf.h"

#include "InterruptSystem/interruptSystem.h"
#include "UART_DMA/xuartps_0.h"

/************************** Constant Definitions **************************/

/**************************** Type Definitions ******************************/

/************************** Function Prototypes *****************************/

/************************** Variable Definitions ***************************/

/****************************************************************************/

#ifdef MAIN_xuartps_intr_example
int main (void){

	UARTPS_0_Init();

	SetupIntrSystem();

	UARTPS_0_Test();

	return 0;
}
#endif // MAIN_xuartps_intr_example
