/***************************************************************
 * Nombre del Proyecto : Registrador de Amplitud y Tiempo (TAR)
 * Archivo             : usb.h
 * Descripción         :
 * Autor               : Sebastián Nahuel Gallo
 * Fecha de creación   : 09/02/2026
 * Fecha de modificación:
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

/***************************** Include Files *********************************/
#include "../USB/usb.h"

#include "xparameters.h"		/* XPAR parameters */
#include "xusbps.h"			/* USB controller driver */
#include "xscugic.h"
#include "xusbps_ch9.h"		/* Generic Chapter 9 handling code */
#include "xusbps_windows.h"
#include "xil_exception.h"
#include "xpseudo_asm.h"
#include "xreg_cortexa9.h"
#include "xil_cache.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../mefs/mefCommand.h"
#include "../interruptSystem/interruptSystem.h"

/************************** Constant Definitions *****************************/
#define MEMORY_SIZE (64 * 1024)
#define INPUT_BUFFER_SIZE   256

u8 Buffer[MEMORY_SIZE] ALIGNMENT_CACHELINE;

/**************************** Type Definitions *******************************/

/***************** Macros (Inline Functions) Definitions *********************/

/************************** Function Prototypes ******************************/

static void XUsbPs_Ep0EventHandler(void *CallBackRef, u8 EpNum, u8 EventType, void *Data);
static void XUsbPs_Ep1EventHandler(void *CallBackRef, u8 EpNum, u8 EventType, void *Data);

/************************** Variable Definitions *****************************/

static XUsbPs UsbInstance;	/* The instance of the USB Controller */

static volatile u8  BufferInput[INPUT_BUFFER_SIZE];
static volatile u32 InputWrIdx = 0;
static volatile u32 InputRdIdx = 0;

static volatile int tx_is_ready = 1;

Intr_Config usbIntrConfig;

/****************************************************************************/

