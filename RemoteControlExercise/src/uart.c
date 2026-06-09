/* ---------------------------------------------------------------------------- *
 * (very) simple demo library to use the UART
 *
 * Note : This demo library shows a basic example how the UART can be used on the Xilinx Zynq platform.
 * It contains some hard coded values for the PYNQ-Z2 board
 * It is meant for educational purposes. The user of this demo code should NOT expect
 * perfect code. The demo code DOES (probably) have room for further improvement/refactoring.
 * Furthermore, the authors of this demo have done their best to test the code.
 * However they may have overlooked conditions in which the demo code may not
 * operate properly. If you notice an error or bug or improvement opportunity,
 * please be so kind to constructively report this to the authors of the demo code.
 * *
 * Authors:
 * 		Martijn Blüm, m.blum@fontys.nl
 *
 * Date: May 2025
 * ---------------------------------------------------------------------------- */

#include "xparameters.h"        // Contains hardware configuration like device ID and base address
#include "xuartps.h"            // Xilinx UART driver header
#include "xil_printf.h"
#include "uart.h"

// Declare the UART instance
XUartPs uartInstance;

// Initialize the UART using XUartPs_CfgInitialize
int uart_init() {
    int Status;

    // Get the configuration structure for the UART device
    XUartPs_Config *uartConfig = XUartPs_LookupConfig(XPAR_XUARTPS_0_DEVICE_ID); // Look up the configuration for the UART

    if (uartConfig == NULL) {
        xil_printf("UART configuration not found\n");
        return XST_FAILURE;
    }

    // Initialize the UART instance using the configuration structure
    Status = XUartPs_CfgInitialize(&uartInstance, uartConfig, uartConfig->BaseAddress);
    if (Status != XST_SUCCESS) {
        xil_printf("UART initialization failed\n");
        return XST_FAILURE;
    }

    // Configure the baud rate, data format, etc.

    XUartPsFormat UartFormat;
    UartFormat.BaudRate=115200;
    UartFormat.DataBits=XUARTPS_FORMAT_8_BITS;
    UartFormat.Parity=XUARTPS_FORMAT_NO_PARITY;
    UartFormat.StopBits=XUARTPS_FORMAT_1_STOP_BIT;

    XUartPs_SetDataFormat(&uartInstance, &UartFormat);

    xil_printf("UART Initialized\n");
    return XST_SUCCESS;
}

int uart_data_available() {
    u32 BaseAddress = uartInstance.Config.BaseAddress;
    if (XUartPs_IsReceiveData(BaseAddress)) {
        return 1;  // Data available
    }
    return 0;  // No data available
}
u8 uart_read_char() {
    u8 data;

    // Read one byte from the UART
    XUartPs_Recv(&uartInstance, &data, 1);

    return data;
}

