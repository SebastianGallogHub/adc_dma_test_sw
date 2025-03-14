
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "xparameters.h"
#include "xil_exception.h"
#include "xil_printf.h"
#include "xdebug.h"
#include "xtime_l.h"
#include "xscugic.h"
#include "xuartps.h" //todo

#include "zmod.h"
#include "zmodadc1410.h"
#include "dma_config.h"
#include "tar_config.h"
#include "tar_hal.h"
#include "assert.h"
#include "log.h"

//-----------------------------------------------------------------------------
//Interupciones
#define INTC			XScuGic
#define INTC_HANDLER	XScuGic_InterruptHandler

#define INTC_DEVICE_ID      XPAR_SCUGIC_SINGLE_DEVICE_ID

//-----------------------------------------------------------------------------
//Funciones
void PrintRxData();

int SetupIntrSystem(INTC*, XAxiDma*, u16 DmaRxIntrId, u16 TARIntrId);
void DisableIntrSystem(INTC*, u16 DmaRxIntrId, u16 TARIntrId);

static INTC Intc;

XTime tStart, tFinish, tElapsed;
#define GetElapsed_ns  ((tFinish-tStart)*3)
#define GetElapsed_us  GetElapsed_ns/1000
#define GetElapsed_ms  GetElapsed_ns/1000000

//-----------------------------------------------------------------------------
int main()
{
	int Status;
	int TimeOut = 1000;
//	u32 slv_reg0, slv_reg1, slv_reg2, slv_reg3;
	char respuesta = 0;
	int i = 0;

//	CLEAR_SCREEN;

	LOG(0, "--------------------- INICIO MAIN -----------------------");
	LOG(1, "PRUEBA SOLO DE LAS INTERRUPCIONES DE MASTER_TEST");

	ZMODADC1410_Init();

	TAR_Init(TAR_TRANSFER_COUNT);

	do
	{
		i ++;
		LOG(0, "--------------------- (%d) -----------------------", i);
		ASSERT_SUCCESS(
				SetupIntrSystem(&Intc, &AxiDma, DMA_RX_INTR_ID, TAR_DR_INTR_ID),
				"Fallo al inicializar el sistema de interrupciones");

		ASSERT_SUCCESS(
				DMA_Init(&AxiDma, DMA_NUMBER_OF_TRANSFERS, sizeof(u32)),
				"Fallo al inicializar el DMA.");

		LOG(1, "----- Inicio interrupciones");
		TAR_Start_master_test();
		XTime_GetTime(&tStart);

		Status = Xil_WaitForEventSet(1000000U, 1, &Error);
		if (Status == XST_SUCCESS) {
			LOG(0, "--> Receive error %d", Status);
			goto goOut;
		}

		while(dmaTransferCount < DMA_NUMBER_OF_TRANSFERS){ // Espero hasta que se den las transacciones
//			if(!(TimeOut--))
//			{
//				TimeOut = 1000;
//				//Confirmo si está prendido
//				slv_reg0 = AXI_TAR_mReadReg(TAR_BASE, TAR_CONFIG_OFF);
//				//Valor de la cuenta de muestras configurado
//				slv_reg1 = AXI_TAR_mReadReg(TAR_BASE, master_COUNT_CFG_OFF);
//				//Valor de la cuenta de muestras actual
//				slv_reg3 = AXI_TAR_mReadReg(TAR_BASE, master_COUNT_OFF);
//				//Valor de la cuenta de interrupciones lanzadas != capturadas por core
//				slv_reg2 = AXI_TAR_mReadReg(TAR_BASE, master_INTR_COUNT_OFF);
//			}
		}

		DisableIntrSystem(&Intc, DMA_RX_INTR_ID, TAR_DR_INTR_ID);

		XTime_GetTime(&tFinish);

		TAR_StopAll();

		XAxiDma_Reset(&AxiDma);

		TimeOut = 10000;
		while (TimeOut)
		{
			if (XAxiDma_ResetIsDone(&AxiDma))
				break;
			TimeOut--;
		}

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
	XAxiDma_Reset(&AxiDma);
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

void DisableIntrSystem(INTC *IntcInstancePtr, u16 RxIntrId, u16 TarIntrId)
{
	LOG(1, "DisableIntrSystem");
	XScuGic_Disconnect(IntcInstancePtr, TarIntrId);
	XScuGic_Disconnect(IntcInstancePtr, RxIntrId);
}
int SetupIntrSystem(INTC *IntcInstancePtr, XAxiDma *AxiDmaPtr, u16 DmaRxIntrId, u16 TarIntrId)
{
	LOG(1, "SetupIntrSystem");

	XAxiDma_BdRing *RxRingPtr = XAxiDma_GetRxRing(AxiDmaPtr);
	XScuGic_Config *IntcConfig;

	IntcConfig = XScuGic_LookupConfig(INTC_DEVICE_ID);
	if (NULL == IntcConfig) {
		return XST_FAILURE;
	}

	//Inicializar el controlador de interrupciones
	ASSERT_SUCCESS(
			XScuGic_CfgInitialize(IntcInstancePtr, IntcConfig, IntcConfig->CpuBaseAddress),
			"Falla al inicializar las configuraciones de INTC");

	//Interrupción TAR
	XScuGic_SetPriorityTriggerType(IntcInstancePtr, TarIntrId, 0xA0, 0x3);
	ASSERT_SUCCESS(
			XScuGic_Connect(IntcInstancePtr, TarIntrId, (Xil_InterruptHandler)TAR_IntrHandler, (void *)TAR_BASE),
			"Failed to connect interruption handler TAR_IntrHandler");
	XScuGic_Enable(IntcInstancePtr, TarIntrId);

	//Interrupción DMA
	XScuGic_SetPriorityTriggerType(IntcInstancePtr, DmaRxIntrId, 0xA0, 0x3);
	ASSERT_SUCCESS(
			XScuGic_Connect(IntcInstancePtr, DmaRxIntrId, (Xil_InterruptHandler)DMA_RxIntrHandler, RxRingPtr),
			"Failed to connect interruption handler DMA_RxIntrHandler");
	XScuGic_Enable(IntcInstancePtr, DmaRxIntrId);

	//Interrupción de hardware
	Xil_ExceptionInit();
	Xil_ExceptionRegisterHandler(
			XIL_EXCEPTION_ID_INT,
			(Xil_ExceptionHandler)INTC_HANDLER,
			(void *)IntcInstancePtr);
	Xil_ExceptionEnable();

	return XST_SUCCESS;
}




