int USB_Init() {
	int	Status;
	u8	*MemPtr = NULL;
	int	ReturnStatus = XST_FAILURE;
	const u8 NumEndpoints = 2;

	XUsbPs_Config *UsbConfigPtr;
	XUsbPs_DeviceConfig	DeviceConfig;
	XUsbPs *UsbInstancePtr = &UsbInstance;

	UsbConfigPtr = XUsbPs_LookupConfig(XPAR_XUSBPS_0_DEVICE_ID);
	if (NULL == UsbConfigPtr) {
		goto out;
	}


	Status = XUsbPs_CfgInitialize(UsbInstancePtr, UsbConfigPtr, UsbConfigPtr->BaseAddress);
	if (XST_SUCCESS != Status) {
		goto out;
	}

	usbIntrConfig.IntrId = XPAR_XUSBPS_0_INTR;
	usbIntrConfig.Handler = (Xil_ExceptionHandler)XUsbPs_IntrHandler;
	usbIntrConfig.CallBackRef = (void *)UsbInstancePtr;
	usbIntrConfig.Priority = 0xC0;

	IntrSystem_AddHandler(&usbIntrConfig);

	DeviceConfig.EpCfg[0].Out.Type			= XUSBPS_EP_TYPE_CONTROL;
	DeviceConfig.EpCfg[0].Out.NumBufs		= 2;
	DeviceConfig.EpCfg[0].Out.BufSize		= 64;
	DeviceConfig.EpCfg[0].Out.MaxPacketSize	= 64;
	DeviceConfig.EpCfg[0].In.Type			= XUSBPS_EP_TYPE_CONTROL;
	DeviceConfig.EpCfg[0].In.NumBufs		= 2;
//	DeviceConfig.EpCfg[0].In.BufSize		= No se configura. Se provee con XUsbPs_EpBufferSend();
	DeviceConfig.EpCfg[0].In.MaxPacketSize	= 64;

	DeviceConfig.EpCfg[1].Out.Type			= XUSBPS_EP_TYPE_BULK;
	DeviceConfig.EpCfg[1].Out.NumBufs 		= USB_NUM_BUFS;
	DeviceConfig.EpCfg[1].Out.BufSize 		= USB_MAX_PACKET_SIZE;
	DeviceConfig.EpCfg[1].Out.MaxPacketSize	= USB_MAX_PACKET_SIZE;
	DeviceConfig.EpCfg[1].In.Type			= XUSBPS_EP_TYPE_BULK;
	DeviceConfig.EpCfg[1].In.NumBufs		= USB_NUM_BUFS;
//	DeviceConfig.EpCfg[1].In.BufSize		= No se configura. Se provee con XUsbPs_EpBufferSend();
	DeviceConfig.EpCfg[1].In.MaxPacketSize	= USB_MAX_PACKET_SIZE;

	DeviceConfig.NumEndpoints = NumEndpoints;

	MemPtr = (u8 *)&Buffer[0];
	memset(MemPtr,0,MEMORY_SIZE);
	Xil_DCacheFlushRange((unsigned int)MemPtr, MEMORY_SIZE);

	DeviceConfig.DMAMemPhys = (u32) MemPtr;

	Status = XUsbPs_ConfigureDevice(UsbInstancePtr, &DeviceConfig);
	if (XST_SUCCESS != Status) {
		goto out;
	}

	Status = XUsbPs_EpSetHandler(UsbInstancePtr, 0, XUSBPS_EP_DIRECTION_OUT, XUsbPs_Ep0EventHandler, UsbInstancePtr);

	Status = XUsbPs_EpSetHandler(UsbInstancePtr, 1, XUSBPS_EP_DIRECTION_OUT, XUsbPs_Ep1EventHandler, UsbInstancePtr);

	//Este handler está para capturar cuándo el host lea toda la data en el buffer de in
	Status = XUsbPs_EpSetHandler(UsbInstancePtr, 1, XUSBPS_EP_DIRECTION_IN, XUsbPs_Ep1EventHandler, UsbInstancePtr);

	XUsbPs_IntrEnable(UsbInstancePtr, XUSBPS_IXR_UR_MASK | XUSBPS_IXR_UI_MASK);

	XUsbPs_Start(UsbInstancePtr);

	ReturnStatus = XST_SUCCESS;
	return ReturnStatus;

out:
	XUsbPs_Stop(UsbInstancePtr);
	XUsbPs_IntrDisable(UsbInstancePtr, XUSBPS_IXR_ALL);
	(int) XUsbPs_IntrSetHandler(UsbInstancePtr, NULL, NULL, 0);

	IntrSystem_DisableIntr(XPAR_XUSBPS_0_INTR);

	/* Free allocated memory.
	 */
	if (NULL != UsbInstancePtr->UserDataPtr) {
		free(UsbInstancePtr->UserDataPtr);
	}

	return ReturnStatus;
}

void USB_SendBuffer(u32 sendBufferAddr, int buffSizeBytes){
	if (!tx_is_ready){
		return;
	}

	tx_is_ready = 0;

	Xil_DCacheFlushRange(
		    (UINTPTR)((u8*)sendBufferAddr),
			buffSizeBytes
		);

	XUsbPs_EpBufferSend(&UsbInstance,
						1,
						(u8 *)sendBufferAddr,
						buffSizeBytes);
}

int USB_DoneSendBuffer(){
	return tx_is_ready;
}

static void XUsbPs_Ep0EventHandler(void *CallBackRef, u8 EpNum, u8 EventType, void *Data)
{
	XUsbPs *InstancePtr;
	int Status;
	XUsbPs_SetupData SetupData;
	u8 *BufferPtr;
	u32	BufferLen;
	u32	Handle;


	Xil_AssertVoid(NULL != CallBackRef);

	InstancePtr = (XUsbPs *) CallBackRef;

	switch (EventType) {

	/* Handle the Setup Packets received on Endpoint 0. */
	case XUSBPS_EP_EVENT_SETUP_DATA_RECEIVED:
		Status = XUsbPs_EpGetSetupData(InstancePtr, EpNum, &SetupData);
		if (XST_SUCCESS == Status) {
			/* Handle the setup packet. */
			(int) XUsbPs_Ch9HandleSetupPacket(InstancePtr, &SetupData);

			(int) XUsbPs_Ch9HandleSetupPacket_WindowsSupport(InstancePtr, &SetupData);

		}
		break;

	case XUSBPS_EP_EVENT_DATA_RX:
		/* Get the data buffer. */
		Status = XUsbPs_EpBufferReceive(InstancePtr, EpNum,
					&BufferPtr, &BufferLen, &Handle);
		if (XST_SUCCESS == Status) {
			/* Return the buffer. */
			XUsbPs_EpBufferRelease(Handle);
		}
		break;

	default:
		/* Unhandled event. Ignore. */
		break;
	}
}

