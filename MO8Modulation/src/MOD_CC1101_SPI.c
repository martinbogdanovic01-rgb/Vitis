#include "MOD_CC1101_SPI.h"
#include "xspips.h"
#include "xparameters.h"
#include "xil_printf.h"
#include "sleep.h"
#include "xstatus.h"
#include "MOD_CC1101_settings.h"
#include "GDO0_Interrupt.h"

// Single shared instance variables owned here
XSpiPs SpiInstance;

// Global Buffers for Transmission / Data Parsing
uint8_t inputBuf[255] = {0};
uint8_t outputChunks[NUM_CHUNKS][CHUNK_SIZE];

// Global Reassembly Buffers and Tracking Variables for Reception
uint8_t rx_final_buffer[255] = {0};
uint8_t chunk_checklist[NUM_CHUNKS] = {0, 0, 0, 0, 0}; // Simple checklist for each chunk ID (0 to 4)
uint8_t unique_chunks_received = 0; // Tracks total unique chunks received

/* ============================================================= */
/* 1. HARDWARE HARDWARE DRIVER SETUP (SPI & GPIO)                */
/* ============================================================= */

int CC1101_Init_Hardware(uint16_t DeviceId)
{
	XSpiPs_Config *SpiConfig;
	int Status;

	SpiConfig = XSpiPs_LookupConfig(SPI_DEVICE_ID);
	if (SpiConfig == NULL) {
		xil_printf("SPI LookupConfig failed\r\n");
		return XST_FAILURE;
	}

	Status = XSpiPs_CfgInitialize(&SpiInstance, SpiConfig,
								   SpiConfig->BaseAddress);
	if (Status != XST_SUCCESS) {
		xil_printf("SPI CfgInitialize failed\r\n");
		return XST_FAILURE;
	}

	XSpiPs_SetOptions(&SpiInstance,
					  XSPIPS_MASTER_OPTION        |
					  XSPIPS_MANUAL_START_OPTION  |
					  XSPIPS_FORCE_SSELECT_OPTION);

	XSpiPs_SetClkPrescaler(&SpiInstance, XSPIPS_CLK_PRESCALE_64);
	XSpiPs_Enable(&SpiInstance);

	return XST_SUCCESS;
}

int CC1101_Init_Check(void)
{
	xil_printf("\r\n=== CC1101 SPI Init ===\r\n");

	// Single point of SPI initialization
	if (CC1101_Init_Hardware(XPAR_PS7_SPI_0_DEVICE_ID) != XST_SUCCESS) {
		xil_printf("FATAL: SPI init failed\r\n");
		return -1;
	}

	// Verify CC1101 is responding: PARTNUM should read 0x00, VERSION 0x14
	uint8_t partnum = CC1101_ReadStatus(PARTNUM_ADR);
	uint8_t version = CC1101_ReadStatus(VERSION_ADR);

	xil_printf("PARTNUM = 0x%02X (expect 0x00)\r\n", partnum);
	xil_printf("VERSION = 0x%02X (expect 0x14)\r\n", version);

	if (partnum != 0x00 || version != 0x14) {
		xil_printf("WARNING: Unexpected chip ID - check wiring and SPI mode\r\n");
		return -1;
	}

	// Verify a written register reads back correctly (e.g. CHANNR = 0x00)
	uint8_t channr = CC1101_ReadReg(CHANNR_ADR);
	xil_printf("CHANNR readback = 0x%02X (expect 0x00)\r\n", channr);

	xil_printf("SUCCESS: CC1101 hardware check passed!\r\n\r\n");
	return 0;
}

void CC1101_Setup_GFSK(void)
{
	uint8_t *cfg;
	int i;

	/* Reset chip using Macro */
	CC1101_Strobe(SRES_ADR);

	usleep(5000);

	/* Write config registers */
	cfg = (uint8_t *)&rfSettings;

	for (i = 0; i <= 0x2E; i++) {
		CC1101_WriteReg((uint8_t)i, cfg[i]);
	}

	/* Write PATABLE */
	{
		uint8_t pa_tx[2] = {0x7E, 0xC0};
		uint8_t pa_rx[2] = {0x00, 0x00};

		XSpiPs_SetSlaveSelect(&SpiInstance, CC1101_SLAVE_SELECT);
		XSpiPs_PolledTransfer(
			&SpiInstance,
			pa_tx,
			pa_rx,
			2
		);
		XSpiPs_SetSlaveSelect(&SpiInstance, NO_SLAVE);
	}

	xil_printf("CC1101 configured\r\n");
}

