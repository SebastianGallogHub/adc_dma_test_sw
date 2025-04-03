
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "xparameters.h"
#include "xil_printf.h"
#include "xil_util.h"
#include "xdebug.h"
#include "xtime_l.h"

#include "zmodadc1410.h"
#include "tar_hal.h"
#include "assert.h"
#include "axitar.h"
#include "axitar_axidma.h"
#include "interruptSystem.h"
#include "log.h"


//-----------------------------------------------------------------------------
//Funciones
void PrintRxData();

//XTime tStart, tFinish, tElapsed; //todo ??
//#define GetElapsed_ns  ((tFinish-tStart)*3)
//#define GetElapsed_us  GetElapsed_ns/1000
//#define GetElapsed_ms  GetElapsed_ns/1000000

//-----------------------------------------------------------------------------

#define MAIN
#ifdef MAIN
int main()
{
	int Status;	char respuesta = 0;
	int i = 0;
	u16 h1_low, h1_high, h2_low, h2_high;
	float resolution = 3.21;

//	LOG_CLEAR_SCREEN;

	LOG(0, "--------------------- INICIO MAIN -----------------------");
	LOG(1, "PRUEBA LÓGICA DE DETECCIÓN DE PULSO");

	AXI_DMA_Init();

	ZMODADC1410_Init();


	h1_low = 1000/resolution;//0x3fff;
	h1_high = 3000/resolution;
	h2_low = 0x3fff;
	h2_high = 0x3fff;

	AXI_TAR_Init();

	LOG(2, "Canal 1, histéresis (%d ; %d)", h1_low, h1_high);
	AXI_TAR_SetHysteresis(0, h1_low, h1_high);

	LOG(2, "Canal 2, histéresis (%d ; %d)", h2_low, h2_high);
	AXI_TAR_SetHysteresis(1, h2_low, h2_high);

	do
	{
		i ++;
		LOG(0, "--------------------- (%d) -----------------------", i);
		ASSERT_SUCCESS(
				SetupIntrSystem(),
				"Fallo al inicializar el sistema de interrupciones");

		ASSERT_SUCCESS(
				AXI_DMA_SetupRx(AXI_DMA_NUMBER_OF_TRANSFERS, TAR_DMA_TRANSFER_LEN),
				"Fallo al inicializar el DMA.");

		LOG(1, "----- Inicio adquisición");

		TAR_Start();
//		XTime_GetTime(&tStart);

		Status = Xil_WaitForEventSet(1000000U, 1, &Error);
		if (Status == XST_SUCCESS) {
			LOG(0, "--> Receive error %d", Status);
			goto goOut;
		}

		// Espero hasta que se den las transacciones
		while(axiDmaTransferCount < AXI_DMA_NUMBER_OF_TRANSFERS){}

		DisableIntrSystem();

//		XTime_GetTime(&tFinish);
		TAR_StopAll();

		AXI_DMA_Reset();

		// Imprimir todos los valores recibidos junto a su dirección
		PrintRxData();

		xil_printf("¿Continuar?"); scanf(" %c", &respuesta);

	}while(respuesta=='s' || respuesta=='S');

	LOG(0, "--------------------- FIN MAIN -----------------------");
	return 0;

goOut:
	TAR_StopAll();
	AXI_DMA_Reset();
	LOG(0, "--------------------- FIN MAIN -----------------------");
	return XST_FAILURE;
}


void PrintRxData()
{
	u64 *buffer  = (u64*)AXI_DMA_RX_BUFFER_BASE;
	LOG_LINE;LOG_LINE;
//	LOG(1, "Datos recibidos (%d ms)", GetElapsed_ms);
//	LOG(2, "--------------------------------------");
	LOG(4, "PULSOS");
	for (u32 i = 0; i<= axiDmaTransferCount; i++){
		LOG(2, "%3d)\t0x%x\t\t[0x%x]", i, buffer[i], &buffer[i]);
	}
	LOG(2, "--------------------------------------");
	LOG(2, "Interrupciones recibidas por DMA: %d", axiDmaIntCount);
	LOG(2, "Transferencias recibidas por DMA: %d", axiDmaTransferCount);
//	LOG(2, "Transferencias lanzadas por TAR: %d", axiTarTransferCount);

	axiDmaIntCount = 0;
	axiDmaTransferCount = 0;
//	axiTarTransferCount = 0;
}
#endif // MAIN



















