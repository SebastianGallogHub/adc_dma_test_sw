
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

u32 sector_rd_buffer[WORDS_PER_SECTOR] __attribute__ ((aligned (64)));

/****************************************************************************/

int mefDatos(){
	static int st = 0;
	int res = 0;

	switch (st){
	case 0:
		if(SD_SectorsToRead() > 1){
			xil_printf("&");
			st = 1;
		}

		break;

	case 1:
		if(SD_SectorsToRead() > 0) {
			if(UARTPS_0_DoneSendBuffer()){
				SD_ReadNextSector((unsigned char*)sector_rd_buffer);
				UARTPS_0_SendBufferAsync((u32)sector_rd_buffer, SD_SECTOR_SIZE, sizeof(u32));
			}
		}

		if(SD_SectorsToRead() <= 0){
			st = 2;
		}

		break;

	case 2:
		if(UARTPS_0_DoneSendBuffer()){
			xil_printf("&");
			st = 0;
			res = 1;
		}

		break;

	default:
		st = 0;
		break;
	}

	return res;
}

int main()
{
	u16 h1_low, h1_high, h2_low, h2_high;

	LOG(0, "--------------------- INICIO MAIN -----------------------");
	LOG(1, "PRUEBA LÓGICA DE DETECCIÓN DE PULSO");

	UARTPS_0_Init();
	SD_Init();
	AXI_DMA_Init();
	AXI_TAR_Init();
	ZMODADC1410_Init();

	ASSERT_SUCCESS(
			SetupIntrSystem(),
			"Fallo al inicializar el sistema de interrupciones");

	UARTPS_0_StartRx();

	h1_low = 1000/ZMODADC1410_RESOLUTION;//0x3fff;
	h1_high = 3000/ZMODADC1410_RESOLUTION;
	LOG(2, "Canal 1, histéresis (%d ; %d)", h1_low, h1_high);
	AXI_TAR_SetHysteresis(0, h1_low, h1_high);

	h2_low = 1000/ZMODADC1410_RESOLUTION;//0x3fff;
	h2_high = 3000/ZMODADC1410_RESOLUTION;
	LOG(2, "Canal 2, histéresis (%d ; %d)", h2_low, h2_high);
	AXI_TAR_SetHysteresis(1, h2_low, h2_high);

//	do
//	{
//		i ++;
//		LOG(0, "--------------------- (%d) -----------------------", i);


//		ASSERT_SUCCESS(
//				AXI_DMA_SetupRx(AXI_DMA_NUMBER_OF_TRANSFERS, TAR_DMA_TRANSFER_LEN),
//				"Fallo al inicializar el DMA.");

		AXI_DMA_SetupRx(
					WORDS_PER_SECTOR * 2,  									// Cantidad de DATOS a recibir en el buffer
					AXI_TAR_DMA_TRANSFER_LEN,								// Longitud de 1 dato en bytes
					WORDS_PER_SECTOR,										// Coalescencia
					(AXI_DMA_ProcessBufferDelegate)SD_WriteNextSector);		// Handler para procesar el buffer

		LOG(1, "----- Inicio adquisición -----");

		AXI_TAR_Start();

//		Status = Xil_WaitForEventSet(1000000U, 1, &Error);
//		if (Status == XST_SUCCESS) {
//			LOG(0, "--> Receive error %d", Status);
//			goto goOut;
//		}

		// Espero hasta que se den las transacciones
//		while(axiDmaTransferCount < AXI_DMA_NUMBER_OF_TRANSFERS){}

		while(1){
			// mefMedicion
			if (axiDmaTransferCount >= SD_WORDS_PER_SECTOR(AXI_TAR_DMA_TRANSFER_LEN) * 2){
				AXI_TAR_StopAll();
				AXI_DMA_Reset();
			}

			if (mefDatos()){
				break;
			}
		}

		DisableIntrSystem();
//
//		AXI_TAR_StopAll();
//
//		AXI_DMA_Reset();

//		xil_printf("¿Continuar?"); scanf(" %c", &respuesta);

//	}while(respuesta=='s' || respuesta=='S');

	LOG(0, "--------------------- FIN MAIN -----------------------");
	return 0;

//goOut:
//	AXI_TAR_StopAll();
//	AXI_DMA_Reset();
//	LOG(0, "--------------------- FIN MAIN -----------------------");
//	return XST_FAILURE;
}

#define MSK_HEADER  0xFF00000000000000
#define MSK_TS		0x00FFFFFFFF000000
#define MSK_CH		0x0000000000C00000
#define MSK_VP		0x00000000003FFF00
#define MSK_FOOTER	0x00000000000000FF
#define MSK_ALL 	MSK_HEADER | MSK_TS | MSK_CH | MSK_VP | MSK_FOOTER
#define MSK_TS_OFF	24
#define MSK_CH_OFF	22
#define MSK_VP_OFF	8
#define GetFieldFromPulse(pulse, mask, offset) (pulse & mask) >> offset

void PrintRxData()
{
	u64 *buffer  = (u64*)AXI_DMA_RX_BUFFER_BASE;
	u32 ts = 0;
	u16 vp = 0;
	u8 ch = 0;

	LOG_LINE;LOG_LINE;

	LOG(3, ";\t\tch;\tts;\tvp(CAD);\tvp(mV);\tdir");
	for (u32 i = 0; i<= axiDmaTransferCount; i++){

		ts = GetFieldFromPulse(buffer[i], MSK_TS, MSK_TS_OFF);
		ch = GetFieldFromPulse(buffer[i], MSK_CH, MSK_CH_OFF);
		vp = GetFieldFromPulse(buffer[i], MSK_VP, MSK_VP_OFF);

		if(ch == 3)
			LOG(2, "%d;\t\t%d;\t0x%08x;\t%d;\t[0x%x];\t**", i, ch, ts, vp, &buffer[i]);
		else
			LOG(2, "%d;\t\t%d;\t0x%08x;\t%d;\t[0x%x]", i, ch, ts, vp, &buffer[i]);
	}
	LOG(2, "--------------------------------------");
	LOG(2, "Interrupciones recibidas por DMA: %d", axiDmaIntCount);
	LOG(2, "Transferencias recibidas por DMA: %d", axiDmaTransferCount);

	axiDmaIntCount = 0;
	axiDmaTransferCount = 0;
}
#endif // MAIN



