/* ============================================================= */
/* 2. TRANSMISSION & DATA PROCESSING FUNCTIONS                   */
/* ============================================================= */

void createInputBuffer(void)
{
	for(int i = 0; i < 255; i++)
	{
		inputBuf[i] = 1; // Sets exactly 1 byte to 0x01
	}
	for (int i = 0; i < NUM_CHUNKS; i++)
	{
		// Calculate the starting point in the input buffer:
		// Chunk 0 starts at inputBuf[0], Chunk 1 at inputBuf[51], etc.
		uint16_t source_offset = i * CHUNK_SIZE;

		// Copy 51 bytes into the current sub-buffer
		memcpy(outputChunks[i], &inputBuf[source_offset], CHUNK_SIZE);
	}
}

void transmitChunks(void)
{
	for (uint8_t chunk_id = 0; chunk_id < NUM_CHUNKS; chunk_id++)
	{
		// Allocate space for: Length Byte (1) + Sequence ID (1) + Data (51) = 53 bytes
		uint8_t packet[1 + 1 + CHUNK_SIZE];

		packet[0] = CHUNK_SIZE + 1; // Length byte (Sequence ID + Data = 52 payload bytes)
		packet[1] = chunk_id;       // Sequence ID (0 to 4)

		// Copy the specific 51-byte chunk into the packet
		memcpy(&packet[2], outputChunks[chunk_id], CHUNK_SIZE);

		// Prepare CC1101 FIFO using Command Strobe Macros
		CC1101_Strobe(SIDLE_ADR);
		usleep(100);
		CC1101_Strobe(SFTX_ADR);
		usleep(100);

		// Write to CC1101 TX FIFO Burst Macro
		CC1101_WriteBurst(TXFIFO_ADR, packet, packet[0] + 1);

		// Transmit
		CC1101_Strobe(STX_ADR);
		xil_printf("Sent chunk %d/5\r\n", chunk_id + 1);
		int32_t tx_timeout = 10000;
		while (((CC1101_ReadStatus(MARCSTATE_ADR) & 0x1F) == 0x13) && tx_timeout > 0)
		{
			usleep(10);
			tx_timeout--;
		}

		// Small 100ms guard delay so receiver doesn't miss the next packet frame
		usleep(100000);
	}
	CC1101_Strobe(SIDLE_ADR);
	usleep(100);
	CC1101_Strobe(SFTX_ADR);
	usleep(100);
}


/* ============================================================= */
/* 3. HIGH-LEVEL PACKET RECEPTION OPERATIONS                     */
/* ============================================================= */

void CC1101_ResetRX(void)
{
	CC1101_Strobe(SIDLE_ADR); // SIDLE Command Strobe Macro
	CC1101_Strobe(SFRX_ADR);  // SFRX Command Strobe Macro
	CC1101_Strobe(SRX_ADR);   // SRX Command Strobe Macro
}

void readPacket(void)
{
	uint8_t marc    = CC1101_ReadStatus(MARCSTATE_ADR) & 0x1F;
	uint8_t rxbytes = CC1101_ReadStatus(RXBYTES_ADR);

	/* RX FIFO overflow or radio in error state — flush and restart */
	if (marc == 0x11 || (rxbytes & 0x80))
	{
		CC1101_ResetRX();
		return;
	}

	/* Need at least the length byte + 1 data byte before reading */
	if ((rxbytes & 0x7F) < 2)
		return;

	uint8_t len;
	CC1101_ReadBurst(RXFIFO_ADR, &len, 1);

	if (len == 0 || len > 61)
	{
		xil_printf("Invalid length: %d\r\n", len);
		CC1101_ResetRX();
		return;
	}

	/* Wait until the full packet (payload + 2 status bytes) has arrived */
	int watchdog = 10000;
	while (((CC1101_ReadStatus(RXBYTES_ADR) & 0x7F) < (len + 2)) && --watchdog);

	if (watchdog == 0)
	{
		xil_printf("Watchdog timeout\r\n");
		CC1101_ResetRX();
		return;
	}

	uint8_t data[64];
	uint8_t status[2];
	CC1101_ReadBurst(RXFIFO_ADR, data,   len);
	CC1101_ReadBurst(RXFIFO_ADR, status, 2);

	if (status[1] & 0x80)
	{
		int32_t value = ((int32_t)data[0] << 24) |
						((int32_t)data[1] << 16) |
						((int32_t)data[2] << 8)  |
						 (int32_t)data[3];
		xil_printf("RX: CRC=OK DATA=%d\r\n", value);
	}
	else
	{
		xil_printf("RX: CRC=FAIL\r\n");
	}
}

