#include "xuartlite.h"
#include "xparameters.h"
#include "xil_printf.h"
#include "xil_io.h"
#include "uart.h"

static XUartLite uartLite;

int uart_init() {
    int Status = XUartLite_Initialize(&uartLite, XPAR_AXI_UARTLITE_0_DEVICE_ID);
    if (Status != XST_SUCCESS) {
        xil_printf("UartLite init failed\r\n");
        return XST_FAILURE;
    }
    XUartLite_ResetFifos(&uartLite);
    return XST_SUCCESS;
}

int uart_data_available() {
    u32 status = Xil_In32(uartLite.RegBaseAddress + 0x08);
    return (status & 0x01) ? 1 : 0;
}

u8 uart_read_char() {
    u8 data;
    XUartLite_Recv(&uartLite, &data, 1);
    return data;
}
