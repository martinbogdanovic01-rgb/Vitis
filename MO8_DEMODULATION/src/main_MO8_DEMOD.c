#include <stdio.h>
#include <stdint.h>
#include "platform.h"
#include "xil_printf.h"
#include "xparameters.h"
#include "sleep.h"

#include "xspips.h"
#include <stdint.h>
#include "xstatus.h"
#include "DEMOD_CC1101_settings.h"
#include "DEMOD_CC1101_SPI.h"

// PARTNUM register (0x30 | 0xC0 = 0xF0 for status reg burst-read)
#define CC1101_REG_PARTNUM  0x30   // status register, needs 0x80|0x40 = 0xC0 prefix
#define CC1101_REG_VERSION  0x31


int main(void)
{
	init_platform();

	xil_printf("\r\n=== CC1101 SPI Init ===\r\n");
	// Single point of SPI initialization - CC1101.c owns the instance
	if (CC1101_Init_Hardware(XPAR_PS7_SPI_0_DEVICE_ID) != XST_SUCCESS) {
		xil_printf("FATAL: SPI init failed\r\n");
		return -1;
	}

	// Verify CC1101 is responding: PARTNUM should read 0x00, VERSION 0x04
	uint8_t partnum = CC1101_ReadStatus(0x30);
	uint8_t version  = CC1101_ReadStatus(0x31);

	xil_printf("PARTNUM = 0x%02X (expect 0x00)\r\n", partnum);
	xil_printf("VERSION  = 0x%02X (expect 0x14)\r\n", version);

	if (partnum != 0x00 || version != 0x14) {
		xil_printf("WARNING: Unexpected chip ID - check wiring and SPI mode\r\n");
		// Do not proceed - likely a wiring or CS polarity issue
		return -1;
	}

	// Verify a written register reads back correctly (e.g. CHANNR = 0x00)
	uint8_t channr = CC1101_ReadReg(0x0A);
	xil_printf("CHANNR readback = 0x%02X (expect 0x00)\r\n", channr);

	CC1101_Setup_GFSK();

	// Flush RX FIFO
	CC1101_Strobe(0x3A);  // SFRX
	usleep(100);

	/* 5. Enter RX mode */
	CC1101_Strobe(0x34);  // SRX
	xil_printf("Listening for data...\r\n");

	while (1)
	{
		/* Read RXBYTES — how many bytes are waiting in the RX FIFO */
		uint8_t rxbytes = CC1101_ReadStatus(0x3B);

		/* Bit 7 of RXBYTES indicates overflow — flush and restart if set */
		if (rxbytes & 0x80) {
			xil_printf("RX FIFO overflow, flushing...\r\n");
			CC1101_Strobe(0x36);  // SIDLE
			usleep(100);
			CC1101_Strobe(0x3A);  // SFRX flush
			usleep(100);
			CC1101_Strobe(0x34);  // SRX restart
			continue;
		}

		/* If there are bytes waiting, read them */
		if (rxbytes > 0) {
			uint8_t i;
			for (i = 0; i < rxbytes; i++) {
				/* Read one byte from RX FIFO (address 0x3F with read bit) */
				uint8_t data = CC1101_ReadReg(0x3F);
				xil_printf("RX: 0x%02X (%d)\r\n", data, data);
			}

			/* Go back to RX mode after reading
			 * CC1101 can drop out of RX after receiving, so re-strobe */
			CC1101_Strobe(0x34);  // SRX
		}

		usleep(1000);  // 1ms poll interval
	}

	cleanup_platform();
	return 0;
}
