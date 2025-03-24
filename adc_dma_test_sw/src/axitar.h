/*
 * tar_config.h
 *
 *  Created on: Mar 14, 2025
 *      Author: sebas
 */

#ifndef SRC_AXITAR_H_
#define SRC_AXITAR_H_

#include "xparameters.h"
#include "xil_types.h"
#include "xil_io.h"
#include "AXI_TAR.h"

#define TAR_BASE			XPAR_AXI_TAR_0_S00_AXI_BASEADDR
// General config
#define TAR_CONFIG_OFF		AXI_TAR_S00_AXI_SLV_REG0_OFFSET
#define TAR_StopAll()		AXI_TAR_mWriteReg(TAR_BASE,TAR_CONFIG_OFF, 0x00);

// TAR
#define TAR_CH1_HIST_OFF	AXI_TAR_S00_AXI_SLV_REG1_OFFSET
#define TAR_CH2_HIST_OFF	AXI_TAR_S00_AXI_SLV_REG2_OFFSET
//#define TAR_Start()			AXI_TAR_mWriteReg(TAR_BASE,TAR_CONFIG_OFF, 0x01);

// master_test
#define master_COUNT_CFG_OFF	AXI_TAR_S00_AXI_SLV_REG1_OFFSET
#define master_COUNT_OFF		AXI_TAR_S00_AXI_SLV_REG3_OFFSET
#define master_INTR_COUNT_OFF	AXI_TAR_S00_AXI_SLV_REG2_OFFSET
#define TAR_Start_master_test()	AXI_TAR_mWriteReg(TAR_BASE,TAR_CONFIG_OFF, 0x10);

#define TAR_MASTER_TRANSFER_PERIOD_ms 	1
//#define TAR_MASTER_TRANSFER_PERIOD_us 	100

#ifdef TAR_MASTER_TRANSFER_PERIOD_ms
#if TAR_MASTER_TRANSFER_PERIOD_ms == 1
#define TAR_TRANSFER_COUNT 	100000//1ms
#elif 	TAR_MASTER_TRANSFER_PERIOD_ms==10
#define TAR_TRANSFER_COUNT 	1000000//10ms
#elif 	TAR_MASTER_TRANSFER_PERIOD_ms == 100
#define TAR_TRANSFER_COUNT 	10000000//100ms
#elif 	TAR_MASTER_TRANSFER_PERIOD_ms == 1000
#define TAR_TRANSFER_COUNT 	100000000//1000ms
#elif TAR_MASTER_TRANSFER_PERIOD_ms == .1f
#define TAR_TRANSFER_COUNT  10000
#endif
#endif

#ifdef TAR_MASTER_TRANSFER_PERIOD_us
#if TAR_MASTER_TRANSFER_PERIOD_us == 100
#define TAR_TRANSFER_COUNT 	10000//100us
#endif
#endif

#define TAR_DMA_TRANSFER_LEN	sizeof(u32)

#define TAR_DR_INTR_ID		XPAR_FABRIC_AXI_TAR_0_INTROUT_INTR

extern u32 axiTarTransferCount;

void TAR_Init(u32);

#endif /* SRC_AXITAR_H_ */
