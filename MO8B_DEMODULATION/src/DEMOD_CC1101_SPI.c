#include "DEMOD_CC1101_SPI.h"

#include "xspips.h"
#include "xparameters.h"
#include "xil_printf.h"
#include "sleep.h"
#include "xstatus.h"
#include "DEMOD_CC1101_settings.h"

#include "xgpio.h"
#include "xparameters.h"

// GDO0 wired to Arduino Pin 4 = arduino_gpio_no_intr_tri_io[0]
#define GDO0_GPIO_DEVICE_ID   XPAR_ARDUINO_ARDUINO_NO_INTR_PINS_DEVICE_ID  // ID 1
#define GDO0_CHANNEL          1
#define GDO0_BIT_MASK         0x01   // bit 0 = no_intr_tri_io[0] = Arduino pin 4

static XGpio GDO0_Gpio;

int GDO0_Init(void)
{
    int Status = XGpio_Initialize(&GDO0_Gpio, GDO0_GPIO_DEVICE_ID);
    if (Status != XST_SUCCESS) {
        xil_printf("ERROR: GDO0 GPIO init failed\r\n");
        return XST_FAILURE;
    }
    // Set entire channel as input
    XGpio_SetDataDirection(&GDO0_Gpio, GDO0_CHANNEL, 0xFFFFFFFF);
    xil_printf("GDO0 GPIO initialised (Arduino pin 4)\r\n");
    return XST_SUCCESS;
}

uint8_t GDO0_Read(void)
{
    return (XGpio_DiscreteRead(&GDO0_Gpio, GDO0_CHANNEL) & GDO0_BIT_MASK) ? 1 : 0;
}
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

void CC1101_ReadBurst(uint8_t addr, uint8_t *data, uint8_t len)
{
    uint8_t tx[65];
    uint8_t rx[65];

    tx[0] = addr | 0xC0;

    for (uint8_t i = 1; i <= len; i++) {
        tx[i] = 0x00;
    }

    XSpiPs_SetSlaveSelect(&SpiInstance, CC1101_SLAVE_SELECT);

    XSpiPs_PolledTransfer(
        &SpiInstance,
        tx,
        rx,
        len + 1
    );

    XSpiPs_SetSlaveSelect(&SpiInstance, NO_SLAVE);

    for (uint8_t i = 0; i < len; i++) {
        data[i] = rx[i + 1];
    }
}
