/***************************************************************
 * Nombre del Proyecto : Registrador de Amplitud y Tiempo (TAR)
 * Archivo             : .h
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

/******************************************************************************
* Copyright (C) 2010 - 2021 Xilinx, Inc.  All rights reserved.
* SPDX-License-Identifier: MIT
******************************************************************************/

/*****************************************************************************/
/**
 *
 * @file xusbps_ch9_storage.h
 *
 * This file contains definitions used in the chapter 9 code.
 *
 *
 * <pre>
 * MODIFICATION HISTORY:
 *
 * Ver   Who  Date     Changes
 * ----- ---- -------- -------------------------------------------------------
 * 1.00a wgr  10/10/10 First release
 * 2.5	 pm   02/20/20 Added SetConfigurationApp and SetInterfaceHandler API
 * </pre>
 *
 ******************************************************************************/

#ifndef XUSBPS_CH9_VENDOR_H
#define XUSBPS_CH9_VENDOR_H



/***************************** Include Files *********************************/

#include "xusbps.h"
#include "xusbps_hw.h"
#include "xil_types.h"
#include "xstatus.h"

/************************** Constant Definitions *****************************/

/**************************** Type Definitions *******************************/

/***************** Macros (Inline Functions) Definitions *********************/

/* Check where these defines need to go  */
#define be2le(val)	(u32)(val)
#define be2les(x)	(u16) (x)
#define htonl(val)	((((u32)(val) & 0x000000FF)<<24) |	\
			 (((u32)(val) & 0x0000FF00)<<8)  |	\
			 (((u32)(val) & 0x00FF0000)>>8)  |	\
			 (((u32)(val) & 0xFF000000)>>24))

#define htons(x)	(u16) ((((u16)(x))<<8) | (((u16)(x))>>8))

/************************** Function Prototypes ******************************/

u32 XUsbPs_Ch9SetupDevDescReply(u8 *BufPtr, u32 BufLen);
u32 XUsbPs_Ch9SetupCfgDescReply(u8 *BufPtr, u32 BufLen);
u32 XUsbPs_Ch9SetupStrDescReply(u8 *BufPtr, u32 BufLen, u8 Index);
void XUsbPs_SetConfiguration(XUsbPs *InstancePtr, int ConfigIdx);
void XUsbPs_SetConfigurationApp(XUsbPs *InstancePtr, XUsbPs_SetupData *SetupData);
void XUsbPs_SetInterfaceHandler(XUsbPs *InstancePtr, XUsbPs_SetupData *SetupData);


#endif /* XUSBPS_CH9_VENDOR_H */
