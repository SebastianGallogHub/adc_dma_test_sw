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

/***************************** Include Files *********************************/

#include "xusbps_windows.h"

#include <string.h>

#include "xusbps.h"		/* USB controller driver */

#include "xusbps_ch9.h"

#include "../includes/log.h"

/************************** Constant Definitions *****************************/

const uint8_t bos_descriptor[] = {

	// ---- BOS Descriptor ----
	0x05,                               // bLength
	XUSBPS_CMD_BOSREQ,            		// bDescriptorType = 0x0F
	0x21, 0x00,                         // wTotalLength = 33 bytes
	0x01,                               // bNumDeviceCaps

	// ---- MS OS 2.0 Platform Capability Descriptor ----
	0x1C,                               // bLength = 28
	USB_DEVICE_CAPABILITY_DESCRIPTOR,   // bDescriptorType = 0x10
	USB_PLATFORM_CAPABILITY_TYPE,       // bDevCapabilityType = 0x05
	0x00,                               // bReserved

	// MS OS 2.0 UUID {DFBCF3C0-7EDA-433E-B3A2-113653131010}
	0xDF,0x60,0xDD,0xD8,0x89,0x45,0xC7,0x4C,0x9C,0xD2,0x65,0x9D,0x9E,0x64,0x8A,0x9F,

	// dwWindowsVersion (Windows 8.1)
	0x00,0x00,0x03,0x06,

	// wMSOSDescriptorSetTotalLength = 46
	MS_OS_20_DESCRIPTOR_SET_TOTAL_LEN, 0x00,

	// bVendorCode
	MS_OS_20_VENDOR_CODE,

	// bAltEnumCode
	0x00
};

//const uint8_t ms_os_20_descriptor_set[46] = {
//
//	// ---- Set Header Descriptor ----
//	0x0A, 0x00,                         // wLength = 10
//	MS_OS_20_SET_HEADER_DESCRIPTOR, 0x00,
//	0x00,0x00,0x03,0x06,                // Windows 8.1
//	MS_OS_20_DESCRIPTOR_SET_TOTAL_LEN, 0x00,
//
//	// ---- Configuration Subset Header ----
//	0x08, 0x00,                         // wLength = 8
//	MS_OS_20_SUBSET_HEADER_CONFIGURATION, 0x00,
//	0x01,                               // bConfigurationValue
//	0x00,                               // bReserved
//	0x24, 0x00,                         // wTotalLength = 36
//
//	// ---- Function Subset Header ----
//	0x08, 0x00,                         // wLength = 8
//	MS_OS_20_SUBSET_HEADER_FUNCTION, 0x00,
//	0x00,                               // bFirstInterface = 0
//	0x00,                               // bReserved
//	0x1C, 0x00,                         // wSubsetLength = 28
//
//	// ---- Compatible ID Descriptor ----
//	0x14, 0x00,                         // wLength = 20
//	MS_OS_20_FEATURE_COMPATIBLE_ID, 0x00,
//
//	// CompatibleID[8] = "WINUSB\0\0"
//	'W','I','N','U','S','B',0x00,0x00,
//
//	// SubCompatibleID[8]
//	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
//};

const uint8_t ms_os_20_descriptor_set[46] = {

    // ---- Set Header Descriptor ----
    0x0A, 0x00,
    0x00, 0x00,                         // SET_HEADER_DESCRIPTOR
    0x00,0x00,0x03,0x06,
    0x2E, 0x00,                         // 46 bytes

    // ---- Configuration Subset Header ----
    0x08, 0x00,
    0x01, 0x00,                         // SUBSET_HEADER_CONFIGURATION
    0x01,
    0x00,
    0x24, 0x00,                         // 36 bytes

    // ---- Function Subset Header ----
    0x08, 0x00,
    0x02, 0x00,                         // SUBSET_HEADER_FUNCTION
    0x00,
    0x00,
    0x1C, 0x00,                         // 28 bytes

    // ---- Compatible ID Descriptor ----
    0x14, 0x00,
    0x03, 0x00,                         // FEATURE_COMPATIBLE_ID

    'W','I','N','U','S','B',0x00,0x00,

    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

/************************** Function Prototypes ******************************/

/************************** Variable Definitions *****************************/

int XUsbPs_Ch9HandleSetupPacket_WindowsSupport(XUsbPs *InstancePtr, XUsbPs_SetupData *SetupData)
{
	LOG(1, "HANDLE WINDOWS SUPPORT");
	if ((SetupData->bmRequestType == 0x80) && // standard IN
		(SetupData->bRequest == 0x06) &&      // GET_DESCRIPTOR
		((SetupData->wValue >> 8) == 0x0F))   // BOS
	{

		u16 len = SetupData->wLength;
		u16 send_len = (len < sizeof(bos_descriptor)) ? len : sizeof(bos_descriptor);

		LOG(2, "ENVIO BOS DESCRIPTOR");
		return XUsbPs_EpBufferSend(InstancePtr, 0,
								  bos_descriptor,
								  send_len);
	}

	// ---- 2. NO manejar vendor acá ----
	// lo maneja el handler original
	return XST_FAILURE; // no era nuestro
}

int XUsbPs_HandleVendorReq(XUsbPs *InstancePtr, XUsbPs_SetupData *SetupData)
{
    if ((SetupData->bmRequestType == 0xC0) &&
        (SetupData->bRequest == MS_OS_20_VENDOR_CODE) &&
        (SetupData->wIndex == 0x07))
    {
    	LOG(0, "ENVIO MS OS 20 DESCRIPTOR VendorReq: bm=%02X bReq=%02X wIndex=%04X\n",
       SetupData->bmRequestType,
       SetupData->bRequest,
       SetupData->wIndex);
//		return XUsbPs_EpBufferSend(InstancePtr, 0,
//								   ms_os_20_descriptor_set,
//								   46);

    	u16 len = SetupData->wLength;

    	if (len > sizeof(ms_os_20_descriptor_set))
    	    len = sizeof(ms_os_20_descriptor_set);

    	return XUsbPs_EpBufferSend(InstancePtr, 0,
    	                          ms_os_20_descriptor_set,
    	                          len);

    }

    return XST_FAILURE;
}
