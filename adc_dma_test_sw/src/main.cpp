
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "xparameters.h"
#include "xil_printf.h"
#include "xdebug.h"
#include "xtime_l.h"
#include "xuartps.h" //todo

#include "zmodadc1410.h"
#include "interrupt_config.h"
#include "dma_config.h"
#include "tar_config.h"
#include "tar_hal.h"
#include "assert.h"
#include "log.h"


//-----------------------------------------------------------------------------
//Funciones
void PrintRxData();

XTime tStart, tFinish, tElapsed; //todo ??
#define GetElapsed_ns  ((tFinish-tStart)*3)
#define GetElapsed_us  GetElapsed_ns/1000
#define GetElapsed_ms  GetElapsed_ns/1000000

//-----------------------------------------------------------------------------
int main()
{
	int Status;
	char respuesta = 0;
	int i = 0;

//	LOG_CLEAR_SCREEN;

	LOG(0, "--------------------- INICIO MAIN -----------------------");
	LOG(1, "PRUEBA SOLO DE LAS INTERRUPCIONES DE MASTER_TEST");

	DMA_Init();

	ZMODADC1410_Init();

	TAR_Init(TAR_TRANSFER_COUNT);

	do
	{
		i ++;
		LOG(0, "--------------------- (%d) -----------------------", i);
		ASSERT_SUCCESS(
				SetupIntrSystem(),
				"Fallo al inicializar el sistema de interrupciones");

		ASSERT_SUCCESS(
				DMA_RxInit(DMA_NUMBER_OF_TRANSFERS, TAR_DMA_TRANSFER_LEN),
				"Fallo al inicializar el DMA.");

		LOG(1, "----- Inicio interrupciones");
		TAR_Start_master_test();
		XTime_GetTime(&tStart);

		Status = Xil_WaitForEventSet(1000000U, 1, &Error);
		if (Status == XST_SUCCESS) {
			LOG(0, "--> Receive error %d", Status);
			goto goOut;
		}

		// Espero hasta que se den las transacciones
		while(dmaTransferCount < DMA_NUMBER_OF_TRANSFERS){}

		DisableIntrSystem();

		XTime_GetTime(&tFinish);

		TAR_StopAll();

		DMA_Reset();

		//Imprimir todos los valores recibidos junto a su dirección
		PrintRxData();
		dmaIntCount = 0;
		dmaTransferCount = 0;
		tarTransferCount = 0;
		printf("¿Continuar?");
		scanf(" %c", &respuesta);
	}while(respuesta=='s' || respuesta=='S');

	LOG(0, "--------------------- FIN MAIN -----------------------");
	return 0;

goOut:
	TAR_StopAll();
	DMA_Reset();
	LOG(0, "--------------------- FIN MAIN -----------------------");
	return XST_FAILURE;
}

void PrintRxData()
{
	u32* buffer  = (u32*)DMA_RX_BUFFER_BASE;
	LOG_LINE;LOG_LINE;
	LOG(1, "Datos recibidos (%d ms)", GetElapsed_ms);

	LOG(2, "--------------------------------------");
	LOG(2, "\t\t\tCH1 \t|\tCH2", tarTransferCount);
	for (u32 i = 0; i<= dmaTransferCount; i+=1){
		LOG(2, "%d)\t%d  \t|\t%d\t\t[0x%08x]", i,  (u16)((buffer[i] >> 16) & 0xffff), (u16)(buffer[i] & 0xffFF), &buffer[i]);
	}
	LOG(2, "--------------------------------------");
	LOG(2, "Interrupciones recibidas por DMA: %d", dmaIntCount);
	LOG(2, "Transferencias recibidas por DMA: %d", dmaTransferCount);
	LOG(2, "Transferencias lanzadas por TAR: %d", tarTransferCount);

	return;
}




















