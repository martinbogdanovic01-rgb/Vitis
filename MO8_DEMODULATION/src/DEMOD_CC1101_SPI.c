#include "DEMOD_CC1101_SPI.h"

#include "xspips.h"
#include "xparameters.h"
#include "xil_printf.h"
#include "sleep.h"
#include "xstatus.h"
#include "DEMOD_CC1101_settings.h"

// Single SPI instance owned here
XSpiPs SpiInstance;

/* ------------------------------------------------------------- */
/* SPI INIT                                                      */
/* ------------------------------------------------------------- */
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

/* ------------------------------------------------------------- */
/* WRITE SINGLE REGISTER                                         */
/* ------------------------------------------------------------- */
void CC1101_WriteReg(uint8_t addr, uint8_t value)
{
    uint8_t tx[2] = { addr & 0x3F, value };
    uint8_t rx[2] = { 0, 0 };

    XSpiPs_SetSlaveSelect(&SpiInstance, CC1101_SLAVE_SELECT);
	/* This is what generates SCK */
	XSpiPs_PolledTransfer(&SpiInstance, tx, rx, 2);

	/* Deassert CS high */
	XSpiPs_SetSlaveSelect(&SpiInstance, NO_SLAVE);
}


/* ------------------------------------------------------------- */
/* READ SINGLE REGISTER                                          */
/* ------------------------------------------------------------- */
uint8_t CC1101_ReadReg(uint8_t addr)
{
    uint8_t tx[2] = { (addr & 0x3F) | 0x80, 0x00 };
    uint8_t rx[2] = { 0, 0 };

    XSpiPs_SetSlaveSelect(&SpiInstance, CC1101_SLAVE_SELECT);
	/* This is what generates SCK */
	XSpiPs_PolledTransfer(&SpiInstance, tx, rx, 2);
	/* Deassert CS high */
	XSpiPs_SetSlaveSelect(&SpiInstance, NO_SLAVE);
    return rx[1];
}

/* ------------------------------------------------------------- */
/* STROBE COMMAND                                                */
/* ------------------------------------------------------------- */
void CC1101_Strobe(uint8_t command)
{
	uint8_t tx[1] = {command};
    uint8_t rx[1] = {0};
    XSpiPs_SetSlaveSelect(&SpiInstance, CC1101_SLAVE_SELECT);
	/* This is what generates SCK */
	XSpiPs_PolledTransfer(&SpiInstance, tx, rx, 1);
	/* Deassert CS high */
	XSpiPs_SetSlaveSelect(&SpiInstance, NO_SLAVE);
}
/* ------------------------------------------------------------- */
/* CONFIGURE RADIO                                               */
/* ------------------------------------------------------------- */
void CC1101_Setup_GFSK(void)
{
    uint8_t *cfg;
    int i;

    /* Reset chip */
    CC1101_Strobe(0x30);

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
