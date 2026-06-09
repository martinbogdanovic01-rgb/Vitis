#include <stdio.h>
#include <stdint.h>
#include "platform.h"
#include "xil_printf.h"
#include "xparameters.h"
#include "sleep.h"
#include "xspips.h"
#include "xstatus.h"
#include "DEMOD_CC1101_settings.h"
#include "DEMOD_CC1101_SPI.h"

int main(void)
{
    init_platform();
    xil_printf("\r\n=== CC1101 SPI Init ===\r\n");

    if (CC1101_Init_Hardware(XPAR_PS7_SPI_0_DEVICE_ID) != XST_SUCCESS) {
        xil_printf("FATAL: SPI init failed\r\n");
        return -1;
    }

    uint8_t partnum = CC1101_ReadStatus(0x30);
    uint8_t version = CC1101_ReadStatus(0x31);
    xil_printf("PARTNUM = 0x%02X\r\n", partnum);
    xil_printf("VERSION = 0x%02X\r\n", version);

    if (partnum != 0x00 || version != 0x14) {
        xil_printf("WARNING: Unexpected chip ID - check wiring and SPI mode\r\n");
        return -1;
    }

    uint8_t channr = CC1101_ReadReg(0x0A);
    xil_printf("CHANNR readback = 0x%02X (expect 0x00)\r\n", channr);

    CC1101_Setup_GFSK();
    CC1101_WriteReg(0x17, 0x3F);  // MCSM1: stay in RX after packet received

    CC1101_Strobe(0x36);  // SIDLE
    usleep(100);
    CC1101_Strobe(0x3A);  // SFRX
    usleep(100);
    CC1101_Strobe(0x34);  // SRX
    xil_printf("Listening for data...\r\n");

    while (1)
    {
        uint8_t rxbytes = CC1101_ReadStatus(0x3B);

        if ((rxbytes & 0x7F) >= 2)
        {
            uint8_t len;
            CC1101_ReadBurst(0x3F, &len, 1);

            if (len == 0 || len > 61)
            {
                xil_printf("Invalid length: %d\r\n", len);
                CC1101_Strobe(0x36);  // SIDLE
                CC1101_Strobe(0x3A);  // SFRX
                CC1101_Strobe(0x34);  // SRX
                continue;
            }

            // Wait for full packet + 2 appended status bytes
            int watchdog = 10000;
            while (((CC1101_ReadStatus(0x3B) & 0x7F) < (len + 2)) && --watchdog);
            if (watchdog == 0)
            {
                xil_printf("Watchdog timeout waiting for full packet\r\n");
                CC1101_Strobe(0x36);  // SIDLE
                CC1101_Strobe(0x3A);  // SFRX
                CC1101_Strobe(0x34);  // SRX
                continue;
            }

            uint8_t data[64];
            uint8_t status[2];
            CC1101_ReadBurst(0x3F, data, len);
            CC1101_ReadBurst(0x3F, status, 2);

            if (status[1] & 0x80)  // CRC OK
            {
                // Reconstruct 32-bit value from 4 bytes
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

            // Only flush if RX FIFO overflow occurred
            if (rxbytes & 0x80)
            {
                CC1101_Strobe(0x36);  // SIDLE
                CC1101_Strobe(0x3A);  // SFRX
                CC1101_Strobe(0x34);  // SRX
            }
        }

        usleep(500000);  // poll every 50ms
    }

    cleanup_platform();
    return 0;
}
