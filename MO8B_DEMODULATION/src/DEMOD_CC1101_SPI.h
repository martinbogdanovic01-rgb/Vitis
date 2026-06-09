#ifndef CC1101_H
#define CC1101_H

#include "xspips.h"
#include <stdint.h>

#define SPI_DEVICE_ID       XPAR_XSPIPS_0_DEVICE_ID
#define CC1101_SLAVE_SELECT 0
#define NO_SLAVE            0x0F
// PARTNUM register (0x30 | 0xC0 = 0xF0 for status reg burst-read)
#define CC1101_REG_PARTNUM  0x30   // status register, needs 0x80|0x40 = 0xC0 prefix
#define CC1101_REG_VERSION  0x31


// Single shared SPI instance
extern XSpiPs SpiInstance;
int GDO0_Init(void);
uint8_t GDO0_Read(void);
int     CC1101_Init_Hardware(uint16_t DeviceId);
void    CC1101_WriteReg(uint8_t addr, uint8_t value);
uint8_t CC1101_ReadReg(uint8_t addr);
void    CC1101_Strobe(uint8_t command);
void    CC1101_Setup_GFSK(void);
uint8_t CC1101_ReadStatus(uint8_t addr);
void CC1101_ReadBurst(uint8_t addr, uint8_t *data, uint8_t len);

#endif
