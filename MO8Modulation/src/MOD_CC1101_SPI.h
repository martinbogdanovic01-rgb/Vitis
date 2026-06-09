#ifndef CC1101_H
#define CC1101_H

#include "xspips.h"
#include <stdint.h>

// --- Hardware Execution Constants ---
#define SPI_DEVICE_ID        XPAR_XSPIPS_0_DEVICE_ID
#define CC1101_SLAVE_SELECT  0
#define NO_SLAVE             0x0F

// --- Data Packing Constants ---
#define CHUNK_SIZE           51
#define NUM_CHUNKS           5

// Single shared SPI instance owned by the source file
extern XSpiPs SpiInstance;


/* ============================================================= */
/* 1. HARDWARE DRIVER SETUP (SPI & GPIO)                         */
/* ============================================================= */


int     CC1101_Init_Hardware(uint16_t DeviceId);
int     CC1101_Init_Check(void);
void    CC1101_Setup_GFSK(void);


/* ============================================================= */
/* 2. TRANSMISSION & DATA PROCESSING FUNCTIONS                   */
/* ============================================================= */

void createInputBuffer(void);
void transmitChunks(void);
void CC1101_WriteReg(uint8_t addr, uint8_t value);
void CC1101_WriteBurst(uint8_t addr, uint8_t *data, uint8_t len);


/* ============================================================= */
/* 3. HIGH-LEVEL PACKET RECEPTION OPERATIONS                     */
/* ============================================================= */

void CC1101_ResetRX(void);
void readPacket(void);
void readChunks(void);


/* ============================================================= */
/* 4. LOW-LEVEL SPI OPERATIONS & STROBES                         */
/* ============================================================= */

uint8_t CC1101_ReadReg(uint8_t addr);
uint8_t CC1101_ReadStatus(uint8_t addr);
void    CC1101_ReadBurst(uint8_t addr, uint8_t *data, uint8_t len);
void    CC1101_Strobe(uint8_t command);

#endif // CC1101_H
