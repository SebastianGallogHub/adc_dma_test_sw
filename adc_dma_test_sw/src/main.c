
#include <stdio.h>
#include "xaxidma.h"
#include "xparameters.h"
#include "xil_exception.h"
#include "xdebug.h"
#include "xil_util.h"
#include "xil_io.h"
#include "xtime_l.h"
#include "xscugic.h"

#include <stdlib.h>
#include <unistd.h>
#include "xil_printf.h"
#include "xparameters.h"
#include "./Zmod/zmod.h"
#include "./ZmodADC1410/zmodadc1410.h"

#define TRANSFER_LEN 0x400
// ZMOD ADC parameters
#define ZMOD_ADC_BASE_ADDR  XPAR_AXI_ZMODADC1410_0_S00_AXI_BASEADDR
#define DMA_ADC_BASE_ADDR  XPAR_AXI_DMA_ADC_BASEADDR
#define IIC_BASE_ADDR  XPAR_PS7_I2C_1_BASEADDR
#define FLASH_ADDR_ADC   0x30
#define ZMOD_ADC_IRQ  XPAR_FABRIC_AXI_ZMODADC1410_0_LIRQOUT_INTR
#define DMA_ADC_IRQ  XPAR_FABRIC_AXI_DMA_ADC_S2MM_INTROUT_INTR

int main(){
	xil_printf("CARGADO CORRECTAMENTE");
	return 0;
}


