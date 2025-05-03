/*
 * tar_config.h
 *
 *  Created on: Mar 14, 2025
 *      Author: sebas
 */

#ifndef SRC_AXITAR_AXITAR_H_
#define SRC_AXITAR_AXITAR_H_

#include "xparameters.h"
#include "xil_types.h"
#include "xil_io.h"
#include "AXI_TAR.h"

#define AXITAR_AXIDMA_TRANSFER_LEN	sizeof(u64)//sizeof(u32)

#define AXITAR_DR_INTR_ID 			XPAR_FABRIC_AXI_TAR_0_INTROUT_INTR

#define AXITAR_BASE					XPAR_AXI_TAR_0_S00_AXI_BASEADDR

#define AXITAR_DISABLE_CH_MASK		0x3FFF3FFF
#define AXITAR_LowHist(p)			(u16)((u32)p & 0xFFFF) 	& 0x3FFF
#define AXITAR_HighHist(p)			(u16)((u32)p >> 16)		& 0x3FFF

// General config
#define AXITAR_CONFIG_OFF			AXI_TAR_S00_AXI_SLV_REG0_OFFSET
#define AXITAR_StopAll_() 			AXI_TAR_mWriteReg(AXITAR_BASE, AXITAR_CONFIG_OFF, 0x00)

// TAR
#define AXITAR_CH0_HIST_OFF			AXI_TAR_S00_AXI_SLV_REG1_OFFSET
#define AXITAR_CH1_HIST_OFF			AXI_TAR_S00_AXI_SLV_REG2_OFFSET
#define AXITAR_Start_()				AXI_TAR_mWriteReg(AXITAR_BASE, AXITAR_CONFIG_OFF, 0x01)

// master_test
#define master_COUNT_CFG_OFF		AXI_TAR_S00_AXI_SLV_REG1_OFFSET
#define master_COUNT_OFF			AXI_TAR_S00_AXI_SLV_REG3_OFFSET
#define master_INTR_COUNT_OFF		AXI_TAR_S00_AXI_SLV_REG2_OFFSET
#define AXITAR_Start_master_test_()	AXI_TAR_mWriteReg(AXITAR_BASE,AXITAR_CONFIG_OFF, 0x10);

//#define AXITAR_MASTER_TRANSFER_PERIOD_ms 	1
#define AXITAR_MASTER_TRANSFER_PERIOD_us 	1

#ifdef 	AXITAR_MASTER_TRANSFER_PERIOD_ms
#if 	AXITAR_MASTER_TRANSFER_PERIOD_ms == 1
#define AXITAR_TRANSFER_PERIOD 	100000//1ms
#elif 	AXITAR_MASTER_TRANSFER_PERIOD_ms==10
#define AXITAR_TRANSFER_PERIOD 	1000000//10ms
#elif 	AXITAR_MASTER_TRANSFER_PERIOD_ms == 100
#define AXITAR_TRANSFER_PERIOD 	10000000//100ms
#elif 	AXITAR_MASTER_TRANSFER_PERIOD_ms == 1000
#define AXITAR_TRANSFER_PERIOD 	100000000//1000ms
#elif 	AXITAR_MASTER_TRANSFER_PERIOD_ms == .1f
#define AXITAR_TRANSFER_PERIOD  10000
#endif
#endif

#ifdef 	AXITAR_MASTER_TRANSFER_PERIOD_us
#if 	AXITAR_MASTER_TRANSFER_PERIOD_us == 100
#define AXITAR_TRANSFER_PERIOD 	10000//100us
#elif 	AXITAR_MASTER_TRANSFER_PERIOD_us == 10
#define AXITAR_TRANSFER_PERIOD 	1000//10us
#elif 	AXITAR_MASTER_TRANSFER_PERIOD_us == 1
#define AXITAR_TRANSFER_PERIOD 	500//1us
#endif
#endif

extern u32 axiTarTransferCount;

void AXITAR_master_test_Init(u32);

void AXITAR_Init();
int AXITAR_SetupRx();
void AXITAR_Start();
void AXITAR_Stop();
void AXITAR_PrintConfigLog(int);

void AXITAR_DisableChannel(int channel);
void AXITAR_SetHysteresis(int channel, u32 hist);

#endif /* SRC_AXITAR_AXITAR_H_ */
