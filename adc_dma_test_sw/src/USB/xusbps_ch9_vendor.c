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

#include "xusbps_ch9_vendor.h"

#include <string.h>

#include "xparameters.h"	/* XPAR parameters */
#include "xusbps.h"		/* USB controller driver */

#include "xusbps_ch9.h"

/************************** Constant Definitions *****************************/

/***************** Macros (Inline Functions) Definitions *********************/

/**************************** Type Definitions *******************************/

#ifdef __ICCARM__
#pragma pack(push, 1)
#endif

typedef struct {
	u8  bLength;
	u8  bDescriptorType;
	u16 bcdUSB;
	u8  bDeviceClass;
	u8  bDeviceSubClass;
	u8  bDeviceProtocol;
	u8  bMaxPacketSize0;
	u16 idVendor;
	u16 idProduct;
	u16 bcdDevice;
	u8  iManufacturer;
	u8  iProduct;
	u8  iSerialNumber;
	u8  bNumConfigurations;
#ifdef __ICCARM__
}USB_STD_DEV_DESC;
#pragma pack(pop)
#else
}__attribute__((__packed__))USB_STD_DEV_DESC;
#endif

#ifdef __ICCARM__
#pragma pack(push, 1)
#endif
typedef struct {
	u8  bLength;
	u8  bDescriptorType;
	u16 wTotalLength;
	u8  bNumInterfaces;
	u8  bConfigurationValue;
	u8  iConfiguration;
	u8  bmAttributes;
	u8  bMaxPower;
#ifdef __ICCARM__
}USB_STD_CFG_DESC;
#pragma pack(pop)
#else
}__attribute__((__packed__))USB_STD_CFG_DESC;
#endif

#ifdef __ICCARM__
#pragma pack(push, 1)
#endif
typedef struct {
	u8  bLength;
	u8  bDescriptorType;
	u8  bInterfaceNumber;
	u8  bAlternateSetting;
	u8  bNumEndPoints;
	u8  bInterfaceClass;
	u8  bInterfaceSubClass;
	u8  bInterfaceProtocol;
	u8  iInterface;
#ifdef __ICCARM__
}USB_STD_IF_DESC;
#pragma pack(pop)
#else
}__attribute__((__packed__))USB_STD_IF_DESC;
#endif

#ifdef __ICCARM__
#pragma pack(push, 1)
#endif
typedef struct {
	u8  bLength;
	u8  bDescriptorType;
	u8  bEndpointAddress;
	u8  bmAttributes;
	u16 wMaxPacketSize;
	u8  bInterval;
#ifdef __ICCARM__
}USB_STD_EP_DESC;
#pragma pack(pop)
#else
}__attribute__((__packed__))USB_STD_EP_DESC;
#endif

#ifdef __ICCARM__
#pragma pack(push, 1)
#endif
typedef struct {
	u8  bLength;
	u8  bDescriptorType;
	u16 wLANGID[1];
#ifdef __ICCARM__
}USB_STD_STRING_DESC;
#pragma pack(pop)
#else
}__attribute__((__packed__))USB_STD_STRING_DESC;
#endif

#ifdef __ICCARM__
#pragma pack(push, 1)
#endif
typedef struct {
	USB_STD_CFG_DESC stdCfg;
	USB_STD_IF_DESC ifCfg;
	USB_STD_EP_DESC epCfg1;
	USB_STD_EP_DESC epCfg2;
#ifdef __ICCARM__
}USB_CONFIG;
#pragma pack(pop)
#else
}__attribute__((__packed__))USB_CONFIG;
#endif

/************************** Function Prototypes ******************************/

/************************** Variable Definitions *****************************/

#define USB_ENDPOINT0_MAXP		0x40

#define USB_BULKIN_EP			1
#define USB_BULKOUT_EP			1

#define USB_DEVICE_DESC			0x01
#define USB_CONFIG_DESC			0x02
#define USB_STRING_DESC			0x03
#define USB_INTERFACE_CFG_DESC		0x04
#define USB_ENDPOINT_CFG_DESC		0x05

#define VID 0xFE1A // Inventado para FcEIA
#define PID 0x0001 // Inventado para este dispositivo
#define BCD 0x0200 // v2.0

u32 XUsbPs_Ch9SetupDevDescReply(u8 *BufPtr, u32 BufLen)
{
	USB_STD_DEV_DESC deviceDesc = {
		sizeof(USB_STD_DEV_DESC),	/* bLength */
		USB_DEVICE_DESC,		/* bDescriptorType */
		be2les(0x0210),			/* bcdUSB 2.1 soporte para Windows plug&play WinUSB */
		0xFF,				/* bDeviceClass */ // Vendor
		0x00,				/* bDeviceSubClass */
		0x00,				/* bDeviceProtocol */
		USB_ENDPOINT0_MAXP,		/* bMaxPackedSize0 */
		be2les(VID),			/* idVendor */ // Vendor inventado para FCEIA
		be2les(PID),			/* idProduct */ // ID Producto inventado
		be2les(BCD),			/* bcdDevice */ // Versión del dispositivo
		0x01,				/* iManufacturer */
		0x02,				/* iProduct */
		0x03,				/* iSerialNumber */
		0x01				/* bNumConfigurations */
	};

	/* Check buffer pointer is there and buffer is big enough. */
	if (!BufPtr) {
		return 0;
	}

	if (BufLen < sizeof(USB_STD_DEV_DESC)) {
		return 0;
	}

	memcpy(BufPtr, &deviceDesc, sizeof(USB_STD_DEV_DESC));

	return sizeof(USB_STD_DEV_DESC);
}