void readChunks(void)
{
	uint8_t rxbytes = CC1101_ReadStatus(RXBYTES_ADR);
	uint8_t marcstate = CC1101_ReadStatus(MARCSTATE_ADR) & 0x1F;

	// 1. Check for hardware RX Overflow error
	if (marcstate == 0x11 || (rxbytes & 0x80)) {
		CC1101_ResetRX();
		return;
	}

	// 2. Wait until at least the Length Byte has arrived
	if ((rxbytes & 0x7F) < 1) {
		return;
	}

	// 3. Read the very first byte (Length Byte)
	uint8_t len;
	CC1101_ReadBurst(RXFIFO_ADR, &len, 1);

	// We expect 52 payload bytes (1 byte Chunk ID + 51 bytes Data)
	if (len != (CHUNK_SIZE + 1)) {
		CC1101_ResetRX();
		return;
	}

	// 4. Wait for the rest of this individual packet to arrive safely
	int watchdog = 10000;
	while (((CC1101_ReadStatus(RXBYTES_ADR) & 0x7F) < (len + 2)) && --watchdog);
	if (watchdog == 0) {
		CC1101_ResetRX();
		return;
	}

	// 5. Read the Second Byte (Chunk ID)
	uint8_t chunk_id;
	CC1101_ReadBurst(RXFIFO_ADR, &chunk_id, 1);

	// 6. Read the 51 Data Bytes and the 2 Status Bytes
	uint8_t chunk_data[CHUNK_SIZE];
	uint8_t status[2];
	CC1101_ReadBurst(RXFIFO_ADR, chunk_data, CHUNK_SIZE);
	CC1101_ReadBurst(RXFIFO_ADR, status, 2);

	// 7. Check the hardware CRC flag (Bit 7 of the second status byte)
	if (status[1] & 0x80)
	{
		// Ensure the received chunk ID is valid (0 to 4)
		if (chunk_id < NUM_CHUNKS) {

			/* ====================================================
			   STREAM CALIBRATION & SYNCHRONIZATION FILTER
			   ==================================================== */

			// RULE A: If our checklist is completely empty, we refuse to start
			// unless this packet is specifically Chunk 1 (chunk_id == 0).
			if (unique_chunks_received == 0 && chunk_id != 0) {
				xil_printf("RX: Stream uncalibrated. Dropping chunk %d (Waiting for Chunk 1 to anchor).\r\n", chunk_id + 1);
				return; // Exit and wait for the next packet on air
			}

			// RULE B: Stream Interruption Detection. If we already saved Chunk 1
			// previously, but we receive a brand new Chunk 1 before finishing the current
			// block, the transmitter must have restarted. Reset tracking and adapt.
			if (chunk_id == 0 && chunk_checklist[0] == 1) {
				xil_printf("RX: Detected asynchronous transmitter restart! Resetting buffer assembly.\r\n");
				memset(chunk_checklist, 0, sizeof(chunk_checklist));
				unique_chunks_received = 0;
			}

			/* ====================================================
			   DATA STORAGE & TRACKING
			   ==================================================== */

			// Check if we have already captured this specific chunk in this cycle
			if (chunk_checklist[chunk_id] == 0) {
				// Copy the 51 bytes into its designated slot in the 255-byte block
				memcpy(&rx_final_buffer[chunk_id * CHUNK_SIZE], chunk_data, CHUNK_SIZE);

				// Mark this chunk as checked off
				chunk_checklist[chunk_id] = 1;

				// Increment our count of unique chunks collected
				unique_chunks_received++;

				xil_printf("RX: Successfully stored chunk %d/5\r\n", chunk_id + 1);
			} else {
				xil_printf("RX: Duplicate chunk %d received. Ignored.\r\n", chunk_id + 1);
			}

			// 8. Check if our counter has reached the 5 unique chunks needed
			if (unique_chunks_received == NUM_CHUNKS) {
				xil_printf("SUCCESS: All 5 chunks collected sequentially! 255-byte block is ready.\r\n");

				/* ====================================================
				   YOUR CHANNEL DECODER GOES HERE
				   Pass 'rx_final_buffer' directly into your decoder.
				   ==================================================== */

				// 9. Reset tracking variables to prepare for a clean, next 255-byte stream
				memset(chunk_checklist, 0, sizeof(chunk_checklist));
				unique_chunks_received = 0;

				// 10. Hardware Cleanup: Flush the hardware RX FIFO
				CC1101_ResetRX();
			}
		}
	} else {
		xil_printf("RX: CRC check failed for chunk %d. Dropping chunk.\r\n", chunk_id + 1);
	}
}