static void XUsbPs_Ep1EventHandler(void *CallBackRef, u8 EpNum, u8 EventType, void *Data)
{
    XUsbPs *InstancePtr = (XUsbPs *)CallBackRef;
    u8 *BufferPtr;
    u32 BufferLen, Handle;
    u32 InvalidateLen;

    switch (EventType) {
		case XUSBPS_EP_EVENT_DATA_RX:
			if (XUsbPs_EpBufferReceive(InstancePtr, EpNum, &BufferPtr, &BufferLen, &Handle) == XST_SUCCESS) {

				// Redondear hacia arriba al múltiplo de 32 más cercano
				// Xil_DCacheInvalidateRange trabaja en renglones de 32
				InvalidateLen = (BufferLen + 31) & ~31;
				Xil_DCacheInvalidateRange((UINTPTR)BufferPtr, InvalidateLen);

				for (u32 i = 0; i < BufferLen; i++) {
	//                BufferInput[InputWrIdx++ % INPUT_BUFFER_SIZE] = BufferPtr[i];
					mefCommand(BufferPtr[i]);
				}

				XUsbPs_EpBufferRelease(Handle);
			}
			break;

		case XUSBPS_EP_EVENT_DATA_TX:
			tx_is_ready = 1;
			break;

		default:
			break;
    }
}

//static int UsbSetupIntrSystem(XScuGic *IntcInstancePtr,
//			      XUsbPs *UsbInstancePtr, u16 UsbIntrId)
//{
//	int Status;
//	XScuGic_Config *IntcConfig;
//
//	/*
//	 * Initialize the interrupt controller driver so that it is ready to
//	 * use.
//	 */
//	IntcConfig = XScuGic_LookupConfig(XPAR_SCUGIC_SINGLE_DEVICE_ID);
//	if (NULL == IntcConfig) {
//		return XST_FAILURE;
//	}
//	Status = XScuGic_CfgInitialize(IntcInstancePtr, IntcConfig,
//					IntcConfig->CpuBaseAddress);
//	if (Status != XST_SUCCESS) {
//		return XST_FAILURE;
//	}
//
//	Xil_ExceptionInit();
//	/*
//	 * Connect the interrupt controller interrupt handler to the hardware
//	 * interrupt handling logic in the processor.
//	 */
//	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_IRQ_INT,
//				    (Xil_ExceptionHandler)XScuGic_InterruptHandler,
//				    IntcInstancePtr);
//	/*
//	 * Connect the device driver handler that will be called when an
//	 * interrupt for the device occurs, the handler defined above performs
//	 * the specific interrupt processing for the device.
//	 */
//	Status = XScuGic_Connect(IntcInstancePtr, UsbIntrId,
//				(Xil_ExceptionHandler)XUsbPs_IntrHandler,
//				(void *)UsbInstancePtr);
//	if (Status != XST_SUCCESS) {
//		return Status;
//	}
//	/*
//	 * Enable the interrupt for the device.
//	 */
//	XScuGic_Enable(IntcInstancePtr, UsbIntrId);
//
//	/*
//	 * Enable interrupts in the Processor.
//	 */
//	Xil_ExceptionEnableMask(XIL_EXCEPTION_IRQ);
//
//	return XST_SUCCESS;
//}
//
///*****************************************************************************/
///**
//*
//* This function disables the interrupts that occur for the USB controller.
//*
//* @param	IntcInstancePtr is a pointer to instance of the INTC driver.
//* @param	UsbIntrId is the Interrupt Id and is typically
//* 		XPAR_<INTC_instance>_<USB_instance>_VEC_ID value
//* 		from xparameters.h
//*
//* @return	None
//*
//* @note		None.
//*
//******************************************************************************/
//static void UsbDisableIntrSystem(XScuGic *IntcInstancePtr, u16 UsbIntrId)
//{
//	/* Disconnect and disable the interrupt for the USB controller. */
//	XScuGic_Disconnect(IntcInstancePtr, UsbIntrId);
//}