u32 XUsbPs_Ch9SetupCfgDescReply(u8 *BufPtr, u32 BufLen)
{
	USB_CONFIG config = {
		/* Std Config */
		{sizeof(USB_STD_CFG_DESC),	/* bLength */
		 USB_CONFIG_DESC,		/* bDescriptorType */
		 be2les(sizeof(USB_CONFIG)),	/* wTotalLength */
		 0x01,				/* bNumInterfaces */ // EP0 no cuenta
		 0x01,				/* bConfigurationValue */
		 0x04,				/* iConfiguration */
		 0xc0,				/* bmAttribute */
		 0x00},				/* bMaxPower  */

		/* Interface Config */
		{sizeof(USB_STD_IF_DESC),	/* bLength */
		 USB_INTERFACE_CFG_DESC,	/* bDescriptorType */
		 0x00,				/* bInterfaceNumber */
		 0x00,				/* bAlternateSetting */
		 0x02,				/* bNumEndPoints */ // 2 EP: EP1Out y EP1In
		 0xFF,				/* bInterfaceClass */ // Vendor
		 0x00,				/* bInterfaceSubClass */
		 0x00,				/* bInterfaceProtocol */
		 0x05},				/* iInterface */

		/* Bulk Out Endpoint Config */
		{sizeof(USB_STD_EP_DESC),	/* bLength */
		 USB_ENDPOINT_CFG_DESC,		/* bDescriptorType */
		 0x00 | USB_BULKOUT_EP,		/* bEndpointAddress */
		 0x02,				/* bmAttribute  */
		 be2les(0x200),			/* wMaxPacketSize */
		 0x00},				/* bInterval */

		/* Bulk In Endpoint Config */
		{sizeof(USB_STD_EP_DESC),	/* bLength */
		 USB_ENDPOINT_CFG_DESC,		/* bDescriptorType */
		 0x80 | USB_BULKIN_EP,		/* bEndpointAddress */
		 0x02,				/* bmAttribute  */
		 be2les(0x200),			/* wMaxPacketSize */
		 0x00}				/* bInterval */
	};

	/* Check buffer pointer is OK and buffer is big enough. */
	if (!BufPtr) {
		return 0;
	}

	if (BufLen < sizeof(USB_STD_DEV_DESC)) {
		return 0;
	}

	memcpy(BufPtr, &config, sizeof(USB_CONFIG));

	return sizeof(USB_CONFIG);
}


u32 XUsbPs_Ch9SetupStrDescReply(u8 *BufPtr, u32 BufLen, u8 Index)
{
	u32 i;

	static const char *StringList[] = {
		"UNUSED",
		"UNR-FCEIA",				//	iManufacturer
		"Dispositivo TAR",			//	iProduct
		"2A49876D9CC1AA4",			//	iSerialNumber
		"Default Configuration",	//	iConfiguration
		"Vendor Bulk Interface",	//	iInterface
	};
	const char *String;
	u32 StringLen;
	u32 DescLen;
	u8 TmpBuf[128];

	USB_STD_STRING_DESC *StringDesc;

	if (!BufPtr) {
		return 0;
	}

	if (Index >= sizeof(StringList) / sizeof(char *)) {
		return 0;
	}

	String = StringList[Index];
	StringLen = strlen(String);

	StringDesc = (USB_STD_STRING_DESC *) TmpBuf;

	/* Index 0 is special as we can not represent the string required in
	 * the table above. Therefore we handle index 0 as a special case.
	 */
	if (0 == Index) {
		StringDesc->bLength = 4;
		StringDesc->bDescriptorType = USB_STRING_DESC;
		StringDesc->wLANGID[0] = be2les(0x0409); // English - United States
	}
	/* All other strings can be pulled from the table above. */
	else {
		StringDesc->bLength = StringLen * 2 + 2; 				// +1
		StringDesc->bDescriptorType = USB_STRING_DESC; 			// +1

		for (i = 0; i < StringLen; i++) {						// +StringLen*2
			StringDesc->wLANGID[i] = be2les((u16) String[i]); 	// Unicode UTF-16LE
		}
	}
	DescLen = StringDesc->bLength;

	/* Check if the provided buffer is big enough to hold the descriptor. */
	if (DescLen > BufLen) {
		return 0;
	}

	memcpy(BufPtr, StringDesc, DescLen);

	return DescLen;
}

void XUsbPs_SetConfiguration(XUsbPs *InstancePtr, int ConfigIdx)
{
	Xil_AssertVoid(InstancePtr != NULL);

	/* We only have one configuration. Its index is 1. Ignore anything
	 * else.
	 */
	if (1 != ConfigIdx) {
		return;
	}

	XUsbPs_EpEnable(InstancePtr, 1, XUSBPS_EP_DIRECTION_OUT);
	XUsbPs_EpEnable(InstancePtr, 1, XUSBPS_EP_DIRECTION_IN);

	/* Set BULK mode for both directions.  */
	XUsbPs_SetBits(InstancePtr, XUSBPS_EPCR1_OFFSET,
						XUSBPS_EPCR_TXT_BULK_MASK |
						XUSBPS_EPCR_RXT_BULK_MASK |
						XUSBPS_EPCR_TXR_MASK |
						XUSBPS_EPCR_RXR_MASK);

	/* Prime the OUT endpoint. */
	XUsbPs_EpPrime(InstancePtr, 1, XUSBPS_EP_DIRECTION_OUT);
}

void XUsbPs_SetConfigurationApp(XUsbPs *InstancePtr,
					XUsbPs_SetupData *SetupData)
{
	(void)InstancePtr;
	(void)SetupData;
}

void XUsbPs_SetInterfaceHandler(XUsbPs *InstancePtr,
					XUsbPs_SetupData *SetupData)
{
	(void)InstancePtr;
	(void)SetupData;
}
