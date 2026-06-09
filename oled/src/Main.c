#include "platform.h"
#include "xil_printf.h"
#include "SH1106_Screen.h"
#include "sleep.h"

int main() {
    init_platform();

    // 1. Check serial console. Does it say "Display Initialized"?
    initDisplay();

    while (1) {
        // 2. Clear the screen and print "OK"
        printNew(10, 10, "SYSTEM READY");
        printDisplay(10, 30, "I2C WORKING");

        xil_printf("Loop running...\r\n");
        sleep(1);
    }

    cleanup_platform();
    return 0;
}