/* ============================================================= */
/* 4. LOW-LEVEL SPI OPERATIONS & STROBES                         */
/* ============================================================= */

void CC1101_WriteReg(uint8_t addr, uint8_t value)
{
	uint8_t tx[2] = { addr & 0x3F, value };
	uint8_t rx[2] = { 0, 0 };

	XSpiPs_SetSlaveSelect(&SpiInstance, CC1101_SLAVE_SELECT);
	XSpiPs_PolledTransfer(&SpiInstance, tx, rx, 2);
	XSpiPs_SetSlaveSelect(&SpiInstance, NO_SLAVE);
}

void CC1101_WriteBurst(uint8_t addr, uint8_t *data, uint8_t len)
{
	uint8_t tx[65];
	uint8_t rx[65];

	tx[0] = addr | 0x40;

	for (uint8_t i = 0; i < len; i++) {
		tx[i + 1] = data[i];
	}

	XSpiPs_SetSlaveSelect(&SpiInstance, CC1101_SLAVE_SELECT);
	XSpiPs_PolledTransfer(&SpiInstance, tx, rx, len + 1);
	XSpiPs_SetSlaveSelect(&SpiInstance, NO_SLAVE);
}

uint8_t CC1101_ReadReg(uint8_t addr)
{
	uint8_t tx[2] = { (addr & 0x3F) | 0x80, 0x00 };
	uint8_t rx[2] = { 0, 0 };

	XSpiPs_SetSlaveSelect(&SpiInstance, CC1101_SLAVE_SELECT);
	XSpiPs_PolledTransfer(&SpiInstance, tx, rx, 2);
	XSpiPs_SetSlaveSelect(&SpiInstance, NO_SLAVE);
	return rx[1];
}

uint8_t CC1101_ReadStatus(uint8_t addr)
{
	uint8_t tx[2] = { (addr & 0x3F) | 0xC0, 0x00 };
	uint8_t rx[2] = { 0, 0 };

	XSpiPs_SetSlaveSelect(&SpiInstance, CC1101_SLAVE_SELECT);
	usleep(10);
	XSpiPs_PolledTransfer(&SpiInstance, tx, rx, 2);
	XSpiPs_SetSlaveSelect(&SpiInstance, NO_SLAVE);
	return rx[1];
}

void CC1101_ReadBurst(uint8_t addr, uint8_t *data, uint8_t len)
{
	uint8_t tx[65];
	uint8_t rx[65];

	tx[0] = addr | 0xC0;

	for (uint8_t i = 1; i <= len; i++) {
		tx[i] = 0x00;
	}

	XSpiPs_SetSlaveSelect(&SpiInstance, CC1101_SLAVE_SELECT);
	XSpiPs_PolledTransfer(&SpiInstance, tx, rx, len + 1);
	XSpiPs_SetSlaveSelect(&SpiInstance, NO_SLAVE);

	for (uint8_t i = 0; i < len; i++) {
		data[i] = rx[i + 1];
	}
}

void CC1101_Strobe(uint8_t command)
{
	uint8_t tx[1] = {command};
	uint8_t rx[1] = {0};

	XSpiPs_SetSlaveSelect(&SpiInstance, CC1101_SLAVE_SELECT);
	XSpiPs_PolledTransfer(&SpiInstance, tx, rx, 1);
	XSpiPs_SetSlaveSelect(&SpiInstance, NO_SLAVE);
}
