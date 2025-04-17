
/***************************** Include Files *******************************/
#define MAIN
#ifdef MAIN

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "xparameters.h"
#include "xil_printf.h"
#include "xil_util.h"
#include "xdebug.h"
#include "xtime_l.h"

#include "includes/log.h"
#include "includes/assert.h"
#include "AXI_TAR/axitar.h"
#include "AXI_TAR/axitar_axidma.h"
#include "SD_CARD/sd_card.h"
#include "UART_DMA/xuartps_0.h"
#include "ZMOD_ADC1410/zmodadc1410.h"
#include "InterruptSystem/interruptSystem.h"

/************************** Constant Definitions **************************/

#define ZMODADC1410_RESOLUTION 	3.21

#define WORDS_PER_SECTOR 		SD_WORDS_PER_SECTOR(AXI_TAR_DMA_TRANSFER_LEN)

/**************************** Type Definitions ******************************/

/************************** Function Prototypes *****************************/
void PrintRxData();

/************************** Variable Definitions ***************************/

u64 sector_rd_buffer[WORDS_PER_SECTOR] __attribute__ ((aligned (64)));

/****************************************************************************/

void mefDatos(){
	static int st = 0;

	switch (st){
	case 0:
		if(SD_SectorsToRead() > 1){
			st = 1;
		}

		break;

	case 1:
		if(SD_SectorsToRead() > 0) {
			if(UARTPS_0_DoneSendBuffer()){
				SD_ReadNextSector((unsigned char*)sector_rd_buffer);
				UARTPS_0_SendBufferAsync((u32)sector_rd_buffer, SD_SECTOR_SIZE, AXI_TAR_DMA_TRANSFER_LEN);
			}
		}

		if(SD_SectorsToRead() <= 0){
			st = 2;
		}

		break;

	case 2:
		if(UARTPS_0_DoneSendBuffer()){
//			usleep(1000);
//			xil_printf("%%%%");
//			usleep(1000);
			st = 0;
//			res = 1;
		}

		break;

	default:
		st = 0;
		break;
	}
}

int main()
{
	u16 h0_low, h0_high, h1_low, h1_high;
	UART_COMMAND c;
	u16 p;

	UARTPS_0_Init();

	LOG(0, "--------------------- INICIO MAIN -----------------------");
	LOG(1, "PRUEBA LÓGICA DE DETECCIÓN DE PULSO + RINGBUFFER SD");
	usleep(2000);

	SD_Init();
	AXI_DMA_Init();
	AXI_TAR_Init();
	ZMODADC1410_Init();

	ASSERT_SUCCESS(
			SetupIntrSystem(),
			"Fallo al inicializar el sistema de interrupciones");

	UARTPS_0_StartRx();

	h0_low = 1000/ZMODADC1410_RESOLUTION;//0x3fff;
	h0_high = 3000/ZMODADC1410_RESOLUTION;
	LOG(2, "Canal 0, histéresis (%d ; %d)", h0_low, h0_high);
	AXI_TAR_SetHysteresis(0, h0_low, h0_high);
//	AXI_TAR_SetHysteresis(0, 0, h0_low);
//	AXI_TAR_SetHysteresis(0, 1, h0_high);

	h1_low = 0x3fff;//1000/ZMODADC1410_RESOLUTION;//0x3fff;
	h1_high = 0x3fff;//3000/ZMODADC1410_RESOLUTION;
	LOG(2, "Canal 1, histéresis (%d ; %d)", h1_low, h1_high);
	AXI_TAR_SetHysteresis(1, h1_low, h1_high);
//	AXI_TAR_SetHysteresis(1, 0, h1_low);
//	AXI_TAR_SetHysteresis(1, 1, h1_high);


	AXI_DMA_SetupRx(
				WORDS_PER_SECTOR * 2,  									// Cantidad de DATOS a recibir en el buffer
				AXI_TAR_DMA_TRANSFER_LEN,								// Longitud de 1 dato en bytes
				WORDS_PER_SECTOR,										// Coalescencia
				(AXI_DMA_ProcessBufferDelegate)SD_WriteNextSector);		// Handler para procesar el buffer



//	AXI_TAR_Start();


	while(1){
		// mefMedicion
		c = UART_0_GetCommand();

		if(c == CMD_START)
		{
			LOG(1, "----- Inicio adquisición -----");
			usleep(2000);
			AXI_TAR_Start();
		}

		if (axiDmaTransferCount >= SD_WORDS_PER_SECTOR(AXI_TAR_DMA_TRANSFER_LEN) * 2){
			AXI_TAR_StopAll();
			AXI_DMA_Reset();
		}

		if(c == CMD_STOP){
			AXI_TAR_StopAll();
			AXI_DMA_Reset();
		}

		mefDatos();
	}

	DisableIntrSystem();

//	LOG(0, "--------------------- FIN MAIN -----------------------");
	return 0;

}
#endif // MAIN



















