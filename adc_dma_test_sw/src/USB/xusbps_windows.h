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

#ifndef XUSBPS_WINDOWS_H
#define XUSBPS_WINDOWS_H


/***************************** Include Files *********************************/

#include "xusbps.h"		/* USB controller driver */

/************************** Constant Definitions *****************************/

#define XUSBPS_CMD_BOSREQ	0x0F

#define USB_DEVICE_CAPABILITY_DESCRIPTOR     0x10
#define USB_PLATFORM_CAPABILITY_TYPE         0x05

#define MS_OS_20_VENDOR_CODE                 0x20
#define MS_OS_20_DESCRIPTOR_INDEX            0x07

#define MS_OS_20_SET_HEADER_DESCRIPTOR       0x00
#define MS_OS_20_SUBSET_HEADER_CONFIGURATION 0x01
#define MS_OS_20_SUBSET_HEADER_FUNCTION      0x02
#define MS_OS_20_FEATURE_COMPATIBLE_ID       0x03

#define MS_OS_20_DESCRIPTOR_SET_TOTAL_LEN    46

/**************************** Type Definitions *******************************/

/***************** Macros (Inline Functions) Definitions *********************/

/************************** Function Prototypes ******************************/

int XUsbPs_Ch9HandleSetupPacket_WindowsSupport(XUsbPs *InstancePtr, XUsbPs_SetupData *SetupData);
int XUsbPs_HandleVendorReq(XUsbPs *InstancePtr, XUsbPs_SetupData *SetupData);

#endif /* XUSBPS_WINDOWS_H */
